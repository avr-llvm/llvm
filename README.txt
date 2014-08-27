AVR Backend for LLVM
====================

I found an old fork of an LLVM AVR backend of an old fork of an LLVM AVR
backend. I have updated the code so that it works with the current master
branch of LLVM. It currently can output AVR assembly (*.s) files. The AVR
backend is licensed under BSD (according to the original project's
SourceForge page), and so it should be compatible with LLVM's BSD-like
license.

Original project:   http://sourceforge.net/projects/avr-llvm
Slighty newer fork: https://github.com/sushihangover/llvm-avr



Here proceeds the official LLVM README:

Low Level Virtual Machine (LLVM)
================================

This directory and its subdirectories contain source code for the Low Level
Virtual Machine, a toolkit for the construction of highly optimized compilers,
optimizers, and runtime environments.

LLVM is open source software. You may freely distribute it under the terms of
the license agreement found in LICENSE.txt.

Please see the documentation provided in docs/ for further
assistance with LLVM, and in particular docs/GettingStarted.rst for getting
started with LLVM and docs/README.txt for an overview of LLVM's
documentation setup.

If you're writing a package for LLVM, see docs/Packaging.rst for our
suggestions.


