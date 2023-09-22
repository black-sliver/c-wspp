#!/bin/bash

# TODO: makefile and vs solution?

OPTIMZE="-Os -s"


get_platform () {
    DUMPMACHINE=$($1 -dumpmachine)
    OS=$($1 -dumpmachine | cut -d'-' -f3)
    ARCH=$($1 -dumpmachine | cut -d'-' -f1)
    if [[ "$OS" == darwin* ]]; then
        # macos
        PLATFORM="macos-$ARCH"
        DLL_EXT=".dylib"
    elif [[ "$OS" == "mingw32" ]]; then
        # windows
        if [[ "$ARCH" == "x86_64" ]] || [[ "$ARCH" == "amd64" ]]; then
            PLATFORM="win64"
        else
            PLATFORM="win32"
        fi
        DLL_EXT=".dll"
    else
        # other
        PLATFORM="$OS-$ARCH"
        DLL_EXT=".so"
    fi
}

needs_build () {
    if [ ! -f $1 ]; then
        return 0  # true
    fi
    for f in src/*; do
        if [ $f -nt $DEST ]; then
            return 0  # true
        fi
    done
    for f in include/*; do
        if [ $f -nt $DEST ]; then
            return 0  # true
        fi
    done
    return 1  # false
}


# native / local lib
get_platform g++
mkdir -p build/$PLATFORM/lib
DEST=build/$PLATFORM/lib/c-wspp$DLL_EXT

# try static linking first, then dynamic
if needs_build $DEST; then
    echo "Building lib for $PLATFORM ($DUMPMACHINE)"
    g++ -o "$DEST" \
        -shared -fpic $OPTIMZE \
        -Wno-deprecated-declarations \
        src/c-wspp.cpp \
        -Iinclude -Isubprojects/websocketpp -Isubprojects/asio/include \
        -Wl,-Bstatic \
        -lssl -lcrypto \
        -Wno-deprecated 2>/dev/null || \
    g++ -o "$DEST" \
        -shared -fpic $OPTIMIZE \
        -Wno-deprecated-declarations \
        src/c-wspp.cpp \
        -Iinclude -Isubprojects/websocketpp -Isubprojects/asio/include \
        -lssl -lcrypto \
        -Wno-deprecated
else
    echo "lib for $PLATFORM is up to date"
fi

#native / local test binary
mkdir -p build/$PLATFORM/bin
DEST=build/$PLATFORM/lib/c-wspp$DLL_EXT

if needs_build $DEST; then
    echo "Building test for $PLATFORM"
    g++ -o "$DEST" \
        -Wno-deprecated-declarations $OPTIMZE \
        src/test.c src/c-wspp.cpp \
        -Iinclude -Isubprojects/websocketpp -Isubprojects/asio/include \
        -lssl -lcrypto
else
    echo "test for $PLATFORM is up to date"
fi

if [ -x "$(which x86_64-w64-mingw32-g++)" ]; then
    # cross build win64
    get_platform x86_64-w64-mingw32-g++
    mkdir -p build/$PLATFORM/lib
    DEST=build/$PLATFORM/lib/c-wspp$DLL_EXT

    if needs_build $DEST; then
        echo "Building lib for $PLATFORM"
        x86_64-w64-mingw32-g++ -o "$DEST" \
            -shared -fpic $OPTIMZE \
            -Wno-deprecated-declarations \
            src/c-wspp.cpp \
            -Iinclude -Isubprojects/websocketpp -Isubprojects/asio/include \
            -lssl -lcrypto -lcrypt32 -lws2_32 \
            -Wl,-Bstatic \
            -Wno-deprecated
    else
        echo "lib for $PLATFORM is up to date"
    fi
fi

if [ -x "$(which i686-w64-mingw32-g++)" ]; then
    # cross build win32
    get_platform i686-w64-mingw32-g++
    mkdir -p build/$PLATFORM/lib
    DEST=build/$PLATFORM/lib/c-wspp$DLL_EXT

    if needs_build $DEST; then
        echo "Building lib for $PLATFORM"
        i686-w64-mingw32-g++ -o "$DEST" \
            -shared -fpic $OPTIMZE \
            -Wno-deprecated-declarations \
            src/c-wspp.cpp \
            -Iinclude -Isubprojects/websocketpp -Isubprojects/asio/include \
            -lssl -lcrypto -lcrypt32 -lws2_32 \
            -Wl,-Bstatic \
            -Wno-deprecated
    else
        echo "lib for $PLATFORM is up to date"
    fi
fi

