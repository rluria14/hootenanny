/*
 * This file is part of Hootenanny.
 *
 * Hootenanny is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --------------------------------------------------------------------
 *
 * The following copyright notices are generated automatically. If you
 * have a new notice to add, please use the format:
 * " * @copyright Copyright ..."
 * This will properly maintain the copyright information. DigitalGlobe
 * copyrights will be updated automatically.
 *
 * @copyright Copyright (C) 2017, 2018, 2019, 2020, 2021 DigitalGlobe (http://www.digitalglobe.com/)
 */

#include "Conflator.h"

// Hoot
#include <hoot/core/conflate/UnifyingConflator.h>
#include <hoot/core/conflate/stats/ConflateStatsHelper.h>
#include <hoot/core/criterion/StatusCriterion.h>
#include <hoot/core/io/MapStatsWriter.h>
#include <hoot/core/io/OsmMapWriterFactory.h>
#include <hoot/core/ops/NamedOp.h>
#include <hoot/core/util/ConfigOptions.h>
#include <hoot/core/io/IoUtils.h>
#include <hoot/core/util/Log.h>
#include <hoot/core/criterion/ReviewScoreCriterion.h>
#include <hoot/core/criterion/ReviewRelationCriterion.h>
#include <hoot/core/util/StringUtils.h>
#include <hoot/core/visitors/CountUniqueReviewsVisitor.h>
#include <hoot/core/util/ConfigUtils.h>
#include <hoot/core/elements/VersionUtils.h>
#include <hoot/core/ops/RemoveRoundabouts.h>
#include <hoot/core/ops/ReplaceRoundabouts.h>
#include <hoot/core/io/ChangesetStatsFormat.h>
#include <hoot/core/util/FileUtils.h>
#include <hoot/core/conflate/SuperfluousConflateOpRemover.h>
#include <hoot/core/util/MemoryUsageChecker.h>
#include <hoot/core/ops/RoadCrossingPolyReviewMarker.h>
#include <hoot/core/elements/MapProjector.h>

// Qt
#include <QFileInfo>
#include <QDir>
#include <QElapsedTimer>

// Standard
#include <fstream>

