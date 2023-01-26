#!/bin/sh

set -ex

# This build is unusual in that we run `mvn package` in Docker containers under both architectures, so we generate
# the native libraries for both. Then we package both of those into a single multi-arch JAR.

cd $WORKSPACE
# Deliberately don't clean before building, so we keep the previous run's native libraries
mvn package

VERSION="0.4.21-hubspot-SNAPSHOT"
ARTIFACT_NAME="hadoop-lzo"

if [ "$GIT_BRANCH" = "master" ]
then
  PACKAGE_NAME=$ARTIFACT_NAME
else
  echo "Not on master."
  PACKAGE_NAME="$ARTIFACT_NAME-$GIT_BRANCH"
fi

fpm \
  --name "${PACKAGE_NAME}" \
  --version ${VERSION} \
  --iteration "hs${BUILD_NUMBER}" \
  --architecture all \
  --force \
  -d lzo \
  -s "dir" \
  -t "rpm" \
  --package "${RPMS_OUTPUT_DIR}" \
  "${WORKSPACE}/target/hadoop-lzo-${VERSION}.jar=/usr/lib/hadoop/lib/hadoop-lzo-${VERSION}.jar"
