
You can convert images to compatible jpg's by using ImageMagick's convert
utility by specifying the output type as TrueColor. ImageMagick downloads
are available from https://imagemagick.org/ for Linux, OSX, Windows and
other operating systems.

The wi-alien.svg icon is from https://github.com/erikflowers/weather-icons
licensed under SIL OFL 1.1

```
convert wi-alien.svg -type TrueColor alien.jpg
```