MIDAS VM
========

The MIDAS (MIDAS IDAS DAS AS S) virtual machine is what the GoldScorpion language is built off.

# Operand Sizes
* B - Byte (8-bit)
* W - Word (16-bit)
* L - Long (32-bit)
* LL - Long Long (64-bit)

# Table

Format: `MOV.B #1, $2200`

| Opcode | Mnemonic   | Sizes    | Description                        |
|--------|------------|----------|------------------------------------|
| 00     | NOP        |          | No-op                              |
| 01     | MOV        | B,W,L,LL | Move                               |
| 02     | ADD        | B,W,L,LL | Add                                |
| 03     | SUB        | B,W,L,LL | Subtract                           |
| 04     | MUL        | B,W,L,LL | Multiply                           |
| 05     | DIV        | B,W,L,LL | Divide                             |
| 06     | ADF        | L,LL     | Floating point add                 |
| 07     | SUF        | L,LL     | Floating point subtract            |
| 08     | MUF        | L,LL     | Floating point multiply            |
| 09     | DIF        | L,LL     | Floating point divide              |
| 0A     | CMP        | B,W,L,LL | Compare Values                     |
| 0B     | JEQ        | Address  | Jump if equal                      |
| 0C     | JNE        | Address  | Jump if not equal                  |
| 0D     | JGE        | Address  | Jump if greater than or equal      |
| 0E     | JLE        | Address  | Jump if less than or equal         |
| 0F     | JGT        | Address  | Jump if greater than               |
| 10     | JLT        | Address  | Jump if less than                  |
| 11     | JMP        | Address  | Jump unconditional                 |
| 12     | JSR        | Address  | Jump subroutine                    |
| 13     | RTS        |          | Return from subroutine             |

# Addressing modes
| Mode                       | Symbol/Example | Preamble |
|----------------------------|----------------|----------|
| Location (stack, constant) | 1              | 00       |
| RAM Location               | $03DF          | 01       |
| Pointer (indirect RAM loc) | ($03DF)        | 02       |
| Location pointer           | (1)            | 03       |
| Immediate value            | #1             | 04       |
