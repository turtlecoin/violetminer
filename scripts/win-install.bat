set PATH=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.1\bin;%PATH%
set PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin;%PATH%
set PATH=C:\Program Files\cmake\bin;%PATH%

set CudaToolkitDir=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.1

mkdir build
cd build

cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_VS_PLATFORM_TOOLSET_CUDA=11.1 .. || type CMakeFiles\CMakeError.log || type CMakeFiles\CMakeOutput.log || exit /b 1

MSBuild violetminer.sln /p:Configuration=Release /m /v:n

cd Release

argon2-cpp-test
