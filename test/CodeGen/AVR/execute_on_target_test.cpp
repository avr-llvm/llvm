// RUN: llvm-avrlit %s %p/add.ll %p/and.ll %p/xor.ll %p/call.ll %p/div.ll %p/rem.ll

#include <avrlit.h>

using namespace avrlit;

AVRLIT_TEST_SUITE();

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
  u8  and8_reg_reg(u8, u8);
  u8  and8_reg_imm(u8);
  u16 and16_reg_reg(u16, u16);
  u16 and16_reg_imm(u16);
  u32 and32_reg_reg(u32, u32);
  u32 and32_reg_imm(u32);
  u64 and64_reg_reg(u64, u64);
  u64 and64_reg_imm(u64);
}

void test_and(test & t) {
  t.plan(16, PSTR("AND"));

  t.ok(_(and8_reg_reg(23, 42) == (23 & 42)));
  t.ok(_(and8_reg_reg(0x0f, 0x2a) == (0x0f & 0x2a)));

  t.ok(_(and8_reg_imm(23) == (23 & 5)));
  t.ok(_(and8_reg_imm(0x0f) == (0x0f & 5)));

  t.ok(_(and16_reg_reg(2323, 4242) == (2323 & 4242)));
  t.ok(_(and16_reg_reg(0xf0f0, 0x2aaa) == (0xf0f0 & 0x2aaa)));

  t.ok(_(and16_reg_imm(2342) == (2342 & 1234)));
  t.ok(_(and16_reg_imm(0xf0f0) == (0xf0f0 & 1234)));

  t.ok(_(and32_reg_reg(0xffaaffaa, 0x55555555) == (0xffaaffaa & 0x55555555)));
  t.ok(_(and32_reg_reg(0x0f, 0x55555555) == (0x0f & 0x55555555)));

  t.ok(_(and32_reg_imm(0x55555555) == (0x55555555 & 123456789)));
  t.ok(_(and32_reg_imm(0xaaaaaaaa) == (0xaaaaaaaa & 123456789)));

  t.ok(_(and64_reg_reg(0xffaaffaaffaaffaa, 0x5555555555555555) == 
                      (0xffaaffaaffaaffaa & 0x5555555555555555)));
  t.ok(_(and64_reg_reg(0xff00000000000000,  0x5555555555555555) ==
                      (0xff00000000000000 & 0x5555555555555555)));

  t.ok(_(and64_reg_imm(0xffaaffaaffaaffaa) == 
                      (0xffaaffaaffaaffaa & 17446744073709551613u)));
  t.ok(_(and64_reg_imm(0x5555555555555555) ==
                      (0x5555555555555555 & 17446744073709551613u)));

}

//=== call.ll =================================================================

namespace {
  i8  i8_args[11];
  i16 i16_args[11];
  i32 i32_args[5];
  i64 i64_args[3];
}

