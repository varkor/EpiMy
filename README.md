# EpiMy
A compositional Epilog and MysoreScript compiler

## Building and testing
```
brew install llvm
rm -rf build; cmake -H. -Bbuild -DLLVM_CONFIG:FILEPATH=/usr/local/Cellar/llvm/3.9.0/bin/llvm-config
make -C build
./bin/epimy examples/hello.epimy
```