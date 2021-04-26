#!/usr/bin/env bash

function check()
{
 if [ ! $? -eq 0 ]
 then 
  echo "[Jaffar] Error fixing style."
  exit -1 
 fi
}

fileDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
pushd $fileDir

root=$fileDir/../source/

##############################################
### Testing C++ Code Style
##############################################

# If clang-format is not installed, run the installation script
clangFormatBin=llvm/bin/clang-format
if [ ! -f $clangFormatBin ]; then
 pushd ${fileDir}
 ./install_clang.sh
 popd
fi
 
# If run-clang-format is not installed, clone it
if [ ! -f  run-clang-format/run-clang-format.py ]; then

 git clone https://github.com/Sarcasm/run-clang-format.git
 if [ ! $? -eq 0 ]; then
  echo "[Jaffar] Error installing run-clang-format."
  exit 1
 fi

fi

clangFormatCmd=clang-format
if [ ! -f $clangFormatCmd ]; then
 clangFormatCmd=llvm/bin/clang-format
fi

runClangCmd=run-clang-format/run-clang-format.py

python3 $runClangCmd --clang-format-executable $clangFormatCmd -r ${root} --extensions cc,h > /dev/null

if [ ! $? -eq 0 ]; then
 echo "[Jaffar] Error: C++ Code formatting is not normalized."
 echo "[Jaffar] Solution: Please run $fileDir/fix_style.sh to fix it."
 exit -1
else
 echo "[Jaffar] C++ Code formatting is correct."
fi

popd

exit 0
