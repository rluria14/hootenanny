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
 * @copyright Copyright (C) 2015, 2016, 2017, 2018, 2019, 2020, 2021 DigitalGlobe (http://www.digitalglobe.com/)
 */
#ifndef SUM_NUMERIC_TAGS_VISITOR_H
#define SUM_NUMERIC_TAGS_VISITOR_H

// hoot
#include <hoot/core/elements/ConstElementVisitor.h>
#include <hoot/core/util/Configurable.h>
#include <hoot/core/info/SingleStatistic.h>
#include <hoot/core/util/StringUtils.h>

namespace hoot
{

/**
 * Sums numeric tag values with a specified key
 *
 * In the future, we may want to have this support substrings as well.
 */
class SumNumericTagsVisitor : public ConstElementVisitor, public SingleStatistic,
  public Configurable
{
public:

  static QString className() { return "hoot::SumNumericTagsVisitor"; }

  static int logWarnCount;

  SumNumericTagsVisitor();
  explicit SumNumericTagsVisitor(const QStringList keys);
  virtual ~SumNumericTagsVisitor() = default;

  /**
   * Given a set of tag keys and for all features having those tags, sums the numerical values of
   * the tags.  If the tag value cannot be converted to a number, a warning is logged and the tag
   * is skipped.
   *
   * @param e element to check for tag on
   */
  virtual void visit(const ConstElementPtr& e);

  virtual double getStat() const { return _sum; }

  virtual QString getDescription() const { return "Sums numeric tag values with specified keys"; }

  virtual void setConfiguration(const Settings& conf);

  virtual QString getName() const { return className(); }

  virtual QString getClassName() const override { return className(); }

  virtual QString getInitStatusMessage() const { return "Summing values of numeric tags..."; }

  virtual QString getCompletedStatusMessage() const
  {
    return
      "Summed " + StringUtils::formatLargeNumber(_numAffected) + " numeric tags on " +
      StringUtils::formatLargeNumber(_numProcessed) + " features.";
  }

private:

  QStringList _keys;
  double _sum;
};

}

#endif // SUM_NUMERIC_TAGS_VISITOR_H
