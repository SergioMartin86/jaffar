#!/usr/bin/env bash

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

root=$fileDir/../source
 
##############################################
### Correcting C++ Code Style
##############################################

# If clang-format is not installed, run the installation script
clangFormatBin=llvm/bin/clang-format
if [ ! -f $clangFormatBin ]; then
 pushd ${root}
 .install_clang.sh 
 check
 popd
fi

src_files=`find $root -type f -name "*.cc" -o -name "*.h"`
echo $src_files | \
    xargs -n6 -P2 $clangFormatBin -style=file -i "$@"

check

popd
