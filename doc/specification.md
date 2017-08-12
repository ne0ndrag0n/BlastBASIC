GoldScorpion Specification
==========================


# Data Types
## Primitive Data Types

A primitive data type is a combination of numeric type (unsigned (u), signed (s), fixed (fix), float (f)) and bit width. The basic, elementary unit is the 8-bit byte. All enumerations of the primitive data types are below:

| Type     | Prefix | Bit Depth/Tokens  |
|----------|--------|-------------------|
| unsigned | u      | u8, u16, u32, u64 |
| signed   | s      | s8, s16, s32, s64 |
| fixed    | fix    | fix16, fix32      |
| float    | f      | f32, f64          |
| bool     | bool   | bool (special u8) |

## Library Types
These are types available in `Scorpion.Language` that are useful for higher-level programming.
* `Scorpion.Language.String` - String data type, capable of u8 or u16 characters.
