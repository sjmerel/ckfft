#!/bin/bash

set -o nounset
set -o errexit

echo "*** About to commit entire ckfft tree! ***"
echo "*** continue? (yes/no)"
read response
if [ "$response" != "yes" ]
then
   echo "exiting"
   exit 1
fi

cd `dirname $0`/..

# get versions
VERSION_MINOR_FILE=etc/version/minor.txt
VERSION_MAJOR_FILE=etc/version/major.txt
VERSION=`cat $VERSION_MAJOR_FILE`.`cat $VERSION_MINOR_FILE`

DIST_NAME=ckfft-$VERSION
DIST_DIR=dist/$DIST_NAME

# update changes.txt
cp $DIST_DIR/doc/changes.txt doc/changes.txt

# commit
svn commit -m "release version $VERSION" .

# tag it
SVN_ROOT=`svn info | grep 'Repository Root' | awk '{print $NF}'`
svn copy ${SVN_ROOT}/trunk ${SVN_ROOT}/tags/ckfft-${VERSION} -m "release version ${VERSION}"

# increment minor version number
VERSION_MINOR=`cat $VERSION_MINOR_FILE`
echo $(( $VERSION_MINOR + 1 )) > $VERSION_MINOR_FILE
svn commit -m "increment build number" $VERSION_MINOR_FILE

