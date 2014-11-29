##===- lib/Target/AVR/Makefile -----------------------------*- Makefile -*-===##
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
##===----------------------------------------------------------------------===##

LEVEL = ../../..
LIBRARYNAME = LLVMAVRCodeGen
TARGET = AVR

# Make sure that tblgen is run, first thing.
BUILT_SOURCES = AVRGenRegisterInfo.inc AVRGenInstrInfo.inc \
		AVRGenAsmWriter.inc \
		AVRGenDAGISel.inc AVRGenCallingConv.inc \
		AVRGenSubtargetInfo.inc \
		AVRGenMCCodeEmitter.inc

DIRS = InstPrinter TargetInfo MCTargetDesc

include $(LEVEL)/Makefile.common
