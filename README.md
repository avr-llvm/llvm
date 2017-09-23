# avr-llvm

This repository has been merged into [upstream llvm](http://llvm.org/viewvc/llvm-project/llvm/trunk/lib/Target/AVR/).
Development now occurs directly in LLVM trunk, while this repository exists here solely as an archive of old work and issues.

At this point in time, no changes or commits are added or otherwise backported to this repository.

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

## Viewing the historic avr-llvm fork

This repository contains a few years of development issues, pull requests, and branches prior to the merge of the backend into LLVM trunk.

The old development branch is named [`avr-support`](https://github.com/avr-llvm/llvm/tree/avr-support).

There is a [`master`](https://github.com/avr-llvm/llvm/tree/master) branch that points to the upstream LLVM commit that `avr-support` is based on.
