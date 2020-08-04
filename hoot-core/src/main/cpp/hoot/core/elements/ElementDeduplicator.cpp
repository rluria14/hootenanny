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

#include "ElementDeduplicator.h"

// Hoot
#include <hoot/core/util/HootException.h>
#include <hoot/core/util/Log.h>
#include <hoot/core/visitors/ElementHashVisitor.h>
#include <hoot/core/elements/WayUtils.h>
#include <hoot/core/ops/RemoveElementByEid.h>

namespace hoot
{

ElementDeduplicator::ElementDeduplicator() :
_dedupeIntraMap(true),
_dedupeNodes(true),
_dedupeWays(true),
_dedupeRelations(true),
_favorMoreConnectedWays(false)
{
}

void ElementDeduplicator::dedupe(OsmMapPtr map)
{
  _validateInputs();

  LOG_STATUS("De-duping intra-map: " << map->getName() << "...");
  LOG_DEBUG(map->getName() << " size before de-duping: " << map->size());

  const long nodesBefore = map->getNodeCount();
  const long waysBefore = map->getWayCount();
  const long relationsBefore = map->getRelationCount();

  // calculate our unique hashes per element and get a list of duplicate pairs within the map
  QMap<QString, ElementId> mapHashes;
  QSet<std::pair<ElementId, ElementId>> duplicates;
  _calcElementHashes(map, mapHashes, duplicates);
  QSet<QString> mapHashesSet = mapHashes.keys().toSet();
  LOG_VARD(mapHashesSet.size());

  // convert the duplicate pairs to the IDs of the element we actually want to remove, sorted by
  // element type and then remove them
  const QMap<ElementType::Type, QSet<ElementId>> elementsToRemove = _dupesToElementIds(duplicates);
  if (_dedupeRelations)
  {
    _removeElements(elementsToRemove[ElementType::Relation], map);
  }
  if (_dedupeWays)
  {
    _removeElements(elementsToRemove[ElementType::Way], map);
  }
  if (_dedupeNodes)
  {
    _removeElements(elementsToRemove[ElementType::Node], map);
  }

  LOG_DEBUG(map->getName() << " size after de-duping: " << map->size());
  LOG_DEBUG(
    "Removed " << nodesBefore - map->getNodeCount() << " duplicate nodes from " <<
    map->getName());
  LOG_DEBUG(
    "Removed " << waysBefore - map->getWayCount() << " duplicate ways from " <<
    map->getName());
  LOG_DEBUG(
    "Removed " << relationsBefore - map->getRelationCount() << " duplicate relations from " <<
    map->getName());
}

void ElementDeduplicator::dedupe(OsmMapPtr map1, OsmMapPtr map2)
{
  _validateInputs();

  LOG_STATUS("De-duping map: " << map1->getName() << " and " << map2->getName() << "...");
  LOG_DEBUG(map1->getName() << " size before de-duping: " << map1->size());
  LOG_DEBUG(map2->getName() << " size before de-duping: " << map2->size());

  const long map1NodesBefore = map1->getNodeCount();
  const long map1WaysBefore = map1->getWayCount();
  const long map1RelationsBefore = map1->getRelationCount();
  const long map2NodesBefore = map2->getNodeCount();
  const long map2WaysBefore = map2->getWayCount();
  const long map2RelationsBefore = map2->getRelationCount();

  // calculate our unique hashes per element for each map and get a list of duplicate pairs within
  // each map

  QMap<QString, ElementId> map1Hashes;
  QSet<std::pair<ElementId, ElementId>> duplicates1;
  _calcElementHashes(map1, map1Hashes, duplicates1);
  QSet<QString> map1HashesSet = map1Hashes.keys().toSet();
  LOG_VARD(map1HashesSet.size());

  QMap<QString, ElementId> map2Hashes;
  QSet<std::pair<ElementId, ElementId>> duplicates2;
  _calcElementHashes(map2, map2Hashes, duplicates2);
  QSet<QString> map2HashesSet = map2Hashes.keys().toSet();
  LOG_VARD(map2HashesSet.size());

  QMap<ElementType::Type, QSet<ElementId>> elementsToRemove;
  QMap<ElementId, QString> elementIdsToRemoveFromMap;

  if (_dedupeIntraMap)
  {
    // remove the dupes within each map

    LOG_DEBUG("Recording " << map1->getName() << " duplicates...");
    _dupesToElementIdsCheckMap(
      duplicates1, map1, map2, elementsToRemove, elementIdsToRemoveFromMap);

    LOG_DEBUG("Recording " << map2->getName() << " duplicates...");
    _dupesToElementIdsCheckMap(
      duplicates2, map1, map2, elementsToRemove, elementIdsToRemoveFromMap);
  }

  // now, calculate the duplicates when comparing the map's between each other; we'll arbitrarily
  // remove features from the second map except where _favorMoreConnectedWays has influence if it
  // has been enabled
  _dupeHashesToElementIdsCheckMap(
    map1HashesSet.intersect(map2HashesSet), map1, map2, map1Hashes, map2Hashes, elementsToRemove,
    elementIdsToRemoveFromMap);

  if (_dedupeRelations)
  {
    _removeElements(elementsToRemove[ElementType::Relation], map2);
  }
  if (_dedupeWays)
  {
    LOG_DEBUG("Removing duplicate ways...");
    if (!_favorMoreConnectedWays)
    {
      _removeElements(elementsToRemove[ElementType::Way], map2);
    }
    else
    {
      _removeWaysCheckMap(
        elementsToRemove[ElementType::Way], map1, map2, elementIdsToRemoveFromMap);
    }
  }
  if (_dedupeNodes)
  {
    _removeElements(elementsToRemove[ElementType::Node], map2);
  }

  LOG_DEBUG(map1->getName() << " size after de-duping: " << map1->size());
  LOG_DEBUG(map2->getName() << " size after de-duping: " << map2->size());
  LOG_DEBUG(
    "Removed " << map1NodesBefore - map1->getNodeCount() << " duplicate nodes from " <<
    map1->getName());
  LOG_DEBUG(
    "Removed " << map1WaysBefore - map1->getWayCount() << " duplicate ways from " <<
    map1->getName());
  LOG_DEBUG(
    "Removed " << map1RelationsBefore - map1->getRelationCount() << " duplicate relations from " <<
    map1->getName());
  LOG_DEBUG(
    "Removed " << map2NodesBefore - map2->getNodeCount() << " duplicate nodes from " <<
    map2->getName());
  LOG_DEBUG(
    "Removed " << map2WaysBefore - map2->getWayCount() << " duplicate ways from " <<
    map2->getName());
  LOG_DEBUG(
    "Removed " << map2RelationsBefore - map2->getRelationCount() << " duplicate relations from " <<
    map2->getName());
}

void ElementDeduplicator::_validateInputs()
{
  if (!_dedupeNodes && !_dedupeWays && !_dedupeRelations)
  {
    throw IllegalArgumentException("All element types set to be skipped for de-duplication.");
  }
}

void ElementDeduplicator::_calcElementHashes(
  OsmMapPtr map, QMap<QString, ElementId>& hashes,
  QSet<std::pair<ElementId, ElementId>>& duplicates)
{
  ElementHashVisitor hashVis;
  hashVis.setWriteHashes(false);
  hashVis.setCollectHashes(true);

  LOG_DEBUG("Calculating " << map->getName() << " element hashes...");
  hashVis.setOsmMap(map.get());
  map->visitRw(hashVis);
  hashes = hashVis.getHashes();
  duplicates = hashVis.getDuplicates();
  LOG_VARD(duplicates.size());
}

void ElementDeduplicator::_removeElements(const QSet<ElementId>& elementsToRemove, OsmMapPtr map)
{
  if (elementsToRemove.size() == 0)
  {
    return;
  }

  // This works b/c we're sending in all ids of the same element type.
  const ElementId id = *elementsToRemove.begin();
  LOG_DEBUG(
    "Removing duplicate " << id.getType().toString() << "s from " << map->getName() << "...");

  ElementCriterionPtr crit;
  if (id.getType() == ElementType::Node && _nodeCrit)
  {
    crit = _nodeCrit;
  }
  else if (id.getType() == ElementType::Way && _wayCrit)
  {
    crit = _wayCrit;
  }
  if (id.getType() == ElementType::Relation && _relationCrit)
  {
    crit = _relationCrit;
  }

  for (QSet<ElementId>::const_iterator itr = elementsToRemove.begin();
       itr != elementsToRemove.end(); ++itr)
  {
    const ElementId elementId = *itr;
    ConstElementPtr element = map->getElement(elementId);
    if (element && (!crit || crit->isSatisfied(element)))
    {
      LOG_TRACE("Removing " << elementId << "...");
      RemoveElementByEid::removeElementNoCheck(map, elementId);
    }
  }
}

void ElementDeduplicator::_removeWaysCheckMap(
  const QSet<ElementId>& waysToRemove, OsmMapPtr map1, OsmMapPtr map2,
  const QMap<ElementId, QString>& elementIdsToRemoveFromMap)
{
  LOG_VARD(waysToRemove.size());

  for (QSet<ElementId>::const_iterator itr = waysToRemove.begin();
       itr != waysToRemove.end(); ++itr)
  {
    const ElementId elementId = *itr;
    assert(elementId.getType() == ElementType::Way);

    OsmMapPtr mapToRemoveFrom;
    if (elementIdsToRemoveFromMap[elementId] == "1")
    {
      mapToRemoveFrom = map1;
    }
    else
    {
      mapToRemoveFrom = map2;
    }

    ConstElementPtr element = mapToRemoveFrom->getElement(elementId);
    if (element && (!_wayCrit || _wayCrit->isSatisfied(element)))
    {
      LOG_TRACE("Removing " << elementId << " from " << mapToRemoveFrom->getName() << "...");
      RemoveElementByEid::removeElementNoCheck(mapToRemoveFrom, elementId);
    }
  }
}

QMap<ElementType::Type, QSet<ElementId>> ElementDeduplicator::_dupesToElementIds(
  const QSet<std::pair<ElementId, ElementId>>& duplicates)
{
  QMap<ElementType::Type, QSet<ElementId>> elementsToRemove;

  for (QSet<std::pair<ElementId, ElementId>>::const_iterator itr = duplicates.begin();
       itr != duplicates.end(); ++itr)
  {
    const std::pair<ElementId, ElementId> dupes = *itr;
    const ElementId dupe1 = dupes.first;
    LOG_VART(dupe1);
    const ElementId dupe2 = dupes.second;
    LOG_VART(dupe2);
    const ElementType elementType = dupe1.getType();
    LOG_VART(elementType);
    assert(elementType == dupe2.getType());

    elementsToRemove[elementType.getEnum()].insert(dupe1);
  }

  return elementsToRemove;
}

void ElementDeduplicator::_dupesToElementIdsCheckMap(
  const QSet<std::pair<ElementId, ElementId>>& duplicates, OsmMapPtr map1, OsmMapPtr map2,
  QMap<ElementType::Type, QSet<ElementId>>& elementsToRemove,
  QMap<ElementId, QString>& elementIdsToRemoveFromMap)
{
  for (QSet<std::pair<ElementId, ElementId>>::const_iterator itr = duplicates.begin();
         itr != duplicates.end(); ++itr)
  {
    const std::pair<ElementId, ElementId> dupes = *itr;
    const ElementId dupe1 = dupes.first;
    LOG_VART(dupe1);
    const ElementId dupe2 = dupes.second;
    LOG_VART(dupe2);
    const ElementType elementType = dupe1.getType();
    LOG_VART(elementType);
    assert(elementType == dupe2.getType());

    if (elementType == ElementType::Way && _favorMoreConnectedWays)
    {
      const int numConnectedTo1 = WayUtils::getNumberOfConnectedWays(dupe1.getId(), map1);
      LOG_VART(numConnectedTo1);
      const int numConnectedTo2 = WayUtils::getNumberOfConnectedWays(dupe2.getId(), map2);
      LOG_VART(numConnectedTo2);
      if (numConnectedTo1 >= numConnectedTo2)
      {
        elementIdsToRemoveFromMap[dupe1] = "2";
        elementsToRemove[elementType.getEnum()].insert(dupe2);
      }
      else
      {
        elementIdsToRemoveFromMap[dupe2] = "1";
        elementsToRemove[elementType.getEnum()].insert(dupe1);
      }
    }
    else
    {
      // We always favor the first map when the number of connected ways aren't a factor, so record
      // the second map's element as the one to be removed.
      elementsToRemove[elementType.getEnum()].insert(dupe2);
    }
  }
}

void ElementDeduplicator::_dupeHashesToElementIdsCheckMap(
  const QSet<QString>& sharedHashes, OsmMapPtr map1, OsmMapPtr map2,
  const QMap<QString, ElementId>& map1Hashes, const QMap<QString, ElementId>& map2Hashes,
  QMap<ElementType::Type, QSet<ElementId>>& elementsToRemove,
  QMap<ElementId, QString>& elementIdsToRemoveFromMap)
{
  LOG_DEBUG(
    "Calculating duplicates between " << map1->getName() << " and " << map2->getName() << "...");
  LOG_VARD(sharedHashes.size());
  for (QSet<QString>::const_iterator itr = sharedHashes.begin(); itr != sharedHashes.end(); ++itr)
  {
    const QString sharedHash = *itr;
    const ElementId toRemove1 = map1Hashes[sharedHash];
    LOG_VART(toRemove1);
    const ElementId toRemove2 = map2Hashes[sharedHash];
    LOG_VART(toRemove2);
    const ElementType elementType = toRemove1.getType();
    LOG_VART(elementType);
    assert(elementType == toRemove2.getType());

    if (elementType == ElementType::Way && _favorMoreConnectedWays)
    {
      const int numConnectedTo1 = WayUtils::getNumberOfConnectedWays(toRemove1.getId(), map1);
      LOG_VART(numConnectedTo1);
      const int numConnectedTo2 = WayUtils::getNumberOfConnectedWays(toRemove2.getId(), map2);
      LOG_VART(numConnectedTo2);
      if (numConnectedTo1 >= numConnectedTo2)
      {
        elementIdsToRemoveFromMap[toRemove1] = "2";
        elementsToRemove[elementType.getEnum()].insert(toRemove2);
      }
      else
      {
        elementIdsToRemoveFromMap[toRemove2] = "1";
        elementsToRemove[elementType.getEnum()].insert(toRemove1);
      }
    }
    else
    {
      // see related note in _dupesToElementIdsCheckMap
      elementsToRemove[elementType.getEnum()].insert(toRemove2);
    }
  }
}

}
