#!/usr/bin/env bash
# File       : correct_style
# Created    : Mon Jan 20 2020 11:29:59 AM (+0100)
# Author     : Fabian Wermelinger (adjusted to Korali by Sergio Martin and Lucas Amoudruz)
# Description: Utility to adjust source code according to style conventions
# Copyright 2020 ETH Zurich. All Rights Reserved.

function check()
{ 
 if [ ! $? -eq 0 ]
 then
  echo "[Korali] Error fixing style."
  exit -1
 fi 
}

fileDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
pushd $fileDir

root=$fileDir/../..
 
##############################################
### Correcting C++ Code Style
##############################################

# If clang-format is not installed, run the installation script
clangFormatBin=${root}/external/clang-format
if [ ! -f $clangFormatBin ]; then
 pushd ${root}
 ./external/install_clang.sh 
 check
 popd
fi

src_files=`find $root -type f -not -name "__*"  -name "*pp" -name "*pp" \
          -not -path "${root}/tools/dev-tools/*" \
          -not -path "${root}/external/*" \
          -not -path "${root}/examples/test.cases/*"`

echo $src_files | \
    xargs -n6 -P2 $clangFormatBin -style=file -i "$@"

check

popd
