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
 * @copyright Copyright (C) 2020 DigitalGlobe (http://www.digitalglobe.com/)
 */
#ifndef CHANGESET_REPLACEMENT_CREATOR_5_H
#define CHANGESET_REPLACEMENT_CREATOR_5_H

// Hoot
#include <hoot/core/algorithms/changeset/ChangesetReplacementCreator1.h>

namespace hoot
{

/**
 * Brings in all relations with mixed children and combines and de-duplicates maps just before ID
 * synchronization.
 */
class ChangesetReplacementCreator5 : public ChangesetReplacementCreator1
{

public:

  static std::string className() { return "hoot::ChangesetReplacementCreator5"; }

  ChangesetReplacementCreator5();

 virtual void create(
    const QString& input1, const QString& input2, const geos::geom::Envelope& bounds,
    const QString& output);

  virtual QString toString() const override
    { return QString::fromStdString(className()).remove("hoot::"); }

protected:

  virtual void _setGlobalOpts();

  virtual QMap<GeometryTypeCriterion::GeometryType, ElementCriterionPtr>
    _getDefaultGeometryFilters() const;

private:

  void _intraDedupeMap(OsmMapPtr& map);
  void _combineProcessedMaps(QList<OsmMapPtr>& maps, const bool deduplicate);
};

}

#endif // CHANGESET_REPLACEMENT_CREATOR_5_H
