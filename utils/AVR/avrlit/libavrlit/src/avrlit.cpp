//===-- avrlit - AVR LLVM Integrated Tester - AVR Side --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "../avrlit.h"

#include <stdlib.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <util/delay.h>

#include <LUFA/Drivers/USB/USB.h>

#include "usb.h"

namespace avrlit {

struct ostream::USB_ClassInfo_CDC_Device_t : ::USB_ClassInfo_CDC_Device_t {};

ostream::ostream(ostream::USB_ClassInfo_CDC_Device_t * cdc) : cdc_(cdc) {};

void
ostream::print(char const* str) {
  for (char c = *str++; c != '\0'; c = *str++) CDC_Device_SendByte(cdc_, c);
}

void
ostream::print(pstr const& str) {
  char const* s = str.str_;
  for (char c = pgm_read_byte(s++); c != '\0'; c = pgm_read_byte(s++)) {
    CDC_Device_SendByte(cdc_, c);
  }
}

ostream get_cdc_stream() { return ostream((ostream::USB_ClassInfo_CDC_Device_t*)&cdc); }

dec::dec(int n) { itoa(n, buf_, 10); }
dec::dec(unsigned int n) { utoa(n, buf_, 10); }

void
test::ok(bool is_ok, char const* expr, char const* file, unsigned line) {
  ++(is_ok ? passed_tests_ : failed_tests_);
  int count = passed_tests_ + failed_tests_;

  os_ << (is_ok ? P("PASS") : P("FAIL")) << P(": ") << pstr(expr) << P(" (")
      << dec(count) << P(" of ") << dec(planned_tests_) << P(")\n");
  if (not is_ok) {
    pstr delineator(PSTR("********************"));
    os_ << delineator << P(" TEST FAILED ") << delineator << P("\n")
        << pstr(file) << P(":") << dec(line) << P(": ") << pstr(expr) << P("\n")
        << delineator << P("\n");
  }

  do_tasks(true);
}

void
test::plan(unsigned count, char const* name) {
  planned_tests_ = count;
  os_ << P("-- Testing ") << pstr(name) << P(": ") << dec(planned_tests_)
      << P(" tests --\n");
}

bool is_terminal_ready = false;

void
init() {
  MCUSR &= ~(1 << WDRF);
  wdt_disable();

  clock_prescale_set(clock_div_1);

  USB_Init();

  sei();
}

void
do_tasks(bool performRead) {
  if (performRead) CDC_Device_ReceiveByte(&cdc);
  CDC_Device_USBTask(&cdc);
  USB_USBTask();    
}

int16_t
cdc_receive_byte() {
  return CDC_Device_ReceiveByte(&cdc);
}

void
test::summarize() {
  uint16_t total = passed_tests_ + failed_tests_;
  if (planned_tests_ != total) {
    os_ << P("FAIL: ") << dec(planned_tests_) << P(" tests planned but ") 
        << dec(total) << P(" tests executed\n");
  }
  os_ << P("Testing done\n"
           "  Expected Passes    : ") << dec(passed_tests_) << P("\n")
      << P("  Unexpected Failures: ") << dec(failed_tests_) << P("\n");
}

inline
void
process_control_line_change(USB_ClassInfo_CDC_Device_t *const device) {
  // this implements the same reset behaviour as the Arduino Leonardo
  uint16_t control_lines = device->State.ControlLineStates.HostToDevice;
  is_terminal_ready = control_lines & CDC_CONTROL_LINE_OUT_DTR;
  if (not is_terminal_ready and
      device->State.LineEncoding.BaudRateBPS == 1200)
  {
    USB_Disable();
    cli();

    *(uint16_t *)0x0800 = 0x7777;
    wdt_enable(WDTO_120MS);
  } else {
    wdt_disable();
    wdt_reset();
    *(uint16_t *)0x0800 = 0x0;
  }
}

} // end of namespace avrlit

//=== USB Event Hooks =========================================================

void
EVENT_USB_Device_ConfigurationChanged() {
  CDC_Device_ConfigureEndpoints(&cdc);
}

void
EVENT_USB_Device_ControlRequest() {
  CDC_Device_ProcessControlRequest(&cdc);
}

void
EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t *const d) {
  avrlit::process_control_line_change(d);
}

