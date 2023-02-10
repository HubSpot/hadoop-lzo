#!/bin/sh

set -ex

# This build is unusual in that we run `mvn package` in Docker containers under both architectures, so we generate
# the native libraries for both. Then we package both of those into a single multi-arch JAR.

cd $WORKSPACE
# Deliberately don't clean before building, so we keep the previous run's native libraries
mvn package -DskipTests -Pbuild-native-linux
mvn package -DskipTests -Pbuild-native-mac

RPM_VERSION="0.4.21-hubspot-SNAPSHOT"
ARTIFACT_NAME="hadoop-lzo"

if [ "$GIT_BRANCH" = "master" ]
then
  PACKAGE_NAME=$ARTIFACT_NAME
  MAVEN_VERSION="0.4.21-hubspot-SNAPSHOT"
else
  echo "Not on master."
  PACKAGE_NAME="$ARTIFACT_NAME-$GIT_BRANCH"
  MAVEN_VERSION="0.4.21-hubspot-${GIT_BRANCH}-SNAPSHOT"
fi

fpm \
  --name "${PACKAGE_NAME}" \
  --version ${RPM_VERSION} \
  --iteration "hs${BUILD_NUMBER}" \
  --architecture all \
  --force \
  -d lzo \
  -s "dir" \
  -t "rpm" \
  --package "${RPMS_OUTPUT_DIR}" \
  "${WORKSPACE}/target/hadoop-lzo-${MAVEN_VERSION}.jar=/usr/lib/hadoop/lib/hadoop-lzo-${MAVEN_VERSION}.jar"
