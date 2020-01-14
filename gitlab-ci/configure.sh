# clean up the last build

rm -rf build
mkdir build
cd build

# yes im assuming this is running on one of my runners,
# probably not gonna run no your machine

cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/llvm/llvm-project/build/bin/clang -DCMAKE_CXX_COMPILER=/llvm/llvm-project/build/bin/clang++ ../