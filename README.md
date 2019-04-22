base256 is like a hexdump but with 256 unique unicode glyphs instead of only 16
unique digits.

Various different sets of glyphs (code page 437, code page 1252, braille), as
well as more traditional hexadecimal, decimal, etc... formats can be chosen.

The code pages are slightly modified to fill up spots that have no character, or
empty characters like space, with other unique printable glyphs.

### Features

Dump binary files in terminal with various formats to view them

Render to Unicode characters or to plain ASCII formats

Reverse direction: convert from a format to binary files (not supported for all formats)

Extra formatting options to tweak formats, including colored output

Render character tables of the formats for reference

Supports reading from/to files or use piping in the terminal

### Building

clang++ -std=c++11 base256.cpp -O3 -o base256

or

g++ -std=c++11 base256.cpp -O3 -o base256

----

License included in the source file
Copyright (C) 2019 by Lode Vandevenne
