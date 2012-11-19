#!/bin/bash

set -o nounset
set -o errexit

if ! [[ "$OSTYPE" =~ "cygwin" ]]
then
   echo "This script is for windows (cygwin) only!"
   exit 1
fi

cd `dirname $0`  
CKFFT_ROOT_DIR=..


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


rm -f build.log


# build ckfft VS2010
VisualStudioVersion=10.0
export VisualStudioVersion
for platform in Win32 x64
do
   for config in DebugStatic DebugDynamic ReleaseStatic ReleaseDynamic
   do
      # build
      echo building ckfft vs2010 $platform $config
      msbuild.exe $CKFFT_ROOT_DIR/src/ckfft/ckfft_vs2010.sln /t:ckfft /p:Configuration=$config /p:Platform=$platform >> build.log || (echo "error building ckfft vs2010 $config!"; exit 1)

      # TODO build & run test
   done
done


# build ckfft VS2012
VisualStudioVersion=11.0
export VisualStudioVersion
for platform in Win32 x64
do
   for config in DebugStatic DebugDynamic DebugMetro ReleaseStatic ReleaseDynamic ReleaseMetro
   do
      # build
      echo building ckfft vs2012 $platform $config
      msbuild.exe $CKFFT_ROOT_DIR/src/ckfft/ckfft_vs2012.sln /t:ckfft /p:Configuration=$config /p:Platform=$platform >> build.log || (echo "error building ckfft vs2012 $config!"; exit 1)

      # TODO build & run test
   done
done


