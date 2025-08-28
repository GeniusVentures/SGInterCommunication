cd build
cd Windows
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release ../..
cmake --build . --parallel 8 --config Release