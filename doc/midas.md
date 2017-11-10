MIDAS VM
========

The MIDAS (MIDAS IDAS DAS AS S) virtual machine is what the GoldScorpion language is built off.

# Endianness

Big

# Operand Sizes

| Operand Size Code   | Size       | Dot operator mnemonic    |
|---------------------|------------|--------------------------|
| <center>00</center> | No operand | <center>None</center>    |
| <center>01</center> | 8-bit      | <center>8-bit</center>   |
| <center>02</center> | 16-bit     | <center>16-bit</center>  |
| <center>03</center> | 32-bit     | <center>32-bit</center>  |
| <center>04</center> | 64-bit     | <center>64-bit</center>  |

# Table

| Opcode | Mnemonic   | Description                        |
|--------|------------|------------------------------------|
| 00     | NOP        | No-op                              |
| 01     | MOV        | Move                               |
| 02     | ADD        | Add                                |
| 03     | SUB        | Subtract                           |
| 04     | MUL        | Multiply                           |
| 05     | DIV        | Divide                             |
| 06     | ADF        | Floating point add                 |
| 07     | SUF        | Floating point subtract            |
| 08     | MUF        | Floating point multiply            |
| 09     | DIF        | Floating point divide              |
| 0A     | CMP        | Compare Values                     |
| 0B     | JEQ        | Jump if equal                      |
| 0C     | JNE        | Jump if not equal                  |
| 0D     | JGE        | Jump if greater than or equal      |
| 0E     | JLE        | Jump if less than or equal         |
| 0F     | JGT        | Jump if greater than               |
| 10     | JLT        | Jump if less than                  |
| 11     | JMP        | Jump unconditional                 |
| 12     | JSR        | Jump subroutine                    |
| 13     | RTS        | Return from subroutine             |
| 14     | PHI        | Push immediate                     |
| 15     | PLM        | Pull and move to location          |
| 16     | CLL        | Call with *n* stack slots as args  |
| 17     | ASL        | Shift left                         |
| 18     | ASR        | Shift right                        |
| 19     | AND        | Bitwise AND                        |
| 1A     | OR         | Bitwise OR                         |
| 1B     | NOT        | Bitwise NOT                        |
| 1C     | XOR        | Bitwise XOR                        |

# Addressing modes

| Mode                       | Symbol/Example | Preamble |
|----------------------------|----------------|----------|
| No operand                 |                | 00       |
| Location (stack, constant) | 1              | 01       |
| RAM Location               | $03DF          | 02       |
| Pointer (indirect RAM loc) | ($03DF)        | 03       |
| Location pointer           | (1)            | 04       |
| Immediate value            | #1             | 05       |

# Instruction Format with Examples

24-bit instruction format: opcode, operand 1 addressing mode and size, operand 2 addressing mode and size


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
