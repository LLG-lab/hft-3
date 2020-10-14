set -e

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

function prep()
{
    ## Obtain HFT source code.
    git clone https://github.com/LLG-lab/hft-3.git /root/rpmbuild/BUILD

    ## Attach precompiled boost libraries to the project tree.
    local DETECT_BOOST_DIR=$(pushd /root/ > /dev/null ; ls -d boost_* ; popd > /dev/null)

    if [ "x$DETECT_BOOST_DIR" = "x"  ] || [ ! -d "/root/$DETECT_BOOST_DIR" ] ; then
        error "Failed to autodetect boost directory ‘$DETECT_BOOST_DIR’"
    fi

    pushd /root/rpmbuild/BUILD/3rd_party > /dev/null
        ln -vs /root/${DETECT_BOOST_DIR} boost_current_release
        ln -vs /root/${DETECT_BOOST_DIR}/boost boost
    popd > /dev/null

    find /root/exchange/SPECS/ -type f -name "*.spec" -exec cp -av {} /root/rpmbuild/SPECS/ \;
}

function post()
{
    mkdir -p /root/exchange/RPMS/
    find /root/rpmbuild/RPMS/ -type f -name "*.rpm" -exec mv -v {} /root/exchange/RPMS/ \;
}

function get_build_number()
{
    is_natural_number() {
        [[ ${1} =~ ^[0-9]+$ ]]
    }

    local BUILD_UID=$(< /root/exchange/build.uid)

    if ! is_natural_number $BUILD_UID ; then
        error "Data ‘${BUILD_UID}’ is not a number. Fix file exchange/build.uid"
    fi

    BUILD_UID=$((BUILD_UID+1))

    echo $BUILD_UID > /root/exchange/build.uid
    echo $BUILD_UID
}

##############
## Main Exec
##############

declare -r TARGET="$1"
declare -r BUILD_NUMBER=$(get_build_number)

[ "x$BUILD_NUMBER" = "x" ] && exit 1

echo "Called internal rpm-generator for target ‘${TARGET}’, Build UID ‘${BUILD_NUMBER}’"

case $TARGET in
    hft-system)
        prep
        pushd /root/ > /dev/null
        rpmbuild -ba rpmbuild/SPECS/hft-system.spec --define "build_number ${BUILD_NUMBER}"
        popd > /dev/null
        post
        ;;

    *)
        error "Unrecognized target ‘${TARGET}’"
        ;;
esac

exit 0
