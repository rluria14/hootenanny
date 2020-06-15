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
 * @copyright Copyright (C) 2015, 2016, 2017, 2018, 2019, 2020 DigitalGlobe (http://www.digitalglobe.com/)
 */
#ifndef EUCLIDEANDISTANCEEXTRACTOR_H
#define EUCLIDEANDISTANCEEXTRACTOR_H

// Hoot
#include <hoot/core/algorithms/extractors/AbstractDistanceExtractor.h>
#include <hoot/core/util/Configurable.h>

namespace hoot
{

class EuclideanDistanceExtractor : public AbstractDistanceExtractor, public Configurable
{
public:

  EuclideanDistanceExtractor();
  virtual ~EuclideanDistanceExtractor() = default;

  static std::string className() { return "hoot::EuclideanDistanceExtractor"; }

  virtual double distance(const OsmMap& map, const std::shared_ptr<const Element>& target,
    const std::shared_ptr<const Element>& candidate) const override;

  virtual std::string getClassName() const { return EuclideanDistanceExtractor::className(); }

  virtual QString getDescription() const
  { return "Calculates the Euclidean distance between two features"; }

  /**
   * @see Configurable
   */
  virtual void setConfiguration(const Settings& conf);

private:

  bool _requireAreaForPolygonConversion;
};

}

#endif // EUCLIDEANDISTANCEEXTRACTOR_H
