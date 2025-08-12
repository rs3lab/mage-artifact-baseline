#!/bin/bash
set -x
set -u
set -e

export CC=gcc-9
export CXX=g++-9
export ROOT_PATH=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
export SCRIPT_DIR=${ROOT_PATH}/scripts
export SOURCE_DIR=${ROOT_PATH}/apps/memcached
export CMAKE_C_COMPILER=$CC
export CMAKE_CXX_COMPILER=$CXX