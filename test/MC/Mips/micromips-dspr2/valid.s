# RUN: llvm-mc %s -triple=mips-unknown-linux -show-encoding -mcpu=mips32r6 -mattr=micromips -mattr=+dspr2 | FileCheck %s

  .set noat
  addqh.ph $3, $4, $5          # CHECK: addqh.ph $3, $4, $5     # encoding: [0x00,0xa4,0x18,0x4d]
  addqh_r.ph $3, $4, $5        # CHECK: addqh_r.ph $3, $4, $5   # encoding: [0x00,0xa4,0x1c,0x4d]
  addqh.w $3, $4, $5           # CHECK: addqh.w $3, $4, $5      # encoding: [0x00,0xa4,0x18,0x8d]
  addqh_r.w $3, $4, $5         # CHECK: addqh_r.w $3, $4, $5    # encoding: [0x00,0xa4,0x1c,0x8d]
  addu.ph $3, $4, $5           # CHECK: addu.ph $3, $4, $5      # encoding: [0x00,0xa4,0x19,0x0d]
  addu_s.ph $3, $4, $5         # CHECK: addu_s.ph $3, $4, $5    # encoding: [0x00,0xa4,0x1d,0x0d]
  adduh.qb $3, $4, $5          # CHECK: adduh.qb $3, $4, $5     # encoding: [0x00,0xa4,0x19,0x4d]
  adduh_r.qb $3, $4, $5        # CHECK: adduh_r.qb $3, $4, $5   # encoding: [0x00,0xa4,0x1d,0x4d]
  absq_s.qb $3, $4             # CHECK: absq_s.qb $3, $4           # encoding: [0x00,0x64,0x01,0x3c]
  dpa.w.ph $ac0, $3, $2        # CHECK: dpa.w.ph $ac0, $3, $2      # encoding: [0x00,0x43,0x00,0xbc]
  dpaqx_s.w.ph $ac3, $12, $7   # CHECK: dpaqx_s.w.ph $ac3, $12, $7 # encoding: [0x00,0xec,0xe2,0xbc]
  dpaqx_sa.w.ph $ac0, $5, $6   # CHECK: dpaqx_sa.w.ph $ac0, $5, $6 # encoding: [0x00,0xc5,0x32,0xbc]
  dpax.w.ph $ac3, $2, $1       # CHECK: dpax.w.ph $ac3, $2, $1     # encoding: [0x00,0x22,0xd0,0xbc]
