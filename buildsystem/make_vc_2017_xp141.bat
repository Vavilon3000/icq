if not exist "..\build" mkdir ..\build
cd ..\build
cmake .. -DAPP_TYPE=ICQ -DCMAKE_BUILD_TYPE=Debug -G "Visual Studio 15 2017" -T v141_xp