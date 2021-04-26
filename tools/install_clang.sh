#!/bin/bash

######### Global Definitions ########
libName="llvm"
binName="clang-format"
minVersion=10.0.0

######### Helper Functions ########

function check()
{
 if [ ! $? -eq 0 ]
 then
  exit $?
 fi 
}

######## Checking for existing software ########

baseLibDir=./_${libName}
installDir=./llvm
buildDir=./build

binFound=0
binPath=${installDir}/bin/${binName}

if [ ! -f ${binPath} ]; then
 binPath=${binName}
fi

$binPath --version > /dev/null 2>&1
if [ $? -eq 0 ]; then
 binVersion=`${binPath} --version | head -n 1`
 cmpver=`printf "${binVersion}\n${minVersion}" | sort -V | head -n 1 | cut -d' ' -f 3`
 
 if [[ "$cmpver" != "$minVersion" ]]; then
    echo " ${libName} version found (${binVersion}) is smaller than required (${minVersion})."
 else
    binFound=1
    echo " Found existing ${libName} version ${binVersion}. Skipping installation..."
 fi
fi

######## If not installed, download and install ########

if [ ${binFound} == 0 ]; then

 echo " Downloading ${libName}... "
 
 rm -rf $buildDir; check
 rm -rf $installDir; check
 
 mkdir -p $buildDir; check
 mkdir -p $installDir; check
 
 pushd $buildDir; check
  
 arch="$(uname -s)"; check
 
 # If using MacOs, use the Darwin package 
 if [ "$arch" == "Darwin" ]; then
  
  wget https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/clang+llvm-10.0.0-x86_64-apple-darwin.tar.xz;  check
  
  tar -xf clang+llvm-10.0.0-x86_64-apple-darwin.tar.xz;  check
  
  mv clang+llvm-10.0.0-x86_64-apple-darwin/* ../llvm; check
 
 else  # Else default to Linux64

  wget https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/clang+llvm-10.0.0-x86_64-linux-sles11.3.tar.xz; check
  
  tar -xf clang+llvm-10.0.0-x86_64-linux-sles11.3.tar.xz;  check
  
  mv clang+llvm-10.0.0-x86_64-linux-sles11.3 llvm;  check
  
  # Fix for systems that have no libcurses lib
  ln -s libunwind.so.1.0 llvm/lib/libncurses.so.5;  check
  
  # Finally copy it to destination
  mv llvm/* ../llvm;  check
  
 fi
 
 popd; check
 
 echo " Finished installing ${libName}."
 binPath=${installDir}/bin/${binName}
 
 echo " Cleaning up build folder..."
 rm -rf $buildDir; check
fi

######## Finalization ######## 

fullBinPath=`which ${binPath}`
binVersion=`echo $versionLine | head -n 1 | cut -d' ' -f 3`; check 
echo " Using ${libName} version $binVersion"

exit 0
