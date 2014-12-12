# Supported Instructions

## Key
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
SBR    not implemented
CBR    not implemented
INC    x
DEC    x
DEC    x
TST    x (untested)
CLR    not implemented
SER    not implemented
MUL    x
MULS   not implemented
MULSU  not implemented
FMUL   not implemented
FMULS  not implemented
FMULSU not implemented
```

### Branch Instructions
```
RJMP    x
IJMP    not implemented
EIJMP   not implemented
JMP     x
RCALL   not implemented
ICALL   x
EICALL  not implemented
CALL    x
RET     x
RETI    x
CPSE    not implemented
CP      x
CPC     x
CPI     not implemented
SBRC    not implemented
SBRS    not implemented
SBIC    not implemented
SBIS    not implemented
BRBS    not implemented
BRBC    not implemented
BREQ    x
BRNE    x
BRCS    not implemented
BRCC    not implemented
BRSH    x
BRLO    x
BRMI    x
BRPL    x
BRGE    x
BRLT    x
BRHS    not implemented
BRHC    not implemented
BRTS    not implemented
BRTC    not implemented
BRVS    not implemented
BRVC    not implemented
BRIE    not implemented
BRID    not implemented
```

### Data Transfer Instructions
```
MOV            x
MOVW           x
LDI            x
LDS            x
LD Rr, P       x
LD Rr, P+      x (untested)
LD Rr, -P      x (untested)
LDD            x
STS            x (untested)
ST Ptr, Rd     x
ST P+, Rr      x (untested)
ST -P, Rr      x (untested)
STD            x
LPM            pseudo
ELPM           not implemented
SPM            not implemented
IN             x
OUT            x
PUSH           x
POP            x
```

### Bit and Bit-test instructions
```
LSL    x
LSR    x
ROL    x
ROR    x
ASR    x
SWAP   x
BSET   not implemented
BCLR   not implemented
SBI    pseudo
CBI    pseudo
BST    not implemented
BLD    not implemented
SEC    not implemented
CLC    not implemented
SEN    not implemented
CLN    not implemented
SEZ    not implemented
CLZ    not implemented
SEI    x
CLI    x
SES    not implemented
CLS    not implemented
SEV    not implemented
CLV    not implemented
SET    not implemented
CLT    not implemented
SEH    not implemented
CLH    not implemented
```

### MCU Control Instructions
```
BREAK    not implemented
NOP      not implemented
SLEEP    not implemented
WDR      not implemented
```
