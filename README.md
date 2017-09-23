# avr-llvm

This repository has been merged into [upstream llvm](http://llvm.org/viewvc/llvm-project/llvm/trunk/lib/Target/AVR/).

In order to compile stock LLVM will AVR support enabled, `-DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=AVR` must be passed to CMake.

## Compiling the up-to-date backend from upstream LLVM

```bash
# Clone the LLVM repository
git clone https://github.com/llvm-mirror/llvm.git

# optionally clone clang into the 'tools' folder for C/C++ compiler support
git clone https://github.com/llvm-mirror/clang.git llvm/tools/clang

# where we want to place object files/executables
mkdir build && cd build

# Run CMake to generate makefiles
cmake ../llvm -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=AVR

# Compile everything
make
```

Once this is done, you will have a number executables in `build/bin`.

