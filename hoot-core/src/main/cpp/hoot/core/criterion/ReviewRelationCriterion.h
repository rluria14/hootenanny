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
#ifndef REVIEWRELATIONCRITERION_H
#define REVIEWRELATIONCRITERION_H

// Hoot
#include <hoot/core/criterion/ElementCriterion.h>

namespace hoot
{

/**
 * Matches conflation review relations
 */
class ReviewRelationCriterion : public ElementCriterion
{
public:

  static QString className() { return "hoot::ReviewRelationCriterion"; }

  ReviewRelationCriterion() = default;
  virtual ~ReviewRelationCriterion() = default;

  virtual bool isSatisfied(const ConstElementPtr& e) const override;

  virtual ElementCriterionPtr clone()
  { return ElementCriterionPtr(new ReviewRelationCriterion()); }

  virtual QString getDescription() const
  { return "Identifies review relations created during conflation"; }

  virtual QString getName() const override { return className(); }
};

}

#endif // REVIEWRELATIONCRITERION_H
