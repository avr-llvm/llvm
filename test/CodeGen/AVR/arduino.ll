; Test to compile "Blink" arduino sketch with required arduino libraries
; Anton Smirnov (dev@antonsmirnov.name)

; RUN: clang -c -g -Os -Wall -fno-exceptions -ffunction-sections -fdata-sections -mmcu=atmega328p -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=105 -I%S/Inputs/arduino/avr/cores/arduino -I%S/Inputs/arduino/avr/variants/standard --target=avr %S/Inputs/Blink.cpp -o %T/Blink.cpp.o -I%S/Inputs/tools/avr/avr/include

; RUN: clang -c -g -Os -Wall -ffunction-sections -fdata-sections -mmcu=atmega328p -DARDUINO=10600 -DARDUINO_AVR_UNO -DARDUINO_ARCH_AVR  -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -I%S/Inputs/arduino/avr/cores/arduino -I%S/Inputs/arduino/avr/variants/standard %S/Inputs/arduino/avr/cores/arduino/hooks.c -o %T/hooks.c.o -I%S/Inputs/tools/avr/avr/include --target=avr

; RUN: clang -c -g -Os -ffunction-sections -fdata-sections -mmcu=atmega328p -DARDUINO=10600 -DARDUINO_AVR_UNO -DARDUINO_ARCH_AVR  -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -I%S/Inputs/arduino/avr/cores/arduino -I%S/Inputs/arduino/avr/variants/standard -I%S/Inputs/tools/avr/avr/include --target=avr %S/Inputs/arduino/avr/cores/arduino/WInterrupts.c -o %T/WInterrupts.c.o

; RUN: clang -c -g -Os -Wall -ffunction-sections -fdata-sections -mmcu=atmega328p -DARDUINO=10600 -DARDUINO_AVR_UNO -DARDUINO_ARCH_AVR  -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -I%S/Inputs/arduino/avr/cores/arduino -I%S/Inputs/arduino/avr/variants/standard -I%S/Inputs/tools/avr/avr/include --target=avr %S/arduino/avr/cores/arduino/wiring.c -o %T/wiring.c.o

; RUN: clang -c -g -Os -Wall -ffunction-sections -fdata-sections -mmcu=atmega328p -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=105 -I%S/Inputs/arduino/avr/cores/arduino -I%S/Inputs/arduino/avr/variants/standard -I%S/Inputs/tools/avr/avr/include --target=avr %S/arduino/avr/cores/arduino/wiring_analog.c -o %T/wiring_analog.c.o

; RUN: clang -c -g -Os -Wall -ffunction-sections -fdata-sections -mmcu=atmega328p -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=105 -I%S/Inputs/arduino/avr/cores/arduino -I%S/Inputs/arduino/avr/variants/standard -I%S/Inputs/tools/avr/avr/include --target=avr %S/Inputs/arduino/avr/cores/arduino/wiring_digital.c -o %T/wiring_digital.c.o

; RUN: clang -c -g -Os -Wall -ffunction-sections -fdata-sections -mmcu=atmega328p -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=105 -I%S/Inputs/arduino/avr/cores/arduino -I%S/Inputs/arduino/avr/variants/standard -I%S/Inputs/tools/avr/avr/include --target=avr %S/Inputs/arduino/avr/cores/arduino/wiring_pulse.c -o %T/wiring_pulse.c.o

; RUN: clang -c -g -Os -Wall -ffunction-sections -fdata-sections -mmcu=atmega328p -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=105 -I%S/Inputs/arduino/avr/cores/arduino -I%S/Inputs/arduino/avr/variants/standard -I%S/Inputs/tools/avr/avr/include --target=avr %S/Inputs/arduino/avr/cores/arduino/wiring_shift.c -o %T/wiring_shift.c.o

; RUN: clang -c -g -Os -Wall -ffunction-sections -fdata-sections -mmcu=atmega328p -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=105 -I%S/Inputs/arduino/avr/cores/arduino -I%S/Inputs/arduino/avr/variants/standard -I%S/Inputs/tools/avr/avr/include --target=avr %S/Inputs/arduino/avr/cores/arduino/CDC.cpp -o %T/CDC.cpp.o

; RUN: clang -c -g -Os -Wall -ffunction-sections -fdata-sections -mmcu=atmega328p -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=105 -I%S/Inputs/arduino/avr/cores/arduino -I%S/Inputs/arduino/avr/variants/standard -I%S/Inputs/tools/avr/avr/include --target=avr %S/Inputs/arduino/avr/cores/arduino/abi.cpp -o %T/abi.cpp.o

