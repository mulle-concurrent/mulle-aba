#! /bin/sh

mulle-bootstrap build "$@"

#
# fucking Xcode stupidity if we build with -scheme
# stuff gets dumped into  build/Products/Release
# w/o -scheme
# stuff gets dumped into  build/Release
#
xcodebuild -configuration Debug -scheme Libraries -project mulle-aba.xcodeproj
xcodebuild -configuration Release -scheme Libraries -project mulle-aba.xcodeproj
