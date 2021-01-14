#!/bin/bash
set -e

# This tests the changeset output of Reference Conflation within a bounds where the inputs contained 
# fully hydrated relations (API DB queries used automatically hydrate the relations) in the 
# following way:
#
# 2) Rivers and their parent relations only are conflated with no input filtering:
#   a) Only rivers that exist within or cross over the bounds should be conflated and added to the 
#      output changeset. 
#   b) Only reference relations who have at least one river that exists within or crosses over the 
#      bounds should be merged with secondary relations and added to the output changeset. Members 
#      in such reference and secondary relations completely outside of the bounds should remain in 
#      their parent relations after changeset application.
#   c) Same as scenario 1a
#
# See related notes in ServiceDiffConflateBounds1Test and other ServiceDiffConflateBounds*Test files 
# for other scenarios.

TEST_NAME=ServiceDiffConflateBounds2Test
GOLD_DIR=test-files/cmd/glacial/serial/ServiceDiffConflateBoundsTest
OUTPUT_DIR=test-output/cmd/glacial/serial/$TEST_NAME
rm -rf $OUTPUT_DIR
mkdir -p $OUTPUT_DIR

source conf/database/DatabaseConfig.sh
export OSM_API_DB_URL="osmapidb://$DB_USER:$DB_PASSWORD@$DB_HOST:$DB_PORT/$DB_NAME_OSMAPI"
export PSQL_DB_AUTH="-h $DB_HOST -p $DB_PORT -U $DB_USER"
export PGPASSWORD=$DB_PASSWORD_OSMAPI
HOOT_DB_URL="hootapidb://$DB_USER:$DB_PASSWORD@$DB_HOST:$DB_PORT/$DB_NAME"
HOOT_EMAIL="$TEST_NAME@hoottestcpp.org"
LOG_LEVEL="--warn"
LOG_FILTER=""

GENERAL_OPTS="-C UnifyingAlgorithm.conf -C ReferenceConflation.conf -C Testing.conf -D uuid.helper.repeatable=true -D writer.include.debug.tags=true -D reader.add.source.datetime=false -D writer.include.circular.error.tags=false"
DB_OPTS="-D api.db.email=$HOOT_EMAIL -D hootapi.db.writer.create.user=true -D hootapi.db.writer.overwrite.map=true -D changeset.user.id=1 -D changeset.max.size=999999" 
# The match/merger creators added here are the only difference between this scenario and scenario 1.
CONFLATE_OPTS="-D match.creators=hoot::ScriptMatchCreator,River.js;hoot::ScriptMatchCreator,Relation.js -D merger.creators=hoot::ScriptMergerCreator;hoot::ScriptMergerCreator -D bounds=-117.729492166,40.9881915574,-117.718505838,40.996484138672 -D bounds.output.file=$OUTPUT_DIR/bounds.osm -D waterway.maximal.subline.auto.optimize=true"
CHANGESET_DERIVE_OPTS="-D changeset.user.id=1 -D changeset.allow.deleting.reference.features=false -D bounds=-117.729492166,40.9881915574,-117.718505838,40.996484138672"

DEBUG=false
if [ "$DEBUG" == "true" ]; then
  GENERAL_OPTS=$GENERAL_OPTS" -D debug.maps.write=true"
  LOG_LEVEL="--trace"
  LOG_FILTER="-D log.class.filter=Relation.js;RelationMemberUtils;RelationMerger;OsmApiDbSqlChangesetFileWriter;ConflateUtils"
fi

scripts/database/CleanAndInitializeOsmApiDb.sh

# write ref to osmapidb
hoot convert $LOG_LEVEL $LOG_FILTER $GENERAL_OPTS $DB_OPTS -D debug.maps.filename=$OUTPUT_DIR/ref-load-debug.osm -D reader.use.data.source.ids=true  $GOLD_DIR/Input1.osm $OSM_API_DB_URL

# write sec to hootapidb
SEC_INPUT=$HOOT_DB_URL/$TEST_NAME-sec-input
hoot convert $LOG_LEVEL $LOG_FILTER $GENERAL_OPTS $DB_OPTS -D debug.maps.filename=$OUTPUT_DIR/sec-load-debug.osm -D reader.use.data.source.ids=true  $GOLD_DIR/Input2.osm $SEC_INPUT

# run ref conflate to osm
SEC_CONFLATED=$HOOT_DB_URL/$TEST_NAME-sec-conflated
hoot conflate $LOG_LEVEL $LOG_FILTER $GENERAL_OPTS $DB_OPTS $CONFLATE_OPTS -D debug.maps.filename=$OUTPUT_DIR/conflate-debug.osm -D conflate.use.data.source.ids.1=true -D conflate.use.data.source.ids.2=true $OSM_API_DB_URL $SEC_INPUT $SEC_CONFLATED --write-bounds

# generate a changeset between the original ref data and the diff calculated in the previous step
hoot changeset-derive $LOG_LEVEL $LOG_FILTER $GENERAL_OPTS $DB_OPTS $CHANGESET_DERIVE_OPTS -D debug.maps.filename=$OUTPUT_DIR/changeset-derive-debug.osm $OSM_API_DB_URL $SEC_CONFLATED $OUTPUT_DIR/diff.osc.sql $OSM_API_DB_URL
if [ "$DEBUG" == "true" ]; then
  hoot changeset-derive $LOG_LEVEL $LOG_FILTER $GENERAL_OPTS $DB_OPTS $CHANGESET_DERIVE_OPTS -D debug.maps.filename=$OUTPUT_DIR/changeset-derive-debug.osm $OSM_API_DB_URL $SEC_CONFLATED $OUTPUT_DIR/diff.osc
fi

# apply changeset back to ref
hoot changeset-apply $LOG_LEVEL $LOG_FILTER $GENERAL_OPTS $DB_OPTS $CHANGESET_DERIVE_OPTS -D debug.maps.filename=$OUTPUT_DIR/changeset-apply-debug.osm $OUTPUT_DIR/diff.osc.sql $OSM_API_DB_URL

# read ref back out and compare to gold
hoot convert $LOG_LEVEL $LOG_FILTER $GENERAL_OPTS $DB_OPTS -D debug.maps.filename=$OUTPUT_DIR/final-out-debug.osm -D reader.use.data.source.ids=true $OSM_API_DB_URL $OUTPUT_DIR/out.osm
hoot diff $LOG_LEVEL $LOG_FILTER $GENERAL_OPTS $GOLD_DIR/out-2.osm $OUTPUT_DIR/out.osm

# cleanup
hoot db-delete --warn $GENERAL_OPTS $DB_OPTS $SEC_DB_INPUT $SEC_INPUT
hoot db-delete --warn $GENERAL_OPTS $DB_OPTS $SEC_DB_INPUT $SEC_CONFLATED
PGPASSWORD=$DB_PASSWORD psql $PSQL_DB_AUTH -d $DB_NAME -c "DELETE FROM users WHERE email='$HOOT_EMAIL';" > /dev/null
