// RUN: llvm-avrlit %s %p/add.ll %p/and.ll %p/xor.ll

#include <avrlit.h>

using namespace avrlit;

//=== add.ll ==================================================================
extern "C" {
  i8  add8_reg_reg(i8, i8);
  i8  add8_reg_imm(i8);
  i8  add8_reg_increment(i8);
  i16 add16_reg_reg(i16, i16);
  i16 add16_reg_imm(i16);
  i16 add16_reg_imm_subi(i16);
  i32 add32_reg_reg(i32, i32);
  i32 add32_reg_imm(i32);
  i64 add64_reg_reg(i64, i64);
  i64 add64_reg_imm(i64);
}

void test_add(test & t) {
  t.plan(20, PSTR("ADD"));

  t.ok(_(add8_reg_reg( 23, 42) ==  23 + 42));
  t.ok(_(add8_reg_reg(-23, 42) == -23 + 42));

  t.ok(_(add8_reg_imm(42) == 42 + 5));
  t.ok(_(add8_reg_imm(23) == 23 + 5));

  t.ok(_(add8_reg_increment(42) == 42 + 1));
  t.ok(_(add8_reg_increment(23) == 23 + 1));

  t.ok(_(add16_reg_reg(1234, 4321) == 1234 + 4321));
  t.ok(_(add16_reg_reg(5678, 8765) == 5678 + 8765));

  t.ok(_(add16_reg_imm(1234) == 1234 + 63));
  t.ok(_(add16_reg_imm(8765) == 8765 + 63));

  t.ok(_(add16_reg_imm_subi(1234) == 1234 + 123));
  t.ok(_(add16_reg_imm_subi(8765) == 8765 + 123));

  t.ok(_(add32_reg_reg(123456, 987654) == 123456 + 987654));
  t.ok(_(add32_reg_reg(3, 5) == 3 + 5));

  t.ok(_(add32_reg_imm(987654) == 987654 + 5));
  t.ok(_(add32_reg_imm(5) == 5 + 5));

  t.ok(_(add64_reg_reg(9876543210,  9988776655) == 
                       9876543210 + 9988776655));
  t.ok(_(add64_reg_reg(198, 126) == 198 +126));

  t.ok(_(add64_reg_imm(9876543210) == 9876543210 + 5));
  t.ok(_(add64_reg_imm(198) == 198 + 5));

}

//=== and.ll ==================================================================
extern "C" {
  i8  and8_reg_reg(i8, i8);
  i8  and8_reg_imm(i8);
  i16 and16_reg_reg(i16, i16);
  i16 and16_reg_imm(i16);
}

void test_and(test & t) {
  t.plan(8, PSTR("AND"));

  t.ok(_(and8_reg_reg(23, 42) ==  (23 & 42)));
  t.ok(_(and8_reg_reg(0xef, 0x2a) ==  (0xef & 0x2a)));

  t.ok(_(and8_reg_imm(23) ==  (23 & 5)));
  t.ok(_(and8_reg_imm(0xef) ==  (0xef & 5)));

  t.ok(_(and16_reg_reg(2323, 4242) ==  (2323 & 4242)));
  t.ok(_(and16_reg_reg(0xefff, 0x2aaa) ==  (0xefff & 0x2aaa)));

  t.ok(_(and16_reg_imm(2342) ==  (2342 & 1234)));
  t.ok(_(and16_reg_imm(0xefff) ==  (0xefff & 1234)));
}

//=== xor.ll ==================================================================
extern "C" {
  i8  xor8_reg_reg(i8, i8);
  i16 xor16_reg_reg(i16, i16);
  i32 xor32_reg_reg(i32, i32);
  i64 xor64_reg_reg(i64, i64);
}

void test_xor(test & t) {
  t.plan(8, PSTR("XOR"));

  t.ok(_(xor8_reg_reg(23, 42) ==  23 ^ 42));
  t.ok(_(xor8_reg_reg(0xff, 0xaa) ==  0xff ^ 0xaa));

  t.ok(_(xor16_reg_reg(0xffff, 0xaaaa) ==  0xffff ^ 0xaaaa));
  t.ok(_(xor16_reg_reg(0x5555, 0xaaaa) ==  0x5555 ^ 0xaaaa));

  t.ok(_(xor32_reg_reg(0xffffffff,  0xaaaaaaaa) == 
                       0xffffffff ^ 0xaaaaaaaa));
  t.ok(_(xor32_reg_reg(0x55555555,  0xaaaaaaaa) ==  
                       0x55555555 ^ 0xaaaaaaaa));

  t.ok(_(xor64_reg_reg(0xffffffffffffffff,  0xaaaaaaaaaaaaaaaa) == 
                       0xffffffffffffffff ^ 0xaaaaaaaaaaaaaaaa));
  t.ok(_(xor64_reg_reg(0x5555555555555555,  0xaaaaaaaaaaaaaaaa) ==  
                       0x5555555555555555 ^ 0xaaaaaaaaaaaaaaaa));
}

//=== Test Suite ==============================================================

AVRLIT_TEST_SUITE() {
  run(test_add, test_and, test_xor);
}

