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
 * @copyright Copyright (C) 2015, 2017, 2018, 2019, 2020 DigitalGlobe (http://www.digitalglobe.com/)
 */
#ifndef WEIGHTEDSHAPEDISTANCEEXTRACTOR_H
#define WEIGHTEDSHAPEDISTANCEEXTRACTOR_H

#include <hoot/core/algorithms/extractors/WayFeatureExtractor.h>

namespace hoot
{

/**
 * Similar to Shape Distance as described in [1].
 * 1. Savary & Zeitouni, 2005
 */
class WeightedShapeDistanceExtractor : public WayFeatureExtractor
{
public:

  static QString className() { return "hoot::WeightedShapeDistanceExtractor"; }

  WeightedShapeDistanceExtractor(ValueAggregatorPtr wayAgg);

  WeightedShapeDistanceExtractor() = default;
  virtual ~WeightedShapeDistanceExtractor() = default;

  virtual QString getName() const { return className(); }

  virtual QString getClassName() const override { return className(); }

  virtual QString getDescription() const
  { return "Calculates shape distance between features"; }

private:

  double _extract(const OsmMap& map, const ConstWayPtr& w1, const ConstWayPtr& w2) const;
};

}

#endif // WEIGHTEDSHAPEDISTANCEEXTRACTOR_H
