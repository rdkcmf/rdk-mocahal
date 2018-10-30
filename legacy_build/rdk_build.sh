#!/bin/bash
##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
#

#######################################
#
# Build Framework standard script for
#
# DTCP component

# use -e to fail on any shell issue
# -e is the requirement from Build Framework
set -e

# path to build script (this script)
export RDK_SCRIPTS_PATH=${RDK_SCRIPTS_PATH:-`readlink -m $0 | xargs dirname`}

# path to sources and target
export RDK_SOURCE_PATH=${RDK_SOURCE_PATH:-$(readlink -m ${RDK_SCRIPTS_PATH}/..)}
export RDK_TARGET_PATH=${RDK_TARGET_PATH:-${RDK_COMPONENT_PATH}}

# the path to the component base
export RDK_COMPONENT_PATH=${RDK_COMPONENT_PATH:-$(readlink -m ${RDK_SOURCE_PATH}/..)}
export RDK_COMPONENT_NAME=${RDK_COMPONENT_NAME:-$(basename ${RDK_COMPONENT_PATH})}

# the path to combined build
export RDK_PROJECT_ROOT_PATH=${RDK_PROJECT_ROOT_PATH:-$(readlink -m ${RDK_COMPONENT_PATH}/..)}
export COMBINED_ROOT=$RDK_PROJECT_ROOT_PATH

# fsroot and toolchain (valid for all devices)
export RDK_FSROOT_PATH=${RDK_FSROOT_PATH-$(readlink -m ${RDK_PROJECT_ROOT_PATH}/sdk/fsroot/ramdisk)}
export RDK_TOOLCHAIN_PATH=${RDK_TOOLCHAIN_PATH-$(readlink -m ${RDK_PROJECT_ROOT_PATH}/sdk/toolchain/staging_dir)}

# parse arguments
INITIAL_ARGS=$@

function usage()
{
    set +x
    echo "Usage: `basename $0` [-h|--help] [-v|--verbose] [action]"
    echo "    -h    --help                  : this help"
    echo "    -v    --verbose               : verbose output"
    echo
    echo "Supported actions:"
    echo "      configure, clean, build (DEFAULT), rebuild, install"
}


# options may be followed by one colon to indicate they have a required argument
if ! GETOPT=$(getopt -n "build.sh" -o hv -l help,verbose -- "$@")
then
    usage
    exit 1
fi

eval set -- "$GETOPT"

while true; do
  case "$1" in
    -h | --help ) usage; exit 0 ;;
    -v | --verbose ) set -x ;;
    -- ) shift; break;;
    * ) break;;
  esac
  shift
done

ARGS=$@

function setup_env()
{
    source ${RDK_PROJECT_ROOT_PATH}/sdk/scripts/setBCMenv.sh

    export GCC=${B_REFSW_ARCH}-gcc
    export GXX=${B_REFSW_ARCH}-g++
    export LD=${B_REFSW_ARCH}-ld
    export CC=${B_REFSW_ARCH}-gcc
    export CXX=${B_REFSW_ARCH}-g++
    export DEFAULT_HOST=${B_REFSW_ARCH}

    export RMH_START_DEFAULT_SINGLE_CHANEL=${RMH_START_DEFAULT_SINGLE_CHANEL:-"1150"}
    export RMH_START_DEFAULT_POWER_REDUCTION=${RMH_START_DEFAULT_POWER_REDUCTION:-"9"}
    export RMH_START_DEFAULT_TABOO_START_CHANNEL=${RMH_START_DEFAULT_TABOO_START_CHANNEL:-"41"}
    export RMH_START_DEFAULT_TABOO_MASK=${RMH_START_DEFAULT_TABOO_MASK:-"0x00FBFFFF"}
    export RMH_START_SET_MAC_FROM_PROC=${RMH_START_SET_MAC_FROM_PROC:-"false"}
    export RMH_START_DEFAULT_PREFERRED_NC=${RMH_START_DEFAULT_PREFERRED_NC:-"false"}

    export CFLAGS="\
 -DRMH_START_DEFAULT_SINGLE_CHANEL=${RMH_START_DEFAULT_SINGLE_CHANEL}\
 -DRMH_START_DEFAULT_POWER_REDUCTION=${RMH_START_DEFAULT_POWER_REDUCTION}\
 -DRMH_START_DEFAULT_TABOO_START_CHANNEL=${RMH_START_DEFAULT_TABOO_START_CHANNEL}\
 -DRMH_START_DEFAULT_TABOO_MASK=${RMH_START_DEFAULT_TABOO_MASK}\
 -DRMH_START_DEFAULT_PREFERRED_NC=${RMH_START_DEFAULT_PREFERRED_NC}\
 -DRMH_START_SET_MAC_FROM_PROC=${RMH_START_SET_MAC_FROM_PROC} "

    if [ "${RDK_PLATFORM_DEVICE}" ]; then
        CFLAGS+="-DMACHINE_NAME='\"${RDK_PLATFORM_DEVICE^^}\"'"
    fi

    return 0
}

