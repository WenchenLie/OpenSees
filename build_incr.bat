call "D:\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
call "D:\oneAPI\setvars.bat" intel64 mod
cd build\Release
cmake --build . --config Release --target OpenSees -j8
cmake --build . --config Release --target OpenSeesPy -j8
ren OpenSeesPy.dll opensees.pyd
copy /Y "D:\oneAPI\compiler\2024.2\bin\libiomp5md.dll" .
echo "========== Build Success =========="
pause
