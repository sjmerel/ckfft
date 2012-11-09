# Wrapper script for building, so assets are also copied.
# (There doesn't seem to be any supported way to put custom rules into Android.mk.)

all:
	cp ../input.txt assets/
	cp ../results_android_*.xml assets/
	@ndk-build 
