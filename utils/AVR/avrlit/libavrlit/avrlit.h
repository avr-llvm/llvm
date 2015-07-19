//===-- avrlit - AVR LLVM Integrated Tester - AVR Side --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef AVRLIT_H
# define AVRLIT_H

# include <stdint.h>

# include <avr/pgmspace.h>

namespace avrlit {

typedef bool     i1;
typedef uint8_t  i8;
typedef uint16_t i16;
typedef uint32_t i32;
typedef uint64_t i64;

struct pstr {
  explicit pstr(char const* p) : str_(p) {}
  char const* str_;
};

class ostream {
  public:
    struct USB_ClassInfo_CDC_Device_t;
    explicit ostream(USB_ClassInfo_CDC_Device_t * cdc);

    void print(char const* str);
    void print(pstr const& str);
    
  private:
    USB_ClassInfo_CDC_Device_t * cdc_;
};

} // end of namespace avrlit

inline
avrlit::ostream &
operator << (avrlit::ostream & os, char const* str) {
  os.print(str);
  return os;
}

inline
avrlit::ostream &
operator << (avrlit::ostream & os, avrlit::pstr const& str) {
  os.print(str);
  return os;
}

#define P(str) pstr(PSTR(str))

namespace avrlit {

class test {
  public:
    explicit test(ostream const& os) :
      os_(os), planned_tests_(), passed_tests_(), failed_tests_() {}
    ~test() { summarize(); }
    void ok(bool result, char const* expression, char const* file, unsigned line);
    void plan(unsigned count, char const * name = nullptr);
  private:
    void summarize();
    ostream os_;
    unsigned planned_tests_;
    unsigned passed_tests_;
    unsigned failed_tests_;
};

void init();

enum test_suite_state {
  waiting,
  delaying,
  running,
  done
};

extern bool is_terminal_ready;

void do_tasks(bool performRead);

ostream get_cdc_stream();

int16_t cdc_receive_byte();

template <typename F>
inline
int
run(F f, ostream const& os) {
  test t(os);
  f(t);
  return 0;
}

template  <typename... F>
void
run(F... tests) {
  init();
  ostream os(get_cdc_stream());
  test_suite_state state = waiting;
  int16_t delay = 10;
  for (;;) {
    char c = cdc_receive_byte();
    switch (state) {
      case waiting:
        if (is_terminal_ready) {
          delay = 10;
          state = delaying;
        }
        break;
      case delaying: 
        if (--delay == 0) {
          state = running; 
        }
        break;
      case running:
        [](...){ }(run(tests, os)...);
        os << P("--\n");
        state = done;
        break;
      case done:
        if (not is_terminal_ready or c == ' ') state = waiting;
        break;
    }
    do_tasks(false);
  }
}

} // end of namespace avrlit

# define AVRLIT_TEST_SUITE() int main()

# define AVRLIT_TEST_EXPRESSION(x) x, PSTR(#x), PSTR(__FILE__), __LINE__

# define _(x)       AVRLIT_TEST_EXPRESSION(x)

#endif // AVRLIT_H
