# OZF2TIFF converter

Basic utility to convert ozf2 file to tiff.

# Copyright

The source was taken from:

http://www.gps-forum.ru/forum/viewtopic.php?t=31525

and slightly modified to fix errors introduced by the forum engine ("[" and "]" were replaced by "<" and ">" in couple of places).

All credit goes to original authors and a forum user who shared the code.

See the original copyright note in the source file.

# Installation

The following steps were tested on Linux Mint 21.2.

## Prerequisites

zlib:

    sudo apt-get install libz-dev

tiffio:

    sudo apt-get install libtiff5-dev

## Compilation

    mkdir build
    cd build
    g++ ../ozf2tiff.cpp -fpermissive -ltiff -lz -o ozf2tiff

# How to use

    ./ozf2tiff <input.osf2> <output.tiff>