#!/bin/bash
BUILD_TARGET_PLATFORM=sim
BUILD_AIPU_VERSION=all
BUILD_DEBUG_FLAG=release
BUILD_KERNEL_VERSION=no
BUILD_TEST=yes
BUILD_BASIC_TEST_ONLY=0
BUILD_UMD_API_TYPE=standard_api
BUILD_WITH_NUMPY=no

set -e

build_help() {
    echo "============================Driver Build Help=============================="
    echo "Test Build Options:"
    echo "-h, --help        help"
    echo "-d, --debug       build debug version (release if not specified)"
    echo "-p, --platform    target platform (optional, by default sim)"
    echo "                    - sim"
    echo "                    - juno"
    echo "                    - 6cg"
    echo "                    - hybrid"
    echo "                    - r329"
    echo "                    - x86"
    echo "                    - [other platforms you add]"
    echo "-k, --kversion    kernel version (optional)"
    echo "                    - [major].[minor]"
    echo "                    - [major].[minor].[patch]"
    echo "-v, --version     AIPU version (optional, by default build all):"
    echo "                    - z1"
    echo "                    - z2"
    echo "                    - z3"
    echo "                    - all"
    echo "-a, --api         Build UMD API type (optional, by default standard api):"
    echo "                    - standard_api"
    echo "                    - python_api"
    echo "--np              Build NumPy in Python API UMD (optional)"
    echo "==========================================================================="
    exit 1
}

ARGS=`getopt -o hdp:v:k:a: --long help,debug,platform:,version:,kversion:,api:,np -n 'build_all.sh' -- "$@"`
eval set -- "$ARGS"

while [ -n "$1" ]
do
    case "$1" in
     -h|--help)
         build_help
         ;;
     -d|--debug)
         BUILD_DEBUG_FLAG=debug
         ;;
     -p|--platform)
         BUILD_TARGET_PLATFORM="$2"
         shift
         ;;
     -v|--version)
         BUILD_AIPU_VERSION="$2"
         shift
         ;;
     -k|--kversion)
         BUILD_KERNEL_VERSION="$2"
         shift
         ;;
     -a|--api)
         BUILD_UMD_API_TYPE="$2"
         shift
         ;;
     --np)
         BUILD_WITH_NUMPY=yes
         ;;
     --)
         shift ;
         break
         ;;
     *)
         break
    esac
    shift
done

if [ "$COMPASS_DRV_BRENVAR_SETUP_DONE"x != "yes"x ]; then
    echo -e "\033[31;1m[DRV ERROR]\033[0m please source env_setup.sh first before building"
    exit 1
fi

if [ "$BUILD_TARGET_PLATFORM"x = "x86"x ]; then
    echo -e "$COMPASS_DRV_BRENVAR_ERROR x86-haps driver is not supported currently, do you mean x86-simulator (please use -p sim)?"
    exit 1
fi

if [ "$BUILD_TARGET_PLATFORM"x != "sim"x ]; then
    export BUILD_AIPU_VERSION_KMD=BUILD_ZHOUYI_$(echo $BUILD_AIPU_VERSION | tr '[a-z]' '[A-Z]')
    export BUILD_TARGET_PLATFORM_KMD=BUILD_PLATFORM_$(echo $BUILD_TARGET_PLATFORM | tr '[a-z]' '[A-Z]')
fi

if [ "$BUILD_AIPU_VERSION"x == "z1"x ]  ||
   [ "$BUILD_AIPU_VERSION"x == "z2"x ]  ||
   [ "$BUILD_AIPU_VERSION"x == "z3"x ]; then
    BUILD_AIPU_VERSION=z123
elif [ "$BUILD_AIPU_VERSION"x != "all"x ]; then
    echo -e "$COMPASS_DRV_BRENVAR_ERROR Invalid AIPU version $BUILD_AIPU_VERSION"
    exit 2
fi

if [ "$BUILD_UMD_API_TYPE"x != "standard_api"x ] &&
   [ "$BUILD_UMD_API_TYPE"x != "python_api"x ]; then
    echo -e "$COMPASS_DRV_BRENVAR_ERROR Invalid UMD lib API type $BUILD_UMD_API_TYPE"
    exit 3
fi
export BUILD_UMD_API_TYPE=$BUILD_UMD_API_TYPE

if [ "$BUILD_UMD_API_TYPE"x = "python_api"x ]; then
    export BUILD_WITH_NUMPY=$BUILD_WITH_NUMPY
fi

### export toolchain/kpath env variables of your supported platform(s)
if [ "$BUILD_TARGET_PLATFORM"x = "sim"x ]; then
    export CXX=$COMPASS_DRV_BTENVAR_X86_CXX
    export LD_LIBRARY_PATH=$CONFIG_DRV_BRENVAR_X86_CLPATH:$LD_LIBRARY_PATH
