call "D:\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
call "D:\oneAPI\setvars.bat" intel64 mod
cd build\Release
cmake.exe -S . -B build/Release -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded" -DCMAKE_Fortran_COMPILER="ifx" -DBLA_STATIC=ON -DMKL_LINK=static -DMKL_INTERFACE_FULL=intel_lp64 -DMUMPS_DIR="..\..\mumps\build" -DCMAKE_EXE_LINKER_FLAGS="/FORCE:MULTIPLE" -DCMAKE_SHARED_LINKER_FLAGS="/FORCE:MULTIPLE" -DCMAKE_NINJA_FORCE_RESPONSE_FILE=ON
cmake --build . --config Release --target OpenSees -j8
cmake --build . --config Release --target OpenSeesPy -j8
ren OpenSeesPy.dll opensees.pyd
copy /Y "D:\oneAPI\compiler\2024.2\bin\libiomp5md.dll" .
echo "========== Build Success =========="
pause
