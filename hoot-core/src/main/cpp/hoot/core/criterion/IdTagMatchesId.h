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
#ifndef ID_TAG_MATCHES_ID_H
#define ID_TAG_MATCHES_ID_H

// hoot
#include <hoot/core/criterion/ElementCriterion.h>
#include <hoot/core/schema/MetadataTags.h>

namespace hoot
{

class IdTagMatchesId : public ElementCriterion
{
public:

  static QString className() { return "hoot::IdTagMatchesId"; }

  IdTagMatchesId() = default;
  virtual ~IdTagMatchesId() = default;

  virtual bool isSatisfied(const ConstElementPtr& e) const override;

  virtual QString getDescription() const
  { return "Determines whether an element's " + MetadataTags::HootId() + " tag matches its ID"; }

  virtual QString getName() const override { return className(); }

  virtual QString getClassName() const override { return className(); }

  virtual ElementCriterionPtr clone() { return ElementCriterionPtr(new IdTagMatchesId()); }
};

}

#endif // ID_TAG_MATCHES_ID_H
