# AVR LLVM Integrated Tester

This tool builds an AVR executable from test code lowered by our backend and the
`libavrlit` test suite library using a known good toolchain (avr-gcc). The
resulting binary is uploaded to a development board. Test results are collected
using a virtual tty.

### Setup

Things you will need:

  * ATmega32U4 board with USB and AVR-109 type bootloader. (Arduino Leonardo,
    RedBear Blend (Micro), &c.)
  * [`pySerial`](http://pyserial.sourceforge.net) python module
  * avr-gcc
  * avrdude
  * GNU Make
  * Fire extinguisher

Set the `AVRLIT_PORT` environment variable to the tty path of your board.

```` bash
> export AVRLIT_PORT=/dev/tty.usbmodemfd141
````

If your board currently runs an Arduino sketch that uses the serial port, you are
all set. Otherwise, you need to reset the board manually for the first run. 

```` bash
> bin/llvm-lit -v ../llvm/test/CodeGen/AVR/
```

### Writing Tests

The on-target execution tests reside in `llvm/test/CodeGen/AVR`. Like other lit 
tests they contain a `RUN: ` line calling `llvm-avrlit`:

```` C++
// RUN: llvm-avrlit %s %p/add.ll 

#include <avrlit.h>

using namespace avrlit;

extern "C" {  // actually this is extern "IR" but Bjarne forgot.
  i8 add8_reg_reg(i8, i8);
}

AVRLIT_TEST(llvm_avr) {
  reenter (this) {  // don't worry about the coroutine
    plan(3);
    ok(_(add8_reg_reg( 23, 42) ==  23 + 42)); yield;
    ok(_(add8_reg_reg(-23, 42) == -23 + 42)); yield;
    ok(_(add8_reg_reg(-23, 42) == 0));        yield;
  }
}
````

All of this is still in flux. I'll explain it if I decide to keep it. ;)


