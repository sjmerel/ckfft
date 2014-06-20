#!/bin/bash

set -o nounset
set -o errexit

function onExit()
{
   if [ $? != 0 ] 
   then 
      echo "EXITED WITH ERRORS!" 
   fi
}
trap onExit  EXIT


if ! [[ "$OSTYPE" =~ "darwin" ]]
then
   echo "This script is for windows (cygwin) only!"
   exit 1
fi

cd `dirname $0`  
CKFFT_ROOT_DIR=..

rm -f build.log

# make sure we don't need to update or commit
svn --show-updates -q status $CKFFT_ROOT_DIR > status.txt
cat status.txt
if [ `cat status.txt | wc -l && rm status.txt` -gt 0 ]
then
   echo
   echo "*** SVN status shown above; you may want to update or commit before continuing! ***"
   echo "*** continue? (yes/no)"
   read response
   if [ "$response" != "yes" ]
   then
      echo "exiting"
      exit 0
   fi
   echo
fi

IOS_ARM_ARCHS="armv7 armv7s arm64"
IOS_INTEL_ARCHS="i386 x86_64"
OSX_INTEL_ARCHS="x86_64"

# build for mac/ios
for config in Debug Release
do
   echo building ckfft iphone $config
   xcrun -sdk iphoneos xcodebuild -workspace $CKFFT_ROOT_DIR/src/ckfft.xcworkspace -scheme test_ios -sdk iphoneos -configuration $config \
      ONLY_ACTIVE_ARCH=NO VALID_ARCHS="$IOS_ARM_ARCHS" ARCHS="$IOS_ARM_ARCHS" build >> build.log 2>&1 || exit 1;

   echo building ckfft iphonesimulator $config
   xcrun -sdk iphoneos xcodebuild -workspace $CKFFT_ROOT_DIR/src/ckfft.xcworkspace -scheme test_ios -sdk iphonesimulator -configuration $config \
      ONLY_ACTIVE_ARCH=NO VALID_ARCHS="$IOS_INTEL_ARCHS" ARCHS="$IOS_INTEL_ARCHS" build >> build.log 2>&1 || exit 1;

   echo building ckfft macos $config 
   xcrun -sdk macosx xcodebuild -workspace $CKFFT_ROOT_DIR/src/ckfft.xcworkspace -scheme test_macos -configuration $config \
      ONLY_ACTIVE_ARCH=NO VALID_ARCHS="$OSX_INTEL_ARCHS" ARCHS="$OSX_INTEL_ARCHS" build >> build.log 2>&1 || exit 1;
done

# build for android
echo building ckfft android 
make -C $CKFFT_ROOT_DIR/src/test/android  >> build.log 2>&1
