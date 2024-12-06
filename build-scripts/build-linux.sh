#!/bin/bash -e

cd $(dirname ${BASH_SOURCE[0]})/..
ROOT_PATH="$(pwd)"
PYMESH_PATH="$ROOT_PATH"
PATCH_PATH="$ROOT_PATH/build-scripts/patches"
VCPKG_PATH="$ROOT_PATH/vcpkg"

# Apply patches
if [ ! -f "$PYMESH_PATH/build/.patch_done_tag" ]; then
    cd $PATCH_PATH
    declare -a PATCHES=($(find . -name '*.patch'))
    for patch in "${PATCHES[@]}"; do
        cd $ROOT_PATH/third_party/$(dirname $patch)
        git apply $PATCH_PATH/$patch
    done
    mkdir -p $PYMESH_PATH/build
    touch $PYMESH_PATH/build/.patch_done_tag
    cd $ROOT_PATH  # return to root path
fi

if [ ! -d "$VCPKG_PATH" ]; then
    git clone --depth 1 https://github.com/microsoft/vcpkg.git
    cd vcpkg
    ./bootstrap-vcpkg.sh
    cd $ROOT_PATH
fi

if [ ! -f "$VCPKG_PATH/installed/x64-linux/include/boost/system.hpp" ] ; then
    cd $VCPKG_PATH
	# of coz, simply vcpkg install boost is fine but it takes too much time & space.
	# but get these list is so painful.
    ./vcpkg install boost-filesystem boost-system boost-program-options boost-chrono boost-date-time boost-thread
	./vcpkg install boost-foreach boost-math boost-multiprecision boost-property-map boost-graph
	./vcpkg install boost-heap boost-logic boost-format boost-ptr-container boost-callable-traits
    cd $ROOT_PATH
fi

export CMAKE_PREFIX_PATH="$VCPKG_PATH/installed/x64-linux"

if [ ! -f "$PYMESH_PATH/third_party/build/.all_done_tag" ] ; then
    cd $PYMESH_PATH/third_party
    if python3 build.py all ; then
        touch $PYMESH_PATH/third_party/build/.all_done_tag
    fi
    cd $ROOT_PATH
fi

if [ ! -f "$PYMESH_PATH/build/Makefile" ] ; then
    cd "$PYMESH_PATH/build"
    cmake ..
fi

if [ ! -f "$PYMESH_PATH/build/.all_done_tag" ] ; then
    cd "$PYMESH_PATH/build"
	if cmake --build . ; then
		cmake --build . --target tests && touch $PYMESH_PATH/build/.all_done_tag
	else
		exit 1
	fi
fi

env PYTHONPATH="$PYMESH_PATH/python/pymesh/lib:$PYMESH_PATH/python:$PYTHONPATH" python3 -c "import pymesh;pymesh.test(verbose=3)"
if [ $? -eq 0 ] ; then
	echo "Cong! ALL DONE! (Note: expected failure is NOT an error)"
fi
