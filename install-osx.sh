#! /bin/sh
#
# when you don't use brew, install with this
xcodebuild -target mulle_aba "DEPLOYMENT_LOCATION=YES" "DSTROOT=/" SYMROOT="build"

