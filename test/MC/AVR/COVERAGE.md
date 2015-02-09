# Coverage table of MC unit tests for AVR instructions

** There should be at least two tests for each instruction to be considered tested **

** Also note that just because a test exists, does not mean it passes **

Arithmetic and Logic Instructions

Mnemonic | Operands  | Description                                     | Has MC Unit Test
:--------|:----------|:------------------------------------------------|------------------------------------
`ADD`    | `Rd, Rr`  | Add without Carry                               | Yes
`ADC`    | `Rd, Rr`  | Add with Carry                                  | Yes
`ADIW`   | `Rd, K`   | Add Immediate to Word                           | Yes
`SUB`    | `Rd, Rr`  | Subtract without Carry                          | Yes
`SUBI`   | `Rd, K`   | Subtract Immediate                              | Yes
`SBC `   | `Rd, Rr`  | Subtract with Carry                             | Yes
`SBCI`   | `Rd, K`   | Subtract Immediate with Carry                   | Yes
`SBIW`   | `Rd, K`   | Subtract Immediate from Word                    | Yes
`AND`    | `Rd, Rr`  | Logical AND                                     | Yes
`ANDI`   | `Rd, K`   | Logical AND with Immediate                      | Yes
`OR`     | `Rd, Rr`  | Logical OR                                      | Yes
`ORI`    | `Rd, K`   | Logical OR with Immediate                       | Yes
`EOR`    | `Rd, Rr`  | Exclusive OR                                    | Yes
`COM`    | `Rd`      | One's Complement                                | Yes
`NEG`    | `Rd`      | Two's Complement                                | Yes
`SBR`    | `Rd,K`    | Set Bit(s) in Register                          | Yes 
`CBR`    | `Rd,K`    | Clear Bit(s) in Register                        | Yes 
`INC`    | `Rd`      | Increment                                       | Yes
`DEC`    | `Rd`      | Decrement                                       | Yes
`TST`    | `Rd`      | Test for Zero or Minus                          | Yes 
`CLR`    | `Rd`      | Clear Register                                  | Yes 
`SER`    | `Rd`      | Set Register                                    | Yes 
`MUL`    | `Rd,Rr`   | Multiply Unsigned                               | Yes
`MULS`   | `Rd,Rr`   | Multiply Signed                                 | Yes
`MULSU`  | `Rd,Rr`   | Multiply Signed with Unsigned                   | Yes
`FMUL`   | `Rd,Rr`   | Fractional Multiply Unsigned                    | Yes
`FMULS`  | `Rd,Rr`   | Fractional Multiply Signed                      | Yes
`FMULSU` | `Rd,Rr`   | Fractional Multiply Signed with Unsigned        | Yes

Branch Instructions

Mnemonic | Operands  | Description                                     | Has MC Unit Test
:--------|:----------|:------------------------------------------------|------------------
`RJMP`   | `k`       | Relative Jump                                   | Yes 
`IJMP`   |           | Indirect Jump to (Z)                            | Yes 
`EIJMP`  |           | Extended Indirect Jump to (Z)                   | Yes 
`JMP`    | `k`       | Jump                                            | Yes
`RCALL`  | `k`       | Relative Call Subroutine                        | Yes
`ICALL`  |           | Indirect Call to (Z)                            | Yes 
`EICALL` |           | Extended Indirect Call to (Z)                   | Yes 
`CALL`   | `k`       | Call Subroutine                                 | Yes 
`RET`    |           | Subroutine Return                               | Yes 
`RETI`   |           | Interrupt Return                                | Yes 
`CPSE`   | `Rd,Rr`   | Compare, Skip if Equal                          | Yes 
`CP`     | `Rd,Rr`   | Compare                                         | Yes 
`CPC`    | `Rd,Rr`   | Compare with Carry                              | Yes 
`CPI`    | `Rd,K `   | Compare with Immediate                          | Yes 
`SBRC`   | `Rr, b`   | Skip if Bit in Register Cleared                 | **No**
`SBRS`   | `Rr, b`   | Skip if Bit in Register Set                     | **No**
`BRBS`   | `s, k`    | Branch if Status Flag Set                       | **No**
`BRBC`   | `s, k`    | Branch if Status Flag Cleared                   | **No**
`BREQ`   | `k`       | Branch if Equal                                 | Yes 
`BRNE`   | `k`       | Branch if Not Equal                             | Yes
`BRCS`   | `k`       | Branch if Carry Set                             | Yes
`BRCC`   | `k`       | Branch if Carry Cleared                         | Yes
`BRSH`   | `k`       | Branch if Same or Higher                        | Yes
`BRLO`   | `k`       | Branch if Lower                                 | Yes
`BRMI`   | `k`       | Branch if Minus                                 | Yes
`BRPL`   | `k`       | Branch if Plus                                  | Yes
`BRGE`   | `k`       | Branch if Greater or Equal, Signed              | Yes
`BRLT`   | `k`       | Branch if Less Than, Signed                     | Yes
`BRHS`   | `k`       | Branch if Half Carry Flag Set                   | Yes
`BRHC`   | `k`       | Branch if Half Carry Flag Cleared               | Yes
`BRTS`   | `k`       | Branch if T Flag Set                            | Yes
`BRTC`   | `k`       | Branch if T Flag Cleared                        | Yes
`BRVS`   | `k`       | Branch if Overflow Flag is Set                  | Yes
`BRVC`   | `k`       | Branch if Overflow Flag is Cleared              | Yes
`BRIE`   | `k`       | Branch if Interrupt Enabled                     | Yes
`BRID`   | `k`       | Branch if Interrupt Disabled                    | Yes

Data Transfer Instructions

