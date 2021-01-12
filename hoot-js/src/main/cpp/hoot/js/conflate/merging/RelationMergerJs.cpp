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
 * @copyright Copyright (C) 2021 DigitalGlobe (http://www.digitalglobe.com/)
 */
#include "RelationMergerJs.h"

// hoot
#include <hoot/core/util/Log.h>
#include <hoot/core/conflate/merging/RelationMerger.h>
#include <hoot/core/elements/ElementId.h>
#include <hoot/core/elements/OsmMap.h>
#include <hoot/core/util/Factory.h>

#include <hoot/js/JsRegistrar.h>
#include <hoot/js/elements/ElementJs.h>
#include <hoot/js/io/DataConvertJs.h>
#include <hoot/js/elements/OsmMapJs.h>
#include <hoot/js/util/HootExceptionJs.h>
#include <hoot/js/elements/ElementIdJs.h>

using namespace v8;

namespace hoot
{

HOOT_JS_REGISTER(RelationMergerJs)

void RelationMergerJs::Init(Handle<Object> exports)
{
  Isolate* current = exports->GetIsolate();
  HandleScope scope(current);
  Handle<Object> thisObj = Object::New(current);
  exports->Set(String::NewFromUtf8(current, "RelationMergerJs"), thisObj);

  thisObj->Set(String::NewFromUtf8(current, "mergeRelations"),
               FunctionTemplate::New(current, mergeRelations)->GetFunction());
}

void RelationMergerJs::mergeRelations(const FunctionCallbackInfo<Value>& args)
{
  Isolate* current = args.GetIsolate();
  HandleScope scope(current);

  OsmMapPtr map = toCpp<OsmMapPtr>(args[0]);
  ElementIdJs* elementId1Js = ObjectWrap::Unwrap<ElementIdJs>(args[1]->ToObject());
  ElementIdJs* elementId2Js = ObjectWrap::Unwrap<ElementIdJs>(args[2]->ToObject());

  RelationMerger merger;
  merger.setOsmMap(map.get());
  try
  {
    merger.merge(elementId1Js->getElementId(), elementId2Js->getElementId());

    args.GetReturnValue().SetUndefined();
  }
  catch (const HootException& err)
  {
    LOG_VARE(err.getWhat());
    args.GetReturnValue().Set(current->ThrowException(HootExceptionJs::create(err)));
  }
}

}
