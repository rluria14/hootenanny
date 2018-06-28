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
 * @copyright Copyright (C) 2014, 2016, 2017, 2018 DigitalGlobe (http://www.digitalglobe.com/)
 */

// geos
#include <geos/io/WKTReader.h>
#include <geos/geom/Point.h>

// Hoot
#include <hoot/core/OsmMap.h>
#include <hoot/core/TestUtils.h>
#include <hoot/core/criterion/BuildingCriterion.h>
#include <hoot/core/io/OsmJsonWriter.h>
#include <hoot/core/io/OsmXmlReader.h>
#include <hoot/core/io/OsmXmlWriter.h>
#include <hoot/core/ops/ReprojectToPlanarOp.h>
#include <hoot/core/util/Log.h>

// Qt
#include <QDebug>

// TGS
#include <tgs/Statistics/Random.h>
using namespace Tgs;

namespace hoot
{

class ReprojectToPlanarOpTest : public HootTestFixture
{
  CPPUNIT_TEST_SUITE(ReprojectToPlanarOpTest);
  CPPUNIT_TEST(runTest);
  CPPUNIT_TEST_SUITE_END();

public:

  virtual void setUp()
  {
    HootTestFixture::setUp();
    TestUtils::resetEnvironment();
  }

  void runTest()
  {
    QString inputPath  = "test-files/ops/ReprojectToPlanarOp/";
    QString inputFile  = "ToyWGS84.osm";
    QString outputPath = "test-output/ops/ReprojectToPlanarOp/";
    QString outputFile = "ToyPlanar.osm";


    OsmXmlReader reader;
    OsmMapPtr map(new OsmMap());
    OsmMap::resetCounters();
    reader.setDefaultStatus(Status::Unknown1);
    reader.read(inputPath + inputFile, map);

    ReprojectToPlanarOp myOp;
    myOp.apply(map);

    TestUtils::mkpath(outputPath);
    OsmXmlWriter writer;
    writer.write(map, outputPath + outputFile);
    HOOT_FILE_EQUALS(inputPath+outputFile,
                     outputPath+outputFile);
  }

};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ReprojectToPlanarOpTest, "quick");
//CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ReprojectToPlanarOpTest, "current");

}