; RUN: clang -c -g -Os -Wall -ffunction-sections -fdata-sections -mmcu=atmega328p -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=105 -I%S/Inputs/arduino/avr/cores/arduino -I%S/Inputs/arduino/avr/variants/standard -I%S/Inputs/tools/avr/avr/include --target=avr %S/Inputs/arduino/avr/cores/arduino/HardwareSerial.cpp -o %T/HardwareSerial.cpp.o

; RUN: clang -c -g -Os -Wall -ffunction-sections -fdata-sections -mmcu=atmega328p -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=105 -I%S/Inputs/arduino/avr/cores/arduino -I%S/Inputs/arduino/avr/variants/standard -I%S/Inputs/tools/avr/avr/include --target=avr %S/Inputs/arduino/avr/cores/arduino/HID.cpp -o %T/HID.cpp.o

; RUN: clang -c -g -Os -Wall -ffunction-sections -fdata-sections -mmcu=atmega328p -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=105 -I%S/Inputs/arduino/avr/cores/arduino -I%S/Inputs/arduino/avr/variants/standard -I%S/Inputs/tools/avr/avr/include --target=avr %S/Inputs/arduino/avr/cores/arduino/IPAddress.cpp -o %T/IPAddress.cpp.o

; RUN: clang -c -g -Os -Wall -ffunction-sections -fdata-sections -mmcu=atmega328p -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=105 -I%S/Inputs/arduino/avr/cores/arduino -I%S/Inputs/arduino/avr/variants/standard -I%S/Inputs/tools/avr/avr/include --target=avr %S/Inputs/arduino/avr/cores/arduino/main.cpp -o %T/main.cpp.o

; RUN: clang -c -g -Os -Wall -ffunction-sections -fdata-sections -mmcu=atmega328p -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=105 -I%S/Inputs/arduino/avr/cores/arduino -I%S/Inputs/arduino/avr/variants/standard -I%S/Inputs/tools/avr/avr/include --target=avr %S/Inputs/arduino/avr/cores/arduino/new.cpp -o %T/new.cpp.o

; RUN: clang -c -g -Os -Wall -ffunction-sections -fdata-sections -mmcu=atmega328p -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=105 -I%S/Inputs/arduino/avr/cores/arduino -I%S/Inputs/arduino/avr/variants/standard -I%S/Inputs/tools/avr/avr/include --target=avr %S/Inputs/arduino/avr/cores/arduino/Print.cpp -o %T/Print.cpp.o

; RUN: clang -c -g -Os -Wall -ffunction-sections -fdata-sections -mmcu=atmega328p -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=105 -I%S/Inputs/arduino/avr/cores/arduino -I%S/Inputs/arduino/avr/variants/standard -I%S/Inputs/tools/avr/avr/include --target=avr %S/Inputs/arduino/avr/cores/arduino/Stream.cpp -o %T/Stream.cpp.o

; RUN: clang -c -g -Os -Wall -ffunction-sections -fdata-sections -mmcu=atmega328p -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=105 -I%S/Inputs/arduino/avr/cores/arduino -I%S/Inputs/arduino/avr/variants/standard -I%S/Inputs/tools/avr/avr/include --target=avr %S/Inputs/arduino/avr/cores/arduino/Tone.cpp -o %T/Tone.cpp.o

; RUN: clang -c -g -Os -Wall -ffunction-sections -fdata-sections -mmcu=atmega328p -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=105 -I%S/Inputs/arduino/avr/cores/arduino -I%S/Inputs/arduino/avr/variants/standard -I%S/Inputs/tools/avr/avr/include --target=avr %S/Inputs/arduino/avr/cores/arduino/USBCore.cpp -o %T/USBCore.cpp.o

; RUN: clang -c -g -Os -Wall -ffunction-sections -fdata-sections -mmcu=atmega328p -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=105 -I%S/Inputs/arduino/avr/cores/arduino -I%S/Inputs/arduino/avr/variants/standard -I%S/Inputs/tools/avr/avr/include --target=avr %S/Inputs/arduino/avr/cores/arduino/WMath.cpp -o %T/WMath.cpp.o

; RUN: clang -c -g -Os -Wall -ffunction-sections -fdata-sections -mmcu=atmega328p -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=105 -I%S/Inputs/arduino/avr/cores/arduino -I%S/Inputs/arduino/avr/variants/standard -I%S/Inputs/tools/avr/avr/include --target=avr  %S/Inputs/arduino/avr/cores/arduino/WString.cpp -o %T/WString.cpp.o