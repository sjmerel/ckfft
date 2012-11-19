#!/bin/bash

set -o nounset
set -o errexit

if ! [[ "$OSTYPE" =~ "darwin" ]]
then
   echo "This script is for windows (cygwin) only!"
   exit 1
fi

cd `dirname $0`  
CKFFT_ROOT_DIR=..

rm -f build.log

# make sure we don't need to update or commit
svn --show-updates status $CKFFT_ROOT_DIR | grep -v '^?'
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


# build for mac/ios
for config in Debug Release
do
   echo building ckfft iphone $config
   xcodebuild -workspace $CKFFT_ROOT_DIR/src/ckfft.xcworkspace -scheme test_ios -sdk iphoneos6.0 -configuration $config build >> build.log || exit 1;
   echo building ckfft iphonesimulator $config
   xcodebuild -workspace $CKFFT_ROOT_DIR/src/ckfft.xcworkspace -scheme test_ios -sdk iphonesimulator6.0 -configuration $config build >> build.log || exit 1;
   echo building ckfft macos $config 
   xcodebuild -workspace $CKFFT_ROOT_DIR/src/ckfft.xcworkspace -scheme test_macos -configuration $config build >> build.log || exit 1;
done

# build for android
echo building ckfft android 
make -C $CKFFT_ROOT_DIR/src/test/android  >> build.log 2>&1