extern "C" {
  // callees: decalred in call.ll, implemented here
  i8 foo8_1(i8 a) { i8_args[0] = a; return a; }
  i8 foo8_2(i8 a, i8 b, i8 c) {
    i8_args[1] = a; i8_args[2] = b; i8_args[3] = c;
    return c;
  }

  i8 foo8_3(i8 a, i8 b, i8 c, i8 d, i8 e, i8 f, i8 g, i8 h, i8 i, i8 j, i8 k) {
    i8_args[0] = a; i8_args[1] = b; i8_args[2] = c; i8_args[3] = d;
    i8_args[4] = e; i8_args[5] = f; i8_args[6] = g; i8_args[7] = h;
    i8_args[8] = i; i8_args[9] = j; i8_args[10] = k;
    return k;
  }

  i16 foo16_1(i16 a, i16 b) {
    i16_args[0] = a; i16_args[1] = b;
    return b;
  }
  i16 foo16_2(i16 a, i16 b, i16 c, i16 d, i16 e, i16 f, i16 g, i16 h, i16 i,
              i16 j, i16 k)
  {
    i16_args[0] = a; i16_args[1] = b; i16_args[2] = c; i16_args[3] = d;
    i16_args[4] = e; i16_args[5] = f; i16_args[6] = g; i16_args[7] = h;
    i16_args[8] = i; i16_args[9] = j; i16_args[10] = k;
    return k;
  }

  i32 foo32_1(i32 a, i32 b) { i32_args[0] = a; i32_args[1] = b; return b; }
  i32 foo32_2(i32 a, i32 b, i32 c, i32 d, i32 e) {
    i32_args[0] = a; i32_args[1] = b; i32_args[2] = c; i32_args[3] = d;
    i32_args[4] = e;
    return e;
  }

  i64 foo64_1(i64 a) { i64_args[0] = a; return a; }
  i64 foo64_2(i64 a, i64 b, i64 c) {
    i64_args[0] = a; i64_args[1] = b; i64_args[2] = c;
    return c;
  }

  void foo64_3(i64, i64, i64, i8, i16*) {}
  i32 foofloat(float a) { return static_cast<i32>(a); }

  // subjects: implemented in call.ll
  i8  calli8_reg();
  i8  calli8_stack();

  i16 calli16_reg();
  i16 calli16_stack();

  i32 calli32_reg();
  i32 calli32_stack();

  i64 calli64_reg();
  i64 calli64_stack();
}

void test_call(test & t) {
  t.plan(47, PSTR("CALL"));

  t.ok(_(calli8_reg() ==  14));
  t.ok(_(i8_args[0] == 12));
  t.ok(_(i8_args[1] == 12));
  t.ok(_(i8_args[2] == 13));
  t.ok(_(i8_args[3] == 14));

  t.ok(_(calli8_stack() ==  11));
  t.ok(_(i8_args[0] == 1));
  t.ok(_(i8_args[1] == 2));
  t.ok(_(i8_args[2] == 3));
  t.ok(_(i8_args[3] == 4));
  t.ok(_(i8_args[4] == 5));
  t.ok(_(i8_args[5] == 6));
  t.ok(_(i8_args[6] == 7));
  t.ok(_(i8_args[7] == 8));
  t.ok(_(i8_args[8] == 9));
  t.ok(_(i8_args[9] == 10));
  t.ok(_(i8_args[10] == 11));

  t.ok(_(calli16_reg() ==  514));
  t.ok(_(i16_args[0] == 513));
  t.ok(_(i16_args[1] == 514));

  t.ok(_(calli16_stack() ==  522));
  t.ok(_(i16_args[0] == 512));
  t.ok(_(i16_args[1] == 513));
  t.ok(_(i16_args[2] == 514));
  t.ok(_(i16_args[3] == 515));
  t.ok(_(i16_args[4] == 516));
  t.ok(_(i16_args[5] == 517));
  t.ok(_(i16_args[6] == 518));
  t.ok(_(i16_args[7] == 519));
  t.ok(_(i16_args[8] == 520));
  t.ok(_(i16_args[9] == 521));
  t.ok(_(i16_args[10] == 522));

  t.ok(_(calli32_reg() ==  35554432));
  t.ok(_(i32_args[0] == 34554432));
  t.ok(_(i32_args[1] == 35554432));

  t.ok(_(calli32_stack() ==  34554432));
  t.ok(_(i32_args[0] == 1));
  t.ok(_(i32_args[1] == 2));
  t.ok(_(i32_args[2] == 3));
  t.ok(_(i32_args[3] == 4));
  t.ok(_(i32_args[4] == 34554432));

  t.ok(_(calli64_reg() == 17446744073709551615u));
  t.ok(_(i64_args[0] == 17446744073709551615u));

  t.ok(_(calli64_stack() == 17446744073709551615u));
  t.ok(_(i64_args[0] == 1));
  t.ok(_(i64_args[1] == 2));
  t.ok(_(i64_args[2] == 17446744073709551615u));

}

