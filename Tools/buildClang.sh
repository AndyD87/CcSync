CLANG_VERSION=-7
if [ -z "$1" ]
  then echo "Default clang version: $CLANG_VERSION"
  else CLANG_VERSION=-$1
fi
cd ..
mkdir Solution-Clang
cd Solution-Clang
export CC=clang$CLANG_VERSION
export CXX=clang++$CLANG_VERSION
cmake ../ -DCMAKE_INSTALL_PREFIX=/usr
make -j $(nproc)
cd ..
cd Tools
