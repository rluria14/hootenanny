#!/bin/bash

export TOMCAT_PORT=`cat $TOMCAT6_HOME/conf/server.xml | grep -e "<Connector.*HTTP\/1.1\"\w*" | grep -v SSL | sed -e "s/\s*<Connector.*port=\"\([0-9]*\)\" protocol=.*/\1/g"`
export NODE_MAPNIK_SERVER_PORT=8000

SERVICES_LOCAL_CONF=$HOOT_HOME/hoot-services/src/main/resources/conf/local.conf
if [ ! -f $SERVICES_LOCAL_CONF ]; then
  echo "# This file is automatically generated by Config.sh" >> $SERVICES_LOCAL_CONF
  echo ErrorLogPath=$TOMCAT6_HOME/logs/catalina.out >> $SERVICES_LOCAL_CONF
  echo coreJobServerUrl=http://localhost:$TOMCAT_PORT >> $SERVICES_LOCAL_CONF
  #If any concurrency issues are found, this value can be raised (in ms)
  echo testJobStatusPollerTimeout=250 >> $SERVICES_LOCAL_CONF
fi
