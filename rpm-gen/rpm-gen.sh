#!/bin/bash -e

function error()
{
    local ERROR_MESSAGE="$1"

    if [ -t 1 ] ; then
        echo -e "\033[;31m ** ERROR! **\033[0m $ERROR_MESSAGE" >&2
    else
        echo "** ERROR! ** $ERROR_MESSAGE" >&2
    fi

    exit 1
}

function validate_target()
{
    local target=

    for target in $AVAILABLE_TARGETS ; do
        if [ "x$target" = "x$TARGET" ] ; then
            return 0
        fi
    done

    error "Unrecognized target ‘${TARGET}’"
}

function exists_generator_docker_image()
{
    local TEST=$(docker images | grep hft-rpm-generator-centos7 | awk '{ print $1 }')

    if [ "x$TEST" = "xhft-rpm-generator-centos7" ] ; then
        return 0
    fi

    return 1
}

###############
## Main Exec
###############

declare -r SELF_DIR=$(D=$(dirname "$0") ; cd "$D" ; pwd)
declare -r AVAILABLE_TARGETS="hft-system"
declare -r TARGET="$1"
INSPECT=no

if [ "x$TARGET" = "x" ] || [ "x$TARGET" = "x--help" ] || [ "x$TARGET" = "x-h" ] ; then
    echo ""
    echo "RPM-GEN - Centos 7 distribution RPM packages generator for HFT"
    echo "Copytight (c) 2017 - 2021 by LLG Ryszard Gradowski, All Rights Reserved"
    echo
    echo "Usage:"
    echo "   $(basename $0) <target>|inspect"
    echo ""
    echo "Available targets are:"
    for t in $AVAILABLE_TARGETS ; do
        echo "    ${t}"
    done
    echo ""

    exit 0
elif [ "x$TARGET" = "xinspect" ] ; then
    INSPECT=yes
else
    validate_target
fi

cd "$SELF_DIR"

if ! exists_generator_docker_image ; then
    echo "Docker image ‘hft-rpm-generator-centos7’ must be generated, now proceeding..."
    pushd hft-rpmbuild-dockerimage-src > /dev/null
    docker build -t hft-rpm-generator-centos7 . || error "Failed to generate docker image ‘hft-rpm-generator-centos7’"
    popd > /dev/null
fi

## Start job inside docker container.

if [ $INSPECT = "no" ] ; then
    docker run --rm -v "${SELF_DIR}/exchange:/root/exchange" hft-rpm-generator-centos7 /root/rpm-generator $TARGET
elif [ $INSPECT = "yes" ] ; then 
    echo ""
    echo "Run manually ‘/root/rpm-generator <target>’, where"
    echo "target is on of the following: $AVAILABLE_TARGETS"
    echo ""
    docker run --rm -it -v "${SELF_DIR}/exchange:/root/exchange" hft-rpm-generator-centos7
else
    error "Logic error. Unrecognized value : ${INSPECT}"
fi
