# Supported Instructions

Here is a working list of the assembly instructions to be implemented and whether we have a test case to handle each. For those interested in either implementing instructions or writing test cases, refer to [Atmel's AVR Instruction Set](http://www.atmel.com/images/atmel-0856-avr-instruction-set-manual.pdf) for documentation on each instruction.

## Testing your work

LLVM provides the Integrated Tester `llvm-lit` to test your work, and this project makes good use of it. To make sure your new instruction(s) work as expected, create a matching test in 'test/MC/AVR', and run the following from the root of your build directory:

```
bin/llvm-lit ../test/MC/AVR/*
```

This will also run the other existing conformance tests. If any fail, feel free to tackle them and send in a pull request.

## Instruction List

### Key
* `x` means that the instruction is supported and can be compiled to machine code
* `pseudo` means that the instruction is supported, but only in assembly output (no machine code support)
* `not implemented` means that no support exists whatsoever


### Arithmetic and Logic Instructions
```
ADD    x
ADC    x
ADIW   x
SUB    x
SUBI   x
SBC    x
SBCI   x
SBIW   x
AND    x
ANDI   x
OR     x
ORI    x
EOR    x
COM    x
NEG    x
SBR    x
CBR    not implemented
  * (I'm fairly sure we need to add a pseudo instruction and lower into compliment and ANDI)
INC    x
DEC    x
DEC    x
TST    x
CLR    x
SER    x
MUL    x
MULS   x
MULSU  x
FMUL   x
FMULS  x
FMULSU x
DES    not implemented
SBIC   not implemented
SBIS   not implemented
```

### Branch Instructions
```
RJMP    x
IJMP    x
EIJMP   x
JMP     x
RCALL   x
ICALL   x
EICALL  x
CALL    x
RET     x
RETI    x
CPSE    not implemented
CP      x
CPC     x
CPI     x
SBRC    not implemented
SBRS    not implemented
SBIC    not implemented
SBIS    not implemented
BRBS    x
BRBC    x
BREQ    x
BRNE    x
BRCS    x
BRCC    x
BRSH    x
BRLO    x
BRMI    x
BRPL    x
BRGE    x
BRLT    x
BRHS    x
BRHC    x
BRTS    x
BRTC    x
BRVS    x
BRVC    x
BRIE    x
BRID    x
```

### Data Transfer Instructions
```
MOV            x
MOVW           x
LDI            x
LDS            x
LD Rr, P       x
LD Rr, P+      x
LD Rr, -P      x
LDD            x
STS            x
ST Ptr, Rd     x
ST P+, Rr      x
ST -P, Rr      x
STD            x
LPM            x
LPM Rd, Z      x
LPM Rd, Z+     x
ELPM           not implemented
SPM            not implemented
SPM (postinc variant) not implemented
IN             x
OUT            x
PUSH           x
POP            x
XCH            not implemented
LAS            not implemented
LAC            not implemented
LAT            not implemented
```

### Bit and Bit-test instructions
```
LSL    x
LSR    x
ROL    x
ROR    x
ASR    x
SWAP   x
BSET   x
BCLR   x
SBI    x
CBI    x
BST    not implemented
BLD    not implemented
SEC    x
CLC    x
SEN    x
CLN    x
SEZ    x
CLZ    x
SEI    x
CLI    x
SES    x
CLS    x
SEV    x
CLV    x
SET    x
CLT    x
SEH    x
CLH    x
```

### MCU Control Instructions
```
BREAK    x
NOP      x
SLEEP    x
WDR      x
```
