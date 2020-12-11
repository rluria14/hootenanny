# -------------------------------------------------
# Main project for hoot production code
# -------------------------------------------------

QT += \
    sql \
    testlib \
    xml \
    network \

TARGET = HootCore
TEMPLATE = lib

DEPENDPATH += \
    ../tbs/src/main/cpp/ \
    ../tgs/src/main/cpp/ \
    ../hoot-js/src/main/cpp/ \
    src/main/cpp/ \

INCLUDEPATH += \
  $${DEPENDPATH} \
  ../local/include/ \

CONFIG += rtti \
    debug \
    precompile_header

OTHER_FILES = \
    $$files(../translations/*.js, true) \
    $$files(../translations/*.py, true) \
    $$files(../translations-local/*.js, true) \
    $$files(../translations-local/*.py, true) \
    $$files(../rules/*.js, true) \
    $$files(../conf/schema/*.json, true) \
    $$files(../docs/*.asciidoc, true) \
    $$files(../conf/core/*.conf, true) \
    ../conf/core/ConfigOptions.asciidoc \
    ../scripts/jenkins/Jenkinsfile \
    $$files(../scripts/copyright/*, true) \
    ../sonar-project.properties \
    $$files(../Vagrant*)

include(../Configure.pri)

LIBS += -L../lib/ -lTgs -ltbs
LIBS -= -lhdfs

UI_DIR = tmp/ui
MOC_DIR = tmp/moc
release:OBJECTS_DIR = tmp/release
else:OBJECTS_DIR = tmp/debug
DESTDIR = ../lib/

PROTOPATH = src/main/cpp/hoot/core/proto/
PROTOS = $${PROTOPATH}/OsmFormat.proto $${PROTOPATH}/FileFormat.proto
PROTO_TMP=src/main/cpp/hoot/core/proto/

include(protobuf.pri)

PRECOMPILED_HEADER = src/main/cpp/hoot/core/HootCoreStable.h

SOURCES += $$files(src/*.cpp, true)
HEADERS += $$files(src/*.h, true)
