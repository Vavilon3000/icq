if not exist "..\build" mkdir ..\build
cd ..\build
call "%VS140COMNTOOLS%\VsDevCmd.bat"
call "%VS140COMNTOOLS%\VsMSBuildCmd.bat"
cmake .. -DAPP_TYPE=%1 -D%2=ON -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 15 2017" -T v141_xp
MSBuild ../build/icq.sln /m /t:Rebuild /p:VisualStudioVersion=15.0
exit %errorlevel%
