## Prerequisites

zlib:

    sudo apt-get install libz-dev

tiffio:

    sudo apt-get install libtiff5-dev

## Compilation

    mkdir build
    cd build
    g++ ../ozf2tiff.cpp -fpermissive -ltiff -lz -o ozf2tiff