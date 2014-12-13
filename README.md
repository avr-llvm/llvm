# AVR-LLVM
An LLVM backend for the Atmel [AVR](http://en.wikipedia.org/wiki/Atmel_AVR) microprocessor architecture.

Currently AVR-LLVM can generate assembly files and ELF object files.

Subtarget features are not implemented (yet), and so generated code will be incompatible to most AVR models except the higher end ones (which support the entire AVR instruction set).
This is being worked on.

As the project is still very much in development, it is likely that you will encounter bugs. If you think you've found one, submit an issue - we're aiming for AVR-LLVM to eventually be
a production-quality compiler backend, so bugs will not be tolerated.

See `AVR_SUPPORT.md` for a list of supported instructions.

## Development
Take a look at the issues page for goals and bugs that currently exist. Pull requests are very welcome!

## History
The original project started several years ago and then it was abandoned. It is hosted on [SourceForge.net](http://sourceforge.net/projects/avr-llvm).

A fork was created which updated the project to a more recent version of LLVM, which is located on [GitHub](https://github.com/sushihangover/llvm-avr).

This repository is a fork of the updated version.

The AVR-LLVM project aims to keep to code updated (so that it's always compatible with LLVM master), and also to improve the backend.
