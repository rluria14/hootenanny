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
 * @copyright Copyright (C) 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 DigitalGlobe (http://www.digitalglobe.com/)
 */

#include "TestUtils.h"

// hoot
#include <hoot/core/elements/OsmMap.h>
#include <hoot/core/conflate/matching/MatchFactory.h>
#include <hoot/core/conflate/merging/MergerFactory.h>
#include <hoot/core/criterion/TagCriterion.h>
#include <hoot/core/io/OsmXmlReader.h>
#include <hoot/core/schema/TagMergerFactory.h>
#include <hoot/core/scoring/MapComparator.h>
#include <hoot/core/util/ConfigOptions.h>
#include <hoot/core/util/FileUtils.h>
#include <hoot/core/util/Log.h>
#include <hoot/core/util/UuidHelper.h>
#include <hoot/core/visitors/FilteredVisitor.h>
#include <hoot/core/visitors/UniqueElementIdVisitor.h>
#include <hoot/core/conflate/SuperfluousConflateOpRemover.h>
#include <hoot/core/util/ConfPath.h>
#include <hoot/core/io/OsmMapWriterFactory.h>

//  tgs
#include <tgs/Statistics/Random.h>

// Qt
#include <QDir>
#include <QFile>

using namespace geos::geom;
using namespace std;

namespace hoot
{

std::shared_ptr<TestUtils> TestUtils::_theInstance;

const QString HootTestFixture::UNUSED_PATH = "";

TestUtils::TestUtils()
{
}

bool TestUtils::compareMaps(const QString& refPath, const QString& testPath)
{
  OsmXmlReader reader;
  reader.setDefaultStatus(Status::Unknown1);
  reader.setUseDataSourceIds(true);
  reader.setUseFileStatus(true);
  reader.setAddSourceDateTime(false);

  OsmMapPtr ref(new OsmMap());
  OsmMapPtr test(new OsmMap());
  reader.read(refPath, ref);
  reader.read(testPath, test);
  return compareMaps(ref, test);
}

bool TestUtils::compareMaps(OsmMapPtr ref, OsmMapPtr test)
{
  return MapComparator().isMatch(ref, test);
}

NodePtr TestUtils::createNode(OsmMapPtr map, Status status, double x, double y,
                              Meters circularError, Tags tags)
{
  NodePtr result(new Node(status, map->createNextNodeId(), x, y, circularError));
  map->addNode(result);
  result->getTags().add(tags);
  return result;
}

WayPtr TestUtils::createDummyWay(OsmMapPtr map, Status status)
{
  geos::geom::Coordinate coords[] =
  { geos::geom::Coordinate(0, 0), geos::geom::Coordinate(0, 10),
    geos::geom::Coordinate::getNull() };
  return createWay(map, status, coords);
}

WayPtr TestUtils::createWay(OsmMapPtr map, Status s, Coordinate c[], Meters circularError,
                            const QString& note)
{
  WayPtr result(new Way(s, map->createNextWayId(), circularError));
  for (size_t i = 0; c[i].isNull() == false; i++)
  {
    NodePtr n(new Node(s, map->createNextNodeId(), c[i], circularError));
    map->addNode(n);
    result->addNode(n->getId());
  }

  if (!note.isEmpty())
  {
    result->getTags().addNote(note);
  }
  map->addWay(result);
  return result;
}

WayPtr TestUtils::createWay(OsmMapPtr map, geos::geom::Coordinate c[], Status status,
                            Meters circularError, Tags tags)
{
  WayPtr way(new Way(status, map->createNextWayId(), circularError));
  for (size_t i = 0; c[i].isNull() == false; i++)
  {
    NodePtr n(new Node(status, map->createNextNodeId(), c[i], circularError));
    map->addNode(n);
    way->addNode(n->getId());
  }
  way->setTags(tags);
  map->addWay(way);
  return way;
}

WayPtr TestUtils::createWay(OsmMapPtr map, const QList<NodePtr>& nodes, Status status,
                            Meters circularError, Tags tags)
{
  WayPtr way(new Way(status, map->createNextWayId(), circularError));
  foreach (NodePtr node, nodes)
  {
    map->addNode(node);
    way->addNode(node->getId());
  }
  way->setTags(tags);
  map->addWay(way);
  return way;
}

RelationPtr TestUtils::createRelation(OsmMapPtr map, const QList<ElementPtr>& elements,
  Status status, Meters circularError, Tags tags)
{
  RelationPtr relation(new Relation(status, map->createNextRelationId(), circularError));
  foreach (ElementPtr element, elements)
  {
    map->addElement(element);
    relation->addElement("test", element);
  }
  relation->setTags(tags);
  map->addRelation(relation);
  return relation;
}

void TestUtils::dumpString(const string& str)
{
  cout << "const unsigned char data[] = {";
  for (size_t i = 0; i < str.size(); i++)
  {
    if (i != 0)
    {
      cout << ", ";
    }
    if (i % 18 == 0)
    {
      cout << endl << "  ";
    }
    printf("%3u", (unsigned char)str.at(i));
  }
  cout << "};" << endl;
  cout << "size_t dataSize = " << str.size() << ";" << endl;
}

ElementPtr TestUtils::getElementWithNote(OsmMapPtr map, QString note)
{
  return getElementWithTag(map, "note", note);
}

ElementPtr TestUtils::getElementWithTag(OsmMapPtr map, const QString& tagKey,
                                        const QString& tagValue)
{
  TagCriterion tc(tagKey, tagValue);
  UniqueElementIdVisitor v;
  FilteredVisitor fv(tc, v);
  map->visitRo(fv);
  const set<ElementId> bag = v.getElementSet();

  if (bag.size() != 1)
  {
    throw HootException("Could not find an expected element with tag: " + tagKey + "=" + tagValue);
  }

  return map->getElement(*bag.begin());
}

TestUtils& TestUtils::getInstance()
{
  //  Local static singleton instance
  static TestUtils instance;
  return instance;
}

std::string TestUtils::readFile(QString f1)
{
  QFile fpTest(f1);
  fpTest.open(QIODevice::ReadOnly);
  return QString(fpTest.readAll()).toStdString();
}

void TestUtils::resetBasic()
{
  LOG_DEBUG("Resetting test environment...");

  // provide the most basic configuration.
  OsmMap::resetCounters();
  // make sure the UUIDs are repeatable
  UuidHelper::resetRepeatableKey();
  //  Reset the pseudo random number generator seed
  Tgs::Random::instance()->seed();
}

void TestUtils::resetEnvironment(const QStringList confs)
{
  LOG_DEBUG("Resetting test environment...");

  // provide the most basic configuration

  OsmMap::resetCounters();

  conf().clear();
  ConfigOptions::populateDefaults(conf());

  // We require that all tests use Testing.conf as a starting point and any conf values
  // specified by it may be overridden when necessary.
  conf().loadJson(ConfPath::search("Testing.conf"));

  // The primary reason for allowing custom configs to be loaded here is in certain situations to
  // prevent the ConfigOptions defaults from being loaded, as they may be too bulky when running
  // many hoot commands at once.
  LOG_VART(confs.size());
  for (int i = 0; i < confs.size(); i++)
  {
    LOG_VART(confs[i]);
    conf().loadJson(confs[i]);
  }
  //LOG_VART(conf());
  conf().set("HOOT_HOME", getenv("HOOT_HOME"));

  // Sometimes we add new projections to the MapProjector, when this happens it may pick a new
  // projection and subtly change the results.
  conf().set(ConfigOptions::getTestForceOrthographicProjectionKey(), true);

  // these factories cache the creators. Flush them so they get any config changes.
  MatchFactory::getInstance().reset();
  MergerFactory::getInstance().reset();
  TagMergerFactory::getInstance().reset();

  // make sure the UUIDs are repeatable
  UuidHelper::resetRepeatableKey();

  foreach (RegisteredReset* rr, getInstance()._resets)
  {
    rr->reset();
  }
  //  Reset the pseudo random number generator seed
  Tgs::Random::instance()->seed();

  // Reset the debug map count used to name output files
  OsmMapWriterFactory::resetDebugMapCount();
}

QString TestUtils::toQuotedString(QString str)
{
  QString result;
  str.replace("\"", "\\\"");
  QStringList l = str.split("\n");
  for (int i = 0; i < l.size(); ++i)
  {
    if (i != l.size() - 1)
    {
      result += "\"" + l[i] + "\\n\"\n";
    }
    else
    {
      result += "\"" + l[i] + "\"\n";
    }
  }
  return result;
}

void TestUtils::verifyStdMatchesOutputIgnoreDate(const QString& stdFilePath,
                                                 const QString& outFilePath)
{
  LOG_VART(stdFilePath);
  LOG_VART(outFilePath);
  const QStringList stdTokens = FileUtils::tokenizeOutputFileWithoutDates(stdFilePath);
  const QStringList outputTokens = FileUtils::tokenizeOutputFileWithoutDates(outFilePath);
  CPPUNIT_ASSERT_EQUAL(stdTokens.size(), outputTokens.size());
  for (int i = 0; i < stdTokens.size(); i++)
  {
    HOOT_STR_EQUALS(stdTokens.at(i), outputTokens.at(i));
  }
}

QStringList TestUtils::getConflateCmdSnapshotPreOps()
{
  QStringList conflatePreOps;
  conflatePreOps.append("hoot::BuildingOutlineRemoveOp");
  conflatePreOps.append("hoot::RemoveRoundabouts");
  conflatePreOps.append("hoot::MapCleaner");
  conflatePreOps.append("hoot::HighwayCornerSplitter");
  return conflatePreOps;
}

QStringList TestUtils::getConflateCmdSnapshotPostOps()
{
  QStringList conflatePostOps;
  conflatePostOps.append("hoot::SuperfluousNodeRemover");
  conflatePostOps.append("hoot::SmallHighwayMerger");
  conflatePostOps.append("hoot::ReplaceRoundabouts");
  conflatePostOps.append("hoot::RemoveMissingElementsVisitor");
  conflatePostOps.append("hoot::RemoveInvalidReviewRelationsVisitor");
  conflatePostOps.append("hoot::RemoveDuplicateReviewsOp");
  conflatePostOps.append("hoot::BuildingOutlineUpdateOp");
  conflatePostOps.append("hoot::WayJoinerOp");
  conflatePostOps.append("hoot::RemoveInvalidRelationVisitor");
  conflatePostOps.append("hoot::RemoveInvalidMultilineStringMembersVisitor");
  conflatePostOps.append("hoot::SuperfluousWayRemover");
  conflatePostOps.append("hoot::RemoveDuplicateWayNodesVisitor");
  conflatePostOps.append("hoot::RemoveEmptyRelationsOp");
  conflatePostOps.append("hoot::ApiTagTruncateVisitor");
  conflatePostOps.append("hoot::AddHilbertReviewSortOrderOp");
  return conflatePostOps;
}

QStringList TestUtils::getConflateCmdSnapshotCleaningOps()
{
  QStringList mapCleanerTransforms;
  mapCleanerTransforms.append("hoot::ReprojectToPlanarOp");
  mapCleanerTransforms.append("hoot::DuplicateNodeRemover");
  mapCleanerTransforms.append("hoot::OneWayRoadStandardizer");
  mapCleanerTransforms.append("hoot::DuplicateWayRemover");
  mapCleanerTransforms.append("hoot::SuperfluousWayRemover");
  mapCleanerTransforms.append("hoot::IntersectionSplitter");
  mapCleanerTransforms.append("hoot::UnlikelyIntersectionRemover");
  mapCleanerTransforms.append("hoot::DualHighwaySplitter");
  mapCleanerTransforms.append("hoot::HighwayImpliedDividedMarker");
  mapCleanerTransforms.append("hoot::DuplicateNameRemover");
  mapCleanerTransforms.append("hoot::SmallHighwayMerger");
  mapCleanerTransforms.append("hoot::RemoveEmptyAreasVisitor");
  mapCleanerTransforms.append("hoot::RemoveDuplicateRelationMembersVisitor");
  mapCleanerTransforms.append("hoot::RelationCircularRefRemover");
  mapCleanerTransforms.append("hoot::RemoveEmptyRelationsOp");
  mapCleanerTransforms.append("hoot::RemoveDuplicateAreasVisitor");
  mapCleanerTransforms.append("hoot::NoInformationElementRemover");
  return mapCleanerTransforms;
}

void TestUtils::runConflateOpReductionTest(
  const QStringList& matchCreators, const int expectedPreOpSize, const int expectedPostOpsSize,
  const int expectedCleaningOpsSize)
{
  QStringList actualOps;

  CPPUNIT_ASSERT_EQUAL(4,  TestUtils::getConflateCmdSnapshotPreOps().size());
  CPPUNIT_ASSERT_EQUAL(15,  TestUtils::getConflateCmdSnapshotPostOps().size());
  CPPUNIT_ASSERT_EQUAL(17,  TestUtils::getConflateCmdSnapshotCleaningOps().size());

  MatchFactory::getInstance().reset();
  MatchFactory::getInstance()._setMatchCreators(matchCreators);
  // This is a snapshot of the ops in order to avoid any changes made to them result in requiring
  // this test's results to change over time. Clearly, any newly added ops could be being filtered
  // incorrectly, and we can update this list periodically if that's deemed important.
  conf().set(ConfigOptions::getConflatePreOpsKey(), TestUtils::getConflateCmdSnapshotPreOps());
  conf().set(ConfigOptions::getConflatePostOpsKey(), TestUtils::getConflateCmdSnapshotPostOps());
  conf().set(
    ConfigOptions::getMapCleanerTransformsKey(), TestUtils::getConflateCmdSnapshotCleaningOps());

  SuperfluousConflateOpRemover::removeSuperfluousOps();

  actualOps = conf().getList(ConfigOptions::getConflatePreOpsKey());
  CPPUNIT_ASSERT_EQUAL(expectedPreOpSize, actualOps.size());

  actualOps = conf().getList(ConfigOptions::getConflatePostOpsKey());
  CPPUNIT_ASSERT_EQUAL(expectedPostOpsSize, actualOps.size());

  actualOps = conf().getList(ConfigOptions::getMapCleanerTransformsKey());
  CPPUNIT_ASSERT_EQUAL(expectedCleaningOpsSize, actualOps.size());
}

}
