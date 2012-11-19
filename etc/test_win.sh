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


rm -f build.log test.log

cd $CKFFT_ROOT_DIR/src/test/win

# VS2010
VisualStudioVersion=10.0
export VisualStudioVersion
for platform in Win32 x64
do
   for config in Debug Release
   do
      # build
      echo building for vs2010 $platform $config
      msbuild.exe test_vs2010.sln /t:test /p:Configuration=$config /p:Platform=$platform >> ../../../etc/build.log || (echo "error building test vs2010 $config!"; exit 1)

      # run
      echo testing for vs2010 $platform $config
      echo -e "testing for vs2010 $platform $config \r" >> ../../../etc/test.log
      ./vs${VisualStudioVersion}/$platform/$config/test.exe >> ../../../etc/test.log || (echo "error running test vs2010 $config!"; exit 1)
   done
done


# VS2012
VisualStudioVersion=11.0
export VisualStudioVersion
for platform in Win32 x64
do
   for config in Debug Release  
   do
      # build
      echo building for vs2012 $platform $config
      msbuild.exe test_vs2012.sln /t:test /p:Configuration=$config /p:Platform=$platform >> ../../../etc/build.log || (echo "error building test vs2012 $config!"; exit 1)

      # run
      echo testing for vs2012 $platform $config
      echo -e "testing for vs2012 $platform $config \r" >> ../../../etc/test.log
      ./vs${VisualStudioVersion}/$platform/$config/test.exe >> ../../../etc/test.log || (echo "error running test vs2012 $config!"; exit 1)
   done
done


