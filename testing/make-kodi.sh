#!/bin/bash

set -u

pushd ../build

declare -r VIS_GOOM_PATH=${HOME}/Prj/github/xbmc/visualization.goom
declare -r VIS_GOOM_MAIN_PATH=${VIS_GOOM_PATH}/src
declare -r GOOM_TESTING_PATH=${VIS_GOOM_PATH}/testing
declare -r GOOM_TESTING_UTILS_PATH=${GOOM_TESTING_PATH}/libs/utils/include
declare -r GOOM_TESTING_LIBS_PATH=${GOOM_TESTING_PATH}/libs
declare -r GOOM_TESTING_GOOM_PATH=${GOOM_TESTING_PATH}/libs/goom/include
export C_INCLUDE_PATH=${GOOM_TESTING_GOOM_PATH}
export CPLUS_INCLUDE_PATH=${GOOM_TESTING_UTILS_PATH}:${GOOM_TESTING_GOOM_PATH}:${VIS_GOOM_MAIN_PATH}:${GOOM_TESTING_LIBS_PATH}

make CXXFLAGS="-D DO_TESTING=1"

popd
