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
 * @copyright Copyright (C) 2019, 2020, 2021 DigitalGlobe (http://www.digitalglobe.com/)
 */
#ifndef IMMEDIATELY_CONNECTED_OUT_OF_BOUNDS_WAY_TAGGER_H
#define IMMEDIATELY_CONNECTED_OUT_OF_BOUNDS_WAY_TAGGER_H

// GEOS
#include <geos/geom/Envelope.h>

// Hoot
#include <hoot/core/elements/OsmMap.h>
#include <hoot/core/ops/OsmMapOperation.h>
#include <hoot/core/util/Boundable.h>
#include <hoot/core/criterion/InBoundsCriterion.h>
#include <hoot/core/util/StringUtils.h>

namespace hoot
{

/**
 * This will custom tag any way that falls completely outside of a given bounds but is directly
 * connected to some way that cross over the bounds. This is useful in generating replacement
 * changesets.
 */
class ImmediatelyConnectedOutOfBoundsWayTagger : public OsmMapOperation, public Boundable
{
public:

  static QString className() { return "hoot::ImmediatelyConnectedOutOfBoundsWayTagger"; }

  ImmediatelyConnectedOutOfBoundsWayTagger();
  ImmediatelyConnectedOutOfBoundsWayTagger(const bool strictBounds);
  virtual ~ImmediatelyConnectedOutOfBoundsWayTagger() = default;

  /**
   * @see OsmMapOperation
   */
  virtual void apply(OsmMapPtr& map);

  /**
   * @see Boundable
   */
  virtual void setBounds(std::shared_ptr<geos::geom::Geometry> bounds) override
  { _boundsChecker.setBounds(bounds); }

  // Why must this be explicitly called once the
  // setBounds(std::shared_ptr<geos::geom::Geometry> bounds) version is overridden to avoid compiler
  // errors?
  virtual void setBounds(const geos::geom::Envelope& bounds) override
  { Boundable::setBounds(bounds); }

  /**
   * @see OperationStatus
   */
  virtual QString getInitStatusMessage() const
  { return "Adding tags to immediately connected out of bounds ways..."; }

  /**
   * @see OperationStatus
   */
  virtual QString getCompletedStatusMessage() const
  {
    return
      "Added " + StringUtils::formatLargeNumber(_numAffected) + " tags out of " +
      StringUtils::formatLargeNumber(_numProcessed) + " total ways.";
  }

  /**
   * @see ApiEntityInfo
   */
  virtual QString getDescription() const
  {
    return
      "Tags ways outside of a bounds but immediately connected to ways that cross the bounds";
  }

  long getNumTagged() const { return _numAffected; }

  virtual QString getName() const { return className(); }

  virtual QString getClassName() const override { return className(); }

private:

  // passed in directly to InBoundsCriterion
  bool _strictBounds;
  InBoundsCriterion _boundsChecker;
};

}

#endif // IMMEDIATELY_CONNECTED_OUT_OF_BOUNDS_WAY_TAGGER_H