namespace hoot
{

const QString Conflator::JOB_SOURCE = "Conflate";

Conflator::Conflator() :
_isDiffConflate(false),
_diffConflateSeparateOutput(false),
_displayStats(false),
_displayChangesetStats(false),
_filterOps(ConfigOptions().getConflateRemoveSuperfluousOps()),
_numTotalTasks(0),
_currentTask(0),
_maxFilePrintLength(ConfigOptions().getProgressVarPrintLengthMax())
{
}

void Conflator::_initConfig()
{
  ConfigUtils::checkForTagValueTruncationOverride();
  QStringList allOps = ConfigOptions().getConflatePreOps();
  allOps += ConfigOptions().getConflatePostOps();
  ConfigUtils::checkForDuplicateElementCorrectionMismatch(allOps);

  // The highway.merge.tags.only option only gets used with Attribute Conflation for now, so we'll
  // use it as the sole identifier for it. If that ever changes, then we'll need a different way
  // to recognize when AC is occurring.
  const bool isAttributeConflate = ConfigOptions().getHighwayMergeTagsOnly();
  if (isAttributeConflate)
  {
    _updateConfigOptionsForAttributeConflation();
  }

  if (_isDiffConflate)
  {
    _updateConfigOptionsForDifferentialConflation();
  }
  if (_isDiffConflate || isAttributeConflate)
  {
    _disableRoundaboutRemoval();
  }

  // Note that we may need to eventually further restrict this to only data with relation having oob
  // members due to full hydration (would then need to move this code to after the data load).
  if (ConfigUtils::boundsOptionEnabled())
  {
    _updateConfigOptionsForBounds();
  }

  if (_filterOps)
  {
    // Let's see if we can remove any ops in the configuration that will have no effect on the
    // feature types we're conflating in order to improve runtime performance.
    SuperfluousConflateOpRemover::removeSuperfluousOps();
  }
}

void Conflator::_initTaskCount()
{
  // The number of steps here must be updated as you add/remove job steps in the logic.
  _numTotalTasks = 5;
  if (_displayStats)
  {
    _numTotalTasks += 3;
  }
  if (_isDiffConflate)
  {
    _numTotalTasks++;
  }

  // Only add one task for each set of conflate ops, since NamedOp will create its own task step for
  // each op internally.
  if (ConfigOptions().getConflatePreOps().size() > 0)
  {
    _numTotalTasks++;
  }
  if (ConfigOptions().getConflatePostOps().size() > 0)
  {
    _numTotalTasks++;
  }
  _currentTask = 1;
}

void Conflator::conflate(const QString& input1, const QString& input2, QString& output)
{
  Tgs::Timer totalTime;
  _taskTimer.reset();
  _progress.reset(
    new Progress(ConfigOptions().getJobId(), JOB_SOURCE, Progress::JobState::Running));
  OsmMapPtr map(new OsmMap());
  const bool isChangesetOutput = output.endsWith(".osc") || output.endsWith(".osc.sql");
  double bytesRead = IoSingleStat(IoSingleStat::RChar).value;
  LOG_VART(bytesRead);
  _allStats.clear();
  _stats.clear();

  _initConfig();
  _initTaskCount();
  if (!IoUtils::isUrl(output))
  {
    // Write the output dir now so we don't get a nasty surprise at the end of a long job that it
    // can't be written.
    IoUtils::writeOutputDir(output);
  }

  QString msg =
    "Conflating ..." + input1.right(_maxFilePrintLength) + " with ..." +
    input2.right(_maxFilePrintLength);
  if (ConfigUtils::boundsOptionEnabled())
  {
    msg += " over bounds: ..." + ConfigUtils::getBoundsString().right(_maxFilePrintLength);
  }
  msg += " and writing the output to ..." + output.right(_maxFilePrintLength) + "...";
  if (_isDiffConflate)
  {
    if (_diffConflator.conflatingTags())
    {
      msg = msg.replace("Conflating", "Differentially conflating (tags only) ");
    }
    else
    {
      msg = msg.replace("Conflating", "Differentially conflating ");
    }
  }
  _progress->set(0.0, msg);

  _load(input1, input2, map, isChangesetOutput);

  msg = "Conflating map with " + StringUtils::formatLargeNumber(map->size()) + " elements";
  if (ConfigUtils::boundsOptionEnabled())
  {
    msg += " over bounds: " + ConfigUtils::getBoundsString().right(_maxFilePrintLength);
  }
  msg += "...";
  LOG_STATUS(msg);

  double inputBytes = IoSingleStat(IoSingleStat::RChar).value - bytesRead;
  LOG_VART(inputBytes);
  double elapsed = _taskTimer.getElapsedAndRestart();
  _stats.append(SingleStat("Read Inputs Time (sec)", elapsed));
  _stats.append(SingleStat("(Dubious) Read Inputs Bytes", inputBytes));
  _stats.append(SingleStat("(Dubious) Read Inputs Bytes per Second", inputBytes / elapsed));

  CalculateStatsOp input1Cso(
    ElementCriterionPtr(new StatusCriterion(Status::Unknown1)), "input map 1");
  CalculateStatsOp input2Cso(
    ElementCriterionPtr(new StatusCriterion(Status::Unknown2)), "input map 2");
  if (_displayStats)
  {
    _progress->set(
      _getJobPercentComplete(_currentTask - 1),
      "Calculating reference statistics for: ..." + input1.right(_maxFilePrintLength) + "...");
    input1Cso.apply(map);
    _allStats.append(input1Cso.getStats());
    _stats.append(
      SingleStat("Calculate Stats for Input 1 Time (sec)", _taskTimer.getElapsedAndRestart()));
    _currentTask++;

    _progress->set(
      _getJobPercentComplete(_currentTask - 1),
      "Calculating secondary data statistics for: ..." + input2.right(_maxFilePrintLength) + "...");
    input2Cso.apply(map);
    _allStats.append(input2Cso.getStats());
    _stats.append(
      SingleStat("Calculate Stats for Input 2 Time (sec)", _taskTimer.getElapsedAndRestart()));
    _currentTask++;
  }
  MemoryUsageChecker::getInstance().check();

  size_t initialElementCount = map->getElementCount();
  _stats.append(SingleStat("Initial Element Count", initialElementCount));
  OsmMapWriterFactory::writeDebugMap(map, "after-load");

  if (ConfigOptions().getConflatePreOps().size() > 0)
  {
    _runConflateOps(map, true);
  }

  OsmMapPtr result = map;
  _runConflate(result);

  if (ConfigOptions().getConflatePostOps().size() > 0)
  {
    _runConflateOps(map, false);
  }

  // Doing this after the conflate post ops run, since some invalid reviews are removed by them.
  _progress->set(_getJobPercentComplete(_currentTask - 1), "Counting feature reviews...");
  CountUniqueReviewsVisitor countReviewsVis;
  result->visitRo(countReviewsVis);
  LOG_INFO(
    "Generated " << StringUtils::formatLargeNumber(countReviewsVis.getStat()) <<
    " feature reviews.");
  _currentTask++;

  MapProjector::projectToWgs84(result);
  _stats.append(SingleStat("Project to WGS84 Time (sec)", _taskTimer.getElapsedAndRestart()));
  OsmMapWriterFactory::writeDebugMap(result, "after-wgs84-projection");

  _writeOutput(result, output, isChangesetOutput);

  double timingOutput = _taskTimer.getElapsedAndRestart();
  double totalElapsed = totalTime.getElapsed();
  _stats.append(SingleStat("(Dubious) Initial Elements Processed per Second",
                          initialElementCount / totalElapsed));
  _stats.append(SingleStat("(Dubious) Final Elements Processed per Second",
                          result->getElementCount() / totalElapsed));
  _stats.append(SingleStat("Write Output Time (sec)", timingOutput));
  _stats.append(SingleStat("Final Element Count", result->getElementCount()));
  _stats.append(SingleStat("Total Time Elapsed (sec)", totalElapsed));
  _stats.append(IoSingleStat(IoSingleStat::RChar));
  _stats.append(IoSingleStat(IoSingleStat::WChar));
  _stats.append(IoSingleStat(IoSingleStat::SysCr));
  _stats.append(IoSingleStat(IoSingleStat::SysCw));
  _stats.append(IoSingleStat(IoSingleStat::ReadBytes));
  _stats.append(IoSingleStat(IoSingleStat::WriteBytes));
  _stats.append(IoSingleStat(IoSingleStat::CancelledWriteBytes));
  _stats.append(SingleStat("(Dubious) Bytes Processed per Second", inputBytes / totalElapsed));

  if (_displayStats)
  {
    _writeStats(result, input1Cso, input2Cso, output);
  }
  if (_displayChangesetStats)
  {
    _writeChangesetStats();
  }

  _progress->set(
    1.0, Progress::JobState::Successful,
    "Conflation job completed in " +
    StringUtils::millisecondsToDhms((qint64)(totalElapsed * 1000)) + " for reference map: ..." +
    input1.right(_maxFilePrintLength) + " and secondary map: ..." +
    input2.right(_maxFilePrintLength) + " and written to output: ..." +
    output.right(_maxFilePrintLength));
}

void Conflator::_load(const QString& input1, const QString& input2, OsmMapPtr& map,
                      const bool isChangesetOut)
{
  //  Loading order is important if datasource IDs 2 is true but 1 is not
  if (!ConfigOptions().getConflateUseDataSourceIds1() &&
       ConfigOptions().getConflateUseDataSourceIds2() &&
      !_isDiffConflate)
  {
    //  For Attribute conflation, the secondary IDs are the ones that we want
    //  to preserve.  So loading them first allows for all of them to be loaded
    //  without conflict, even if they are negative.  Then the reference dataset
    //  is loaded with negative IDs
    _progress->set(
      _getJobPercentComplete(_currentTask - 1),
      "Loading secondary map: ..." + input2.right(_maxFilePrintLength) + "...");
    IoUtils::loadMap(
      map, input2, ConfigOptions().getConflateUseDataSourceIds2(), Status::Unknown2);
    _currentTask++;

    // read input 1
    _progress->set(
      _getJobPercentComplete(_currentTask - 1),
      "Loading reference map: ..." + input1.right(_maxFilePrintLength) + "...");
    IoUtils::loadMap(map, input1, ConfigOptions().getConflateUseDataSourceIds1(),
                     Status::Unknown1);
     _currentTask++;
  }
  else
  {
    // read input 1
    _progress->set(
      _getJobPercentComplete(_currentTask - 1),
      "Loading reference map: ..." + input1.right(_maxFilePrintLength) + "...");
    IoUtils::loadMap(map, input1, ConfigOptions().getConflateUseDataSourceIds1(), Status::Unknown1);
    _currentTask++;

    if (_isDiffConflate)
    {
      if (isChangesetOut)
      {
        VersionUtils::checkVersionLessThanOneCountAndLogWarning(map);
      }

      // Store original IDs for tag diff
      _progress->set(
        _getJobPercentComplete(_currentTask - 1),
       "Storing original features for tag differential...");
      _diffConflator.storeOriginalMap(map);
      _diffConflator.markInputElements(map);
      _currentTask++;
    }

    _progress->set(
      _getJobPercentComplete(_currentTask - 1),
      "Loading secondary map: ..." + input2.right(_maxFilePrintLength) + "...");
    IoUtils::loadMap(
      map, input2, ConfigOptions().getConflateUseDataSourceIds2(), Status::Unknown2);
    _currentTask++;
  }
  MemoryUsageChecker::getInstance().check();
}

void Conflator::_runConflate(OsmMapPtr& map)
{
  if (_isDiffConflate)
  {
    _diffConflator.setProgress(
      Progress(
        ConfigOptions().getJobId(), JOB_SOURCE, Progress::JobState::Running,
        _getJobPercentComplete(_currentTask - 1), _getTaskWeight()));
    _diffConflator.apply(map);
    if (_diffConflator.conflatingTags())
    {
      _pTagChanges = _diffConflator.getTagDiff();
    }
    _stats.append(_diffConflator.getStats());
  }
  else
  {
    UnifyingConflator unifyingConflator;
    unifyingConflator.setProgress(
      Progress(
        ConfigOptions().getJobId(), JOB_SOURCE, Progress::JobState::Running,
        _getJobPercentComplete(_currentTask - 1), _getTaskWeight()));
    unifyingConflator.apply(map);
    _stats.append(unifyingConflator.getStats());
  }
  _stats.append(SingleStat("Conflation Time (sec)", _taskTimer.getElapsedAndRestart()));
  _currentTask++;
}

void Conflator::_runConflateOps(OsmMapPtr& map, const bool runPre)
{
  QStringList opNames;
  QString opStr;
  if (runPre)
  {
    // By default rubbersheeting has no filters. When conflating, we need to add the ones from the
    // config.
    conf().set(
      ConfigOptions::getRubberSheetElementCriteriaKey(),
      ConfigOptions().getConflateRubberSheetElementCriteria());

    opNames = ConfigOptions().getConflatePreOps();
    opStr = "Pre";
  }
  else
  {
    opNames = ConfigOptions().getConflatePostOps();
    opStr = "Post";
  }

  // apply any user specified pre-conflate operations
  LOG_STATUS("Running " << opStr.toLower() << "-conflate operations...");
  QElapsedTimer opsTimer;
  opsTimer.start();
  NamedOp ops(opNames);
  ops.setProgress(
    Progress(
      ConfigOptions().getJobId(), JOB_SOURCE, Progress::JobState::Running,
      _getJobPercentComplete(_currentTask - 1), _getTaskWeight()));
  ops.apply(map);
  _stats.append(
    SingleStat("Apply " + opStr + "-Conflate Ops Time (sec)", _taskTimer.getElapsedAndRestart()));
  OsmMapWriterFactory::writeDebugMap(map, "after-" + opStr.toLower() + "-ops");
  _currentTask++;
  LOG_STATUS(
    "Conflate " << opStr.toLower() << "-operations ran in " +
    StringUtils::millisecondsToDhms(opsTimer.elapsed()) << " total.");
}

void Conflator::_writeOutput(OsmMapPtr& map, QString& output, const bool isChangesetOutput)
{
  // Figure out what to write
  _progress->set(
    _getJobPercentComplete(_currentTask - 1),
    "Writing conflated output: ..." + output.right(_maxFilePrintLength) + "...");
  if (_isDiffConflate && isChangesetOutput)
  {
    // Get the changeset stats output format from the changeset stats file extension, or if no
    // extension is there assume a text table output to the display.
    ChangesetStatsFormat statsFormat;
    if (_displayChangesetStats)
    {
      if (!_outputChangesetStatsFile.isEmpty())
      {
        QFileInfo changesetStatsFileInfo(_outputChangesetStatsFile);
        statsFormat.setFormat(
          ChangesetStatsFormat::fromString(changesetStatsFileInfo.completeSuffix()));
      }
      else
      {
        statsFormat.setFormat(ChangesetStatsFormat::Text);
      }
    }
    _diffConflator.writeChangeset(
      map, output, _diffConflateSeparateOutput, statsFormat, _osmApiDbUrl);
  }
  else
  {
    // Write a map
    if (_isDiffConflate && _diffConflator.conflatingTags())
    {
      // Add tag changes to our map
      _diffConflator.addChangesToMap(map, _pTagChanges);
      _currentTask++;
    }
    IoUtils::saveMap(map, output);
    OsmMapWriterFactory::writeDebugMap(map, "after-conflate-output-write");
  }
  _currentTask++;
}

void Conflator::_writeStats(
  OsmMapPtr& map, const CalculateStatsOp& input1Cso, const CalculateStatsOp& input2Cso,
  const QString& outputFileName)
{
  _progress->set(
    _getJobPercentComplete(_currentTask - 1),
    "Calculating output data statistics for: ..." + outputFileName.right(_maxFilePrintLength) +
    "...");
  CalculateStatsOp outputCso("output map", true);
  outputCso.apply(map);
  QList<SingleStat> outputStats = outputCso.getStats();
  ConflateStatsHelper(input1Cso.getStats(), input2Cso.getStats(), outputCso.getStats())
    .updateStats(
      outputStats,
      outputCso.indexOfSingleStat("Total Unmatched Features"));
  _allStats.append(outputStats);
  _stats.append(
    SingleStat("Calculate Stats for Output Time (sec)", _taskTimer.getElapsedAndRestart()));
  _currentTask++;

  if (_isDiffConflate)
  {
    _progress->set(
      _getJobPercentComplete(_currentTask - 1),
      "Calculating differential output statistics for: ..." +
      outputFileName.right(_maxFilePrintLength) + "...");
    _diffConflator.calculateStats(map, _stats);
    _currentTask++;
  }

  _allStats.append(_stats);
  if (_outputStatsFile.isEmpty())
  {
    QString statsMsg = MapStatsWriter().statsToString(_allStats, "\t");
    std::cout << "stats = (stat) OR (input map 1 stat) (input map 2 stat) (output map stat)\n" <<
      statsMsg << std::endl;
  }
  else
  {
    MapStatsWriter().writeStatsToJson(_allStats, _outputStatsFile);
    std::cout << "stats = (stat) OR (input map 1 stat) (input map 2 stat) (output map stat) in file: " <<
      _outputStatsFile << std::endl;
  }
}

void Conflator::_writeChangesetStats()
{
  if (_outputChangesetStatsFile.isEmpty())
  {
    // output to display
    LOG_STATUS("Changeset Geometry Stats:\n" << _diffConflator.getGeometryChangesetStats());
    if (_diffConflator.conflatingTags())
    {
      LOG_STATUS("\nChangeset Tag Stats:\n" << _diffConflator.getTagChangesetStats() << "\n");
    }
  }
  else
  {
    // output to file
    if (_diffConflateSeparateOutput)
    {
      // output separate files for geometry and tag change stats
      FileUtils::writeFully(_outputChangesetStatsFile, _diffConflator.getGeometryChangesetStats());
      if (_diffConflator.conflatingTags())
      {
        QString tagsOutFile = _outputChangesetStatsFile.replace(".json", "");
        tagsOutFile.append(".tags.json");
        FileUtils::writeFully(tagsOutFile, _diffConflator.getTagChangesetStats());
      }
    }
    else
    {
      // output a single stats file with both geometry and tags change stats
      FileUtils::writeFully(_outputChangesetStatsFile, _diffConflator.getUnifiedChangesetStats());
    }
  }
}

float Conflator::_getTaskWeight() const
{
  return 1.0 / (float)_numTotalTasks;
}

float Conflator::_getJobPercentComplete(const int currentTaskNum) const
{
  return (float)currentTaskNum / (float)_numTotalTasks;
}

void Conflator::_disableRoundaboutRemoval()
{
  // This applies to both Attribute and Differential Conflation.

  // Since Differential throws out all matches, there's no way we can have a bad merge between
  // ref/secondary roundabouts. Therefore, no need to replace/remove them. If there's a match, we'll
  // end up with no secondary roundabout in the diff output and only the ref roundabout when the
  // diff is applied back to the ref.

  // Our roundabout handling used with other types of conflation makes no sense for Attribute
  // Conflation. For the other types of conflation we remove roundabouts b/c hoot isn't very good
  // at merging them together, so we keep the ref roundabouts and skip conflating them altogether.
  // Since we always keep the ref geometry in AC, there's no opportunity for bad merging and,
  // thus, no need to remove them in the first place. So, we'll override that behavior here.

  // If the work for #3442 is done, then we could handle this removal in AttributeConflation.conf
  // add DifferentialConflation.conf instead of doing it here.

  ConfigUtils::removeListOpEntry(
    ConfigOptions::getConflatePreOpsKey(), RemoveRoundabouts::className());
  ConfigUtils::removeListOpEntry(
    ConfigOptions::getConflatePostOpsKey(), ReplaceRoundabouts::className());
}

void Conflator::_updateConfigOptionsForAttributeConflation()
{
  // These are some custom adjustments to config opts that must be done for Attribute Conflation.
  // There may be a way to eliminate these by adding more custom behavior to the UI.

  const QString reviewRelationCritName = ReviewRelationCriterion::className();

  // This swaps the logic that removes all reviews with the logic that removes them based on score
  // thresholding.
  if (ConfigOptions().getAttributeConflationAllowReviewsByScore())
  {
    QStringList removeElementsCriteria =
      conf().get(ConfigOptions::getRemoveElementsVisitorElementCriteriaKey()).toStringList();
    removeElementsCriteria.replaceInStrings(
      reviewRelationCritName, ReviewScoreCriterion::className());
    conf().set(
      ConfigOptions::getRemoveElementsVisitorElementCriteriaKey(), removeElementsCriteria);
  }
}

void Conflator::_updateConfigOptionsForDifferentialConflation()
{
  // This is for custom adjustments to config opts that must be done for Differential Conflation.

  // The list option removal being done here could be made obsolete by handling it in the JSON
  // config instead with custom syntax (#3442).
  ConfigUtils::removeListOpEntry(
    ConfigOptions::getConflatePostOpsKey(), RoadCrossingPolyReviewMarker::className());
}

void Conflator::_updateConfigOptionsForBounds()
{
  // If we're working with a bounds, we need to ensure that IDs of the original ref parents created
  // by a split operation are applied to their split children.
  conf().set(ConfigOptions::getWayJoinerWriteParentIdToChildIdKey(), true);
}

}
