sh Test-Gcc.sh
if [ $? -ne 0 ]
then
    exit -1
fi

sh Test-Clang.sh
if [ $? -ne 0 ]
then
    exit -1
fi

# Test Windows Cross Compilation
sh Test-MinGW.sh
if [ $? -ne 0 ]
then
    exit -1
fi

