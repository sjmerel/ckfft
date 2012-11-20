#!/bin/bash

set -o nounset
set -o errexit

cd `dirname $0`/..

# make sure we don't need to update or commit
svn --show-updates status | grep -v '^?'
echo
echo "*** SVN status shown above; you may want to update or commit before continuing! ***"
echo "*** continue? (yes/no)"
read response
if [ "$response" != "yes" ]
then
   echo "exiting"
   exit 1
fi
echo

rm -rf dist
mkdir dist

# get versions
VERSION_MINOR_FILE=etc/version/minor.txt
VERSION_MAJOR_FILE=etc/version/major.txt
VERSION=`cat $VERSION_MAJOR_FILE`.`cat $VERSION_MINOR_FILE`

DIST_NAME=ckfft-$VERSION
DIST_DIR=dist/$DIST_NAME
svn export . $DIST_DIR

# update changes.txt
echo "$VERSION:" > $DIST_DIR/doc/changes.txt
cat doc/changes.txt >> $DIST_DIR/doc/changes.txt

DIST_DEV_NAME=ckfft-dev-$VERSION
DIST_DEV_DIR=dist/$DIST_DEV_NAME
cp -r $DIST_DIR $DIST_DEV_DIR

# remove dev-only stuff 
rm -rf $DIST_DIR/etc
rm -rf $DIST_DIR/ext
rm -rf $DIST_DIR/src/test
rm -rf $DIST_DIR/src/ckfft.xcworkspace

# remove stuff from dev 
# TODO temporarily removing these until we're using them, since they are big
rm -rf $DIST_DEV_DIR/ext/libav
rm -rf $DIST_DEV_DIR/ext/fftw-3.3.2

pushd dist
zip -r $DIST_NAME.zip $DIST_NAME
zip -r $DIST_DEV_NAME.zip $DIST_DEV_NAME
popd

