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
 * @copyright Copyright (C) 2015, 2017, 2018, 2019, 2020, 2021 DigitalGlobe (http://www.digitalglobe.com/)
 */

#include "ConflateCmd.h"

// Hoot
#include <hoot/core/util/Factory.h>
#include <hoot/core/util/Log.h>
#include <hoot/core/conflate/Conflator.h>

// Qt
#include <QFileInfo>

using namespace std;
using namespace Tgs;

namespace hoot
{

HOOT_FACTORY_REGISTER(Command, ConflateCmd)

int ConflateCmd::runSimple(QStringList& args)
{
  BoundedCommand::runSimple(args);

  Conflator conflator;

  // This parsing is for map stats. Changeset stats args for differential are processed further down
  // below.
  bool displayStats = false;
  QString outputStatsFile;
  if (args.contains("--stats"))
  {
    displayStats = true;
    const int statsIndex = args.indexOf("--stats");
    if (statsIndex != -1 && statsIndex != (args.size() - 1) &&
        args[statsIndex + 1].toLower().endsWith(".json"))
    {
      outputStatsFile = args[statsIndex + 1];
      args.removeAll(outputStatsFile);
    }
    args.removeAll("--stats");
  }
  LOG_VARD(displayStats);
  LOG_VARD(outputStatsFile);
  conflator.setDisplayStats(displayStats);
  conflator.setOutputStatsFile(outputStatsFile);

  bool isDiffConflate = false;
  bool diffConflateEnableTags = false;

  if (args.contains("--differential"))
  {
    isDiffConflate = true;
    conflator.setIsDiffConflate(true);
    args.removeAt(args.indexOf("--differential"));

    if (args.contains("--include-tags"))
    {
      diffConflateEnableTags = true;
      conflator.setDiffConflateEnableTags(diffConflateEnableTags);
      args.removeAt(args.indexOf("--include-tags"));
    }
  }
  LOG_VARD(isDiffConflate);

  // Check for separate output files (for geometry & tags)
  bool separateOutput = false;
  if (args.contains("--separate-output"))
  {
    const QString errorMsg =
      QString("--separate-output is only valid when combiend with the --differential and ") +
      QString("--include-tags options.");
    if (!isDiffConflate || !diffConflateEnableTags)
    {
      throw IllegalArgumentException(errorMsg);
    }
    separateOutput = true;
    args.removeAt(args.indexOf("--separate-output"));
  }
  LOG_VARD(separateOutput);
  conflator.setDiffConflateSeparateOutput(separateOutput);

  bool displayChangesetStats = false;
  QString outputChangesetStatsFile;
  if (args.contains("--changeset-stats"))
  {
    if (!isDiffConflate)
    {
      throw IllegalArgumentException(
        "--changeset-stats is only valid when combined with the --differential option.");
    }
    else
    {
      displayChangesetStats = true;
      const int statsIndex = args.indexOf("--changeset-stats");
      LOG_VARD(statsIndex);
      // If the input immediately after the changeset stats arg isn't a valid changeset stats file
      // output format, we'll just silently skip it and assume we're outputting stats to the display
      // only. This mimics how the map stats args and stats args in other commands are parsed. We
      // may want to eventually return an error or warning here instead.
      if (statsIndex != -1 && statsIndex != (args.size() - 1) &&
          !args[statsIndex + 1].startsWith("--"))
      {
        outputChangesetStatsFile = args[statsIndex + 1];
        QFileInfo changesetStatsInfo(outputChangesetStatsFile);
        if (!ChangesetStatsFormat::isValidFileOutputFormat(changesetStatsInfo.completeSuffix()))
        {
          outputChangesetStatsFile = "";
        }
        else
        {
          args.removeAll(outputChangesetStatsFile);
        }
      }
      args.removeAll("--changeset-stats");
    }
  }
  LOG_VARD(displayChangesetStats);
  LOG_VARD(outputChangesetStatsFile);
  conflator.setDisplayChangesetStats(displayChangesetStats);
  conflator.setOutputChangesetStatsFile(outputChangesetStatsFile);

  LOG_VARD(args.size() );
  if (args.size() < 3 || args.size() > 4)
  {
    cout << getHelp() << endl << endl;
    throw IllegalArgumentException(
      QString("%1 takes three or four parameters. You provided %2: %3")
        .arg(getName())
        .arg(args.size())
        .arg(args.join(",")));
  }

  const QString input1 = args[0];
  const QString input2 = args[1];
  QString output = args[2];

  QString osmApiDbUrl;
  if (output.endsWith(".osc.sql"))
  {
    if (args.size() != 4)
    {
      std::cout << getHelp() << std::endl << std::endl;
      throw IllegalArgumentException(
        QString("%1 with SQL changeset output takes four parameters.").arg(getName()));
    }
    else if (displayChangesetStats)
    {
      throw IllegalArgumentException(
        QString("Changeset statistics (--changeset-stats) may only be calculated with an XML ") +
        QString("changeset output (.osc)."));
    }
    osmApiDbUrl = args[3];
    conflator.setOsmApiDbUrl(osmApiDbUrl);
  }
  else if (args.size() > 3)
  {
    std::cout << getHelp() << std::endl << std::endl;
    throw IllegalArgumentException(
      QString("%1 with output: " + output + " takes three parameters. You provided %2: %3")
        .arg(getName())
        .arg(args.size())
        .arg(args.join(",")));
  }

  conflator.conflate(input1, input2, output);

  return 0;
}

}
