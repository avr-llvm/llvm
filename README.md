# AVR-LLVM

See the [Wiki](https://github.com/dylanmckay/avr-llvm/wiki/Getting%20Started) for more information.

An LLVM backend for the Atmel [AVR](http://en.wikipedia.org/wiki/Atmel_AVR) microprocessor architecture.

Currently AVR-LLVM can generate assembly files and ELF object files.

As the project is still very much in development, it is likely that you will encounter bugs. If you think you've found one, submit an issue - we're aiming for AVR-LLVM to eventually be
a production-quality compiler backend, so bugs will not be tolerated.

See `AVR_SUPPORT.md` for a list of supported instructions.

AVR-related code can be found in `lib/Target/AVR`.

## Mailing List

The mailing list can be found [here](https://lists.sourceforge.net/lists/listinfo/avr-llvm-backend-mail).

## Features

* Machine code generation
* Subtarget features (not completely finished)
* Support for most instructions
* ELF object outputting

## Development

Take a look at the issues page for goals and bugs that currently exist. Pull requests are very welcome!

## History

The original project started several years ago and then it was abandoned. It is hosted on [SourceForge.net](http://sourceforge.net/projects/avr-llvm).

A fork was created which updated the project to a more recent version of LLVM, which is located on [GitHub](https://github.com/sushihangover/llvm-avr).

This repository is a fork of the updated version.

The AVR-LLVM project aims to keep to code updated (so that it's always compatible with LLVM master), and also to improve the backend.

## Goals

We would like to be able to compile any program that AVR-GCC can, have comparable diagnostics, and on-par performance.

Other goals include:
* Support the LLVM `compiler-rt` library