Mnemonic | Operands  | Description                                     | Has MC Unit Test
:--------|:----------|:------------------------------------------------|------------------
`MOV`    | `Rd, Rr`  | Copy Register                                   | **No**
`MOVW`   | `Rd, Rr`  | Copy Register Pair                              | **No**
`LDI`    | `Rd, K`   | Load Immediate                                  | **No**
`LDS`    | `Rd, k`   | Load Direct from data space                     | **No**
`LD`     | `Rd, X`   | Load Indirect                                   | Yes
`LD`     | `Rd, X+`  | Load Indirect and Post-Increment                | **No**
`LD`     | `Rd, -X`  | Load Indirect and Pre-Decrement                 | **No**
`LD`     | `Rd, Y`   | Load Indirect                                   | Yes
`LD`     | `Rd, Y+`  | Load Indirect and Post-Increment                | **No**
`LD`     | `Rd, -Y`  | Load Indirect and Pre-Decrement                 | **No**
`LDD`    | `Rd,Y+q`  | Load Indirect with Displacement                 | **No**
`LD`     | `Rd, Z`   | Load Indirect                                   | Yes
`LD`     | `Rd, Z+`  | Load Indirect and Post-Increment                | **No**
`LD`     | `Rd, -Z`  | Load Indirect and Pre-Decrement                 | **No**
`LDD`    | `Rd, Z+q` | Load Indirect with Displacement                 | **No**
`STS`    | `k, Rr`   | Store Direct to data space                      | **No**
`ST`     | `X, Rr`   | Store Indirect                                  | Yes
`ST`     | `X+, Rr`  | Store Indirect and Post-Increment               | **No**
`ST`     | `-X, Rr`  | Store Indirect and Pre-Decrement                | **No**
`ST`     | `Y, Rr`   | Store Indirect                                  | Yes
`ST`     | `Y+, Rr`  | Store Indirect and Post-Increment               | **No**
`ST`     | `-Y, Rr`  | Store Indirect and Pre-Decrement                | **No**
`STD`    | `Y+q,Rr`  | Store Indirect with Displacement                | **No**
`ST`     | `Z, Rr`   | Store Indirect                                  | Yes
`ST`     | `Z+, Rr`  | Store Indirect and Post-Increment               | **No**
`ST`     | `-Z, Rr`  | Store Indirect and Pre-Decrement                | **No**
`STD`    | `Z+q,Rr`  | Store Indirect with Displacement                | **No**
`LPM`    |           | Load Program Memory                             | **No**
`LPM`    | `Rd, Z`   | Load Program Memory                             | **No**
`LPM`    | `Rd, Z+`  | Load Program Memory and Post-Increment          | **No**
`ELPM`   |           | Extended Load Program Memory                    | **No**
`ELPM`   | `Rd, Z`   | Extended Load Program Memory                    | **No**
`ELPM`   | `Rd, Z+`  | Extended Load Program Memory and Post-Increment | **No**
`SPM`    |           | Store Program Memory                            | **No**
`IN`     | `Rd, A`   | In From I/O Location                            | **No**
`OUT`    | `A, Rr`   | Out To I/O Location                             | **No**
`PUSH`   | `Rr`      | Push Register on Stack                          | **No**
`POP`    | `Rd`      | Pop Register from Stack                         | **No**

Bit and Bit-test Instructions

Mnemonic | Operands  | Description                                     | Has MC Unit Test
:--------|:----------|:------------------------------------------------|------------------
`LSL`    | `Rd`      |  Logical Shift Left                             | **No**
`LSR`    | `Rd`      | Logical Shift Right                             | **No**
`ROL`    | `Rd`      | Rotate Left Through Carry                       | **No**
`ROR`    | `Rd`      | Rotate Right Through Carry                      | **No**
`ASR`    | `Rd`      | Arithmetic Shift Right                          | **No**
`SWAP`   | `Rd`      | Swap Nibbles                                    | **No**
`BSET`   | `s`       | Flag Set                                        | **No**
`BCLR`   | `s`       | Flag Clear                                      | **No**
`SBI`    | `A, b`    | Set Bit in I/O Register                         | **No**
`CBI`    | `A, b`    | Clear Bit in I/O Register                       | **No**
`BST`    | `Rr, b`   | Bit Store from Register to T                    | **No**
`BLD`    | `Rd, b`   | Bit load from T to Register                     | **No**
`SEC`    |           | Set Carry                                       | **No**
`CLC`    |           | Clear Carry                                     | **No**
`SEN`    |           | Set Negative Flag                               | **No**
`CLN`    |           | Clear Negative Flag                             | **No**
`SEZ`    |           | Set Zero Flag                                   | **No**
`CLZ`    |           | Clear Zero Flag                                 | **No**
`SEI`    |           | Global Interrupt Enable                         | **No**
`CLI`    |           | Global Interrupt Disable                        | **No**
`SES`    |           | Set Signed Test Flag                            | **No**
`CLS`    |           | Clear Signed Test Flag                          | **No**
`SEV`    |           | Set Two's Complement Overflow                   | **No**
`CLV`    |           | Clear Two's Complement Overflow                 | **No**
`SET`    |           | Set T in SREG                                   | **No**
`CLT`    |           | Clear T in SREG                                 | **No**
`SEH`    |           | Set Half Carry Flag in SREG                     | **No**
`CLH`    |           | Clear Half Carry Flag in SREG                   | **No**

MCU Control Instructions

Mnemonic | Operands  | Description                                     | Has MC Unit Test
:--------|:----------|:------------------------------------------------|------------------
`BREAK`  |           | Break                                           | Yes
`NOP`    |           | No Operation                                    | Yes
`SLEEP`  |           | Sleep                                           | Yes
`WDR`    |           | Watchdog Reset                                  | Yes
