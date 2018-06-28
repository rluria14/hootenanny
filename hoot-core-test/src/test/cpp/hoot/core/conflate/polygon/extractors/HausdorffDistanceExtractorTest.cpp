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
 * @copyright Copyright (C) 2013, 2014, 2016, 2017, 2018 DigitalGlobe (http://www.digitalglobe.com/)
 */

// Hoot
#include <hoot/core/OsmMap.h>
#include <hoot/core/TestUtils.h>
#include <hoot/core/conflate/polygon/extractors/HausdorffDistanceExtractor.h>
#include <hoot/core/elements/Way.h>
#include <hoot/core/io/OsmXmlReader.h>
#include <hoot/core/io/OsmXmlWriter.h>
#include <hoot/core/util/MapProjector.h>

// CPP Unit
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>

// Tgs
#include <tgs/StreamUtils.h>

namespace hoot
{

class HausdorffDistanceExtractorTest : public HootTestFixture
{
  CPPUNIT_TEST_SUITE(HausdorffDistanceExtractorTest);
  CPPUNIT_TEST(runRoadsTest);
  CPPUNIT_TEST_SUITE_END();

public:

  OsmMapPtr _map;

  NodePtr createNode(double x, double y)
  {
    NodePtr n(new Node(Status::Unknown1, _map->createNextNodeId(), x, y, 10.0));
    _map->addNode(n);
    return n;
  }

  void runRoadsTest()
  {
    //test highway (linestring)
    OsmMapPtr map(new OsmMap());
    _map = map;

    WayPtr w1(new Way(Status::Unknown1, map->createNextWayId(), 13.0));
    w1->setTag("highway", "track");
    w1->setTag("name", "w1");
    w1->addNode(createNode(0, 0)->getId());
    w1->addNode(createNode(10, 0)->getId());
    _map->addWay(w1);

    WayPtr w2(new Way(Status::Unknown1, map->createNextWayId(), 13.0));
    w2->setTag("highway", "road");
    w2->setTag("name", "w2");
    w2->addNode(createNode(-1, 1)->getId());
    w2->addNode(createNode(9, 0)->getId());
    _map->addWay(w2);

    HausdorffDistanceExtractor uut;
    const OsmMap* constMap = const_cast<const OsmMap*>(_map.get());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(
      sqrt(2.0),
      uut.distance(*constMap, boost::const_pointer_cast<const Way>(w1),
        boost::const_pointer_cast<const Way>(w2)),
      0.000001);
  }
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(HausdorffDistanceExtractorTest, "quick");

}
