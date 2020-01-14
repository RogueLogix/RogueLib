
echo $1
echo $2
echo "$2"
$1 -DLLVM_TARGETS_TO_BUILD="X86;PowerPC;ARM;NVPTX" $2 ../