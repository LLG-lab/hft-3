#!/bin/bash -e

PROJECT_TREE_DIRECTORY=$(dirname $0)/../

pushd $PROJECT_TREE_DIRECTORY > /dev/null

#
# Check if we have boost.
#

BOOST_DIRECTORY="3rd_party/boost_1_66_0"
BOOST_TARBALL=boost_1_66_0.tar.bz2
BOOST_URL="https://dl.bintray.com/boostorg/release/1.66.0/source/$BOOST_TARBALL"

if [ -d $BOOST_DIRECTORY ] ; then
    echo "Boost libraries already downloaded into the project tree"
else
    echo "Going to download boost"
    cd 3rd_party/
    wget $BOOST_URL
    echo "Uncompressing tarball $BOOST_TARBALL"
    tar -jxf $BOOST_TARBALL
    rm -f $BOOST_TARBALL
    cd ../
fi

#
# Check if we have compiled boost libraries.
#

if [ -d $BOOST_DIRECTORY/stage/lib ] ; then
    echo "Boost libraries seems to be already compiled"
else
    echo "Boost libraries have to be compiled"

    #
    # Check if it is b2 build tool.
    #

    if [ ! -f $BOOST_DIRECTORY/b2 ] ; then
        pushd $BOOST_DIRECTORY > /dev/null
        ./bootstrap.sh
        popd > /dev/null
    fi

    #
    # Boost libraries compilation.
    #

    pushd $BOOST_DIRECTORY > /dev/null
    ./b2 --build-dir=build-directory toolset=gcc cxxflags=-std=gnu++0x stage
    popd > /dev/null
fi

pushd 3rd_party > /dev/null
rm -f boost_current_release
ln -vs $(basename $BOOST_DIRECTORY) boost_current_release
rm -f boost
ln -vs $(basename $BOOST_DIRECTORY)/boost boost
popd > /dev/null

popd > /dev/null

