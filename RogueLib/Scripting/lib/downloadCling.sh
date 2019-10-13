cd ../..
rm -r ./cling
git clone http://root.cern.ch/git/llvm.git cling
cd cling
mkdir build
git checkout cling-patches
cd tools
git clone http://root.cern.ch/git/cling.git
git clone http://root.cern.ch/git/clang.git
cd clang
git checkout cling-patches
cd ../..