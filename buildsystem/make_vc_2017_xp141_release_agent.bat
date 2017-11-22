if not exist "..\build" mkdir ..\build
cd ..\build
cmake .. -DAPP_TYPE=Agent -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 15 2017" -T v141_xp