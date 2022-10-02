cd ..
mkdir Solution
cd Solution
cmake ../Sources -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
make -j $(nproc)
cd ..
cd Tools