//=== div.ll ==================================================================

extern "C" {
  u8 udiv8(u8, u8);
  i8 sdiv8(i8, i8);
  u16 udiv16(u16, u16);
  i16 sdiv16(i16, i16);
  u32 udiv32(u32, u32);
  i32 sdiv32(i32, i32);
}

void test_div(test & t) {
  t.plan(17, PSTR("DIV"));

  t.ok(_(udiv8(150, 3) == 150 / 3));
  t.ok(_(udiv8(200, 4) == 200 / 4));
  t.ok(_(udiv8(200, 3) == 200 / 3));

  t.ok(_(sdiv8(90, 3) == 90 / 3));
  t.ok(_(sdiv8(60, 3) == 60 / 3));
  t.ok(_(sdiv8(-66, 7) == -66 / 7));

  t.ok(_(udiv16(300, 3) == 300 / 3));
  t.ok(_(udiv16(400, 3) == 400 / 3));
  t.ok(_(udiv16(4000, 3) == 4000u / 3));
  t.ok(_(udiv16(34567, 3) == 34567u / 3));
  t.ok(_(udiv16(0xbfff, 2) == 0xbfffu / 2));

  t.ok(_(sdiv16(4321, 1234) == 4321 / 1234));
  t.ok(_(sdiv16(-4321, 1234) ==  -4321 / 1234));

  t.ok(_(udiv32(0x12345678, 0x87654321) == 0x12345678 / 0x87654321));
  t.ok(_(udiv32(15923, 3) == 15923 / 3));

  t.ok(_(sdiv32(-135333, -12345) == -135333 / -12345));
  t.ok(_(sdiv32(-1, 1) == -1 / 1));
}

//=== rem.ll ==================================================================

extern "C" {
  u8 urem8(u8, u8);
  i8 srem8(i8, i8);
  u16 urem16(u16, u16);
  i16 srem16(i16, i16);
  u32 urem32(u32, u32);
  i32 srem32(i32, i32);
}

void test_rem(test & t) {
  t.plan(21, PSTR("REM"));

  t.ok(_(urem8(123, 5) == 123 % 5));
  t.ok(_(urem8(15, 5) == 15 % 5));
  t.ok(_(urem8(123, 124) == 123 % 124));
  t.ok(_(urem8(0, 15) == 0 % 15));
  t.ok(_(urem8(255, 1) == 255 % 1));
  t.ok(_(urem8(255, 15) == 255 % 15));

  t.ok(_(srem8(-123, 15) == -123 % 15));
  t.ok(_(srem8(23, -3) == 23 % -3));
  t.ok(_(srem8(127, 3) == 127 % 3));
  t.ok(_(srem8(23, 3) == 23 % 3));

  t.ok(_(urem16(23, 3) == 23 % 3));
  t.ok(_(urem16(1234, 123) == 1234 % 123));
  t.ok(_(urem16(12345, 123) == 12345 % 123));
  t.ok(_(urem16(0xffff, 123) == 0xffff % 123));

  t.ok(_(srem16(64, 3) == 64 % 3));
  t.ok(_(srem16(-1234, -23) == -1234 % -23));
  t.ok(_(srem16(-12345, 2345) == -12345 % 2345));

  t.ok(_(urem32(1357910, 8) == 1357910 % 8));
  t.ok(_(urem32(0xffff0000, 0x0000ffff) == 0xffff0000 % 0x0000ffff));

  t.ok(_(srem32(-1234555, -4) == -1234555 % -4));
  t.ok(_(srem32(-1, -1) == -1 % -1));
}

//=== xor.ll ==================================================================

extern "C" {
  u8  xor8_reg_reg(u8, u8);
  u16 xor16_reg_reg(u16, u16);
  u32 xor32_reg_reg(u32, u32);
  u64 xor64_reg_reg(u64, u64);
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
int main() {
  run(test_add, test_and, test_call, test_div, test_rem, test_xor);
}