else
    export CXX=$COMPASS_DRV_BTENVAR_CROSS_CXX
    export PATH=$CONFIG_DRV_BTENVAR_CROSS_CXX_PATH:$PATH

    if [ "$BUILD_TARGET_PLATFORM"x = "hybrid"x ]; then
        export CROSS_COMPILE=$COMPASS_DRV_BTENVAR_CROSS_COMPILE
    else
        export CROSS_COMPILE=$COMPASS_DRV_BTENVAR_CROSS_COMPILE_GNU
    fi

    if [ "$BUILD_KERNEL_VERSION"x = "no"x ]; then
        KPATH=CONFIG_DRV_BTENVAR_$(echo $BUILD_TARGET_PLATFORM | tr '[a-z]' '[A-Z]')_KPATH
    else
        KVERSION=$(echo $BUILD_KERNEL_VERSION | tr '[.]' '[_]')
        KPATH=CONFIG_DRV_BTENVAR_$(echo $BUILD_TARGET_PLATFORM | tr '[a-z]' '[A-Z]')_${KVERSION}_KPATH
    fi

    if [ -z ${!KPATH} ]; then
        echo -e "$COMPASS_DRV_BRENVAR_ERROR Environment variable $KPATH was used but not set"
        exit 4
    fi
    export COMPASS_DRV_BTENVAR_KPATH=${!KPATH}
fi

export BUILD_TARGET_PLATFORM
export BUILD_AIPU_VERSION
export BUILD_DEBUG_FLAG
export BUILD_AIPU_DRV_ODIR=$COMPASS_DRV_BRENVAR_ODIR_TOP/$BUILD_TARGET_PLATFORM/$BUILD_DEBUG_FLAG

mkdir -p $COMPASS_DRV_BRENVAR_ODIR_TOP
mkdir -p $COMPASS_DRV_BRENVAR_ODIR_TOP/$BUILD_TARGET_PLATFORM
mkdir -p $BUILD_AIPU_DRV_ODIR
rm -rf $COMPASS_DRV_BTENVAR_UMD_BUILD_DIR

echo -e "$COMPASS_DRV_BRENVAR_INFO Start building [$BUILD_DEBUG_FLAG version] compass driver..."
echo -e "$COMPASS_DRV_BRENVAR_INFO Target platform is $BUILD_TARGET_PLATFORM"
echo -e "$COMPASS_DRV_BRENVAR_INFO Target AIPU version is $BUILD_AIPU_VERSION"
echo -e "$COMPASS_DRV_BRENVAR_INFO UMD API type is $BUILD_UMD_API_TYPE"
### Build KMD
if [ "$BUILD_TARGET_PLATFORM"x != "sim"x ]; then
    cd $COMPASS_DRV_BTENVAR_KMD_DIR
        echo -e "$COMPASS_DRV_BRENVAR_INFO Build KMD..."
        cp include/uapi/misc/armchina_aipu.h $COMPASS_DRV_BTENVAR_KPATH/include/uapi/misc
        make -j32 ARCH=$COMPASS_DRV_BTENVAR_ARCH CROSS_COMPILE=$CROSS_COMPILE
        if [ -f aipu.ko ]; then
            cp aipu.ko $BUILD_AIPU_DRV_ODIR
            echo -e "$COMPASS_DRV_BRENVAR_INFO Build KMD done: binaries are in $BUILD_AIPU_DRV_ODIR"
        fi
        make clean
    cd -
fi

### Build UMD
cd $COMPASS_DRV_BTENVAR_UMD_DIR
    echo -e "$COMPASS_DRV_BRENVAR_INFO Build UMD..."
    make -j32 $BUILD_UMD_API_TYPE CXX=$CXX
cd -
echo -e "$COMPASS_DRV_BRENVAR_INFO Build UMD done: binaries are in $BUILD_AIPU_DRV_ODIR"

if [ "$BUILD_UMD_API_TYPE"x = "python_api"x ]; then
    exit 0
fi

if [ $BUILD_TEST = no ]; then
    exit 0
fi

ln -bs $COMPASS_DRV_BTENVAR_UMD_SO_NAME_FULL    $BUILD_AIPU_DRV_ODIR/$COMPASS_DRV_BTENVAR_UMD_SO_NAME_MAJOR
ln -bs $COMPASS_DRV_BTENVAR_UMD_SO_NAME_MAJOR   $BUILD_AIPU_DRV_ODIR/$COMPASS_DRV_BTENVAR_UMD_SO_NAME

### Build Armchina private test application(s)
rm -rf $COMPASS_DRV_BTENVAR_TEST_BUILD_DIR
cd $COMPASS_DRV_BTENVAR_TEST_DIR
# Build for either simulator or arm64
if [ "$BUILD_TARGET_PLATFORM"x = "sim"x ]; then
    make -j32 CXX=$CXX BUILD_TEST_CASE=simulation_test
else
    make -j32 CXX=$CXX BUILD_TEST_CASE=benchmark_test
fi

cd -
echo -e "$COMPASS_DRV_BRENVAR_INFO Build test(s) done: binaries are in $BUILD_AIPU_DRV_ODIR"