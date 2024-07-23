#!/bin/bash

mkdir -p supershuckie.iconset
pushd supershuckie.iconset

magick ../supershuckie.png -resize 16x16 icon_16x16.png
magick ../supershuckie.png -resize 32x32 icon_32x32.png
magick ../supershuckie.png -resize 64x64 icon_64x64.png
magick ../supershuckie.png -resize 128x128 icon_128x128.png
magick ../supershuckie.png -resize 256x256 icon_256x256.png

magick ../supershuckie.png -resize 32x32 icon_16x16@2x.png
magick ../supershuckie.png -resize 64x64 icon_32x32@2x.png
magick ../supershuckie.png -resize 128x128 icon_64x64@2x.png
magick ../supershuckie.png -resize 256x256 icon_128x128@2x.png

popd

iconutil -c icns supershuckie.iconset