function configure()
{
    # First create the header file that will be used to compile the SoC library
    generate_rmh_soc_header

    pushd ${RDK_SOURCE_PATH}
    aclocal -I cfg
    libtoolize --automake
    automake --foreign --add-missing
    rm -f configure
    autoconf
    echo "  CONFIG_MODE = $CONFIG_MODE"
    configure_options=" "
    if [ "x$DEFAULT_HOST" != "x" ]; then
        configure_options="--host ${DEFAULT_HOST}"
    fi

    ./configure --prefix=${RDK_FSROOT_PATH}/usr $configure_options
    popd
    return 0
}

function clean()
{
    pushd ${RDK_SOURCE_PATH}
    if [ -f Makefile ]; then
            make distclean
    fi
    rm -f configure;
    rm -rf aclocal.m4 autom4te.cache config.log config.status libtool
    find . -iname "Makefile.in" -exec rm -f {} \;
    find . -iname "Makefile" | xargs rm -f
    ls cfg/* | grep -v "Makefile.am" | xargs rm -f
    popd
    return 0
}

function build()
{
    make -C ${RDK_SOURCE_PATH}
    return 0
}

function install()
{

    make -C ${RDK_SOURCE_PATH} install
    return 0
}

function rebuild()
{
    clean
    configure
    build
    return 0
}

function generate_rmh_soc_header() {
    cp ${RDK_SOURCE_PATH}/rmh_interface/soc_header/rmh_soc_header.h ${RDK_SOURCE_PATH}/rmh_interface/rmh_soc.h
    (
      echo " "
      echo "/**********************************************"
      echo "* ====== THIS HEADER WAS AUTO GENERATED ======"
      echo "* Date: $(date)"
      echo "* Branch: ${RDK_GIT_BRANCH}"
      echo "* Revision: ${PV}"
      echo "**********************************************/"
      echo '#include "rmh_type.h"'
      echo " "
      echo '#ifdef __cplusplus'
      echo 'extern "C" {'
      echo '#endif'
      echo " "
      gcc -E -P ${RDK_SOURCE_PATH}/rmh_interface/soc_header/rmh_generate_soc_header.c -I${RDK_SOURCE_PATH}/rmh_interface -I${RDK_SOURCE_PATH}/rmh_lib
      echo " "
      echo '#ifdef __cplusplus'
      echo '}'
      echo '#endif'
      echo " "
    ) >> ${RDK_SOURCE_PATH}/rmh_interface/rmh_soc.h

    # Cleanup empty lines from gcc. We could do this directly on the output of gcc but then we have to add a pipefail to catch potential
    # failures. Also if we use pipefail bitbake will show the error in sed rather than gcc. So for now we will just delete them all at
    # the end and if you want a blank line, add a space.
    sed -i '/^$/d' ${RDK_SOURCE_PATH}/rmh_interface/rmh_soc.h
    return 0
}


setup_env
HIT=false

for i in "$ARGS"; do
    case $i in
        configure)  HIT=true; configure ;;
        clean)      HIT=true; clean ;;
        build)      HIT=true; build ;;
        rebuild)    HIT=true; rebuild ;;
        install)    HIT=true; install ;;
        *)
            #skip unknown
        ;;
    esac
done

# if not HIT do build by default
if ! $HIT; then
  build
fi
