MIDAS VM
========

The MIDAS (MIDAS IDAS DAS AS S) virtual machine is what the GoldScorpion language is built off.

# Operand Sizes
* B - Byte (8-bit)
* W - Word (16-bit)
* L - Long (32-bit)
* LL - Long Long (64-bit)

# Endianness

Big

# Table

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
| 14     | PHI        | B,W,L,LL | Push immediate                     |
| 15     | PLM        | B,W,L,LL | Pull and move to location          |
| 16     | CLL        |          | Call with *n* stack slots as args  |
| 17     | ASL        | B,W,L,LL | Shift left                         |
| 18     | ASR        | B,W,L,LL | Shift right                        |
| 19     | AND        | B,W,L,LL | Bitwise AND                        |
| 1A     | OR         | B,W,L,LL | Bitwise OR                         |
| 1B     | NOT        | B,W,L,LL | Bitwise NOT                        |
| 1C     | XOR        | B,W,L,LL | Bitwise XOR                        |

# Addressing modes
| Mode                       | Symbol/Example | Preamble |
|----------------------------|----------------|----------|
| No operand                 |                | 00       |
| Location (stack, constant) | 1              | 01       |
| RAM Location               | $03DF          | 02       |
| Pointer (indirect RAM loc) | ($03DF)        | 03       |
| Location pointer           | (1)            | 04       |
| Immediate value            | #1             | 05       |

# Byte format

The format of an instruction word is variable, depending on the width of the operands. If certain instructions have an implied/inapplicable argument, that part of the instruction word is simply omitted.

## Examples

`NOP`

| Opcode (uint8_t)    |
|---------------------|
| <center>00</center> |


`MOV.W #1, $2200`

| Opcode (uint8_t)    | Operand Width (uint8_t) | Address Mode Operand #1 (uint8_t) | Address Mode Operand #2 (uint8_t) | Operand #1             | Operand #2               |
|---------------------|-------------------------|-----------------------------------|-----------------------------------|------------------------|--------------------------|
| <center>01</center> | <center>01</center>     | <center>05</center>               | <center>02</center>               | <center>00 01</center> | <center>22 00</center>   |

`PLM`

*Note how Operand Width is still required to differentiate this instruction from the following*

| Opcode (uint8_t)    | Operand Width (uint8_t) |
|---------------------|-------------------------|
| <center>15</center> | <center>00</center>     |

`PLM.W $2200`

| Opcode (uint8_t)    | Operand Width (uint8_t) | Address Mode Operand #1 (uint8_t) | Operand #1             |
|---------------------|-------------------------|-----------------------------------|------------------------|
| <center>15</center> | <center>01</center>     | <center>02</center>               | <center>22 00</center> |

# Stack/Heap Variables and Type Identifiers

## Stack data format

The stack is represented as a collection of variable-length *slots*. Slots begin with a byte signifying their type from the Type Table. If the stack preamble is a pointer type, the garbage collector uses this as the beginning point for a mark in the mark-sweep operation.

The stack is incremented and decremented one slot at a time. This means that if you PLM two slots, one being 4 bytes and the other being 1 byte, you will have removed 5 bytes + the 2 control bytes from the stack.

## Heap data format

Heap data is represented as a slab of consecutive allocations. Each allocation has a 16-bit control block. The upper two bits contain whether or not this block is free and whether or not this block is the last allocation in the slab. The remainder of the bits are used to index into the Type Table.

## Default Types
These Type IDs are used in both stack and heap allocations. They are used to determine the size of allocated types, stack or heap.

| Type      | ID | Size (bytes) |
|-----------|----|--------------|
| null      | 0  | 1            |
| i8        | 1  | 1            |
| i16       | 2  | 2            |
| i32       | 3  | 4            |
| i64       | 4  | 8            |
| u8        | 5  | 1            |
| u16       | 6  | 2            |
| u32       | 7  | 4            |
| u64       | 8  | 8            |
| f32       | 9  | 4            |
| f64       | 10 | 8            |

All types above 10 are user-defined types which compose any of the above types into compound types. If a type is seen as 11 or greater in the stack, it is traced as a starting point for garbage collection.
