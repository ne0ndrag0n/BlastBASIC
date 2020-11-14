GoldScorpion Scratch Samples
============================

Version 0.0.1

# Expressions
```
// Expressions
def a as u16

a = ( 5 + 2 )
a = ( 5 + \
    2 )

// Assignment disallowed in if expression
if a == 7 then end

// not is used instead of the bang
if not a == 7 then end
```

# Functions
```
function add( x as u8, y as u8 ) as u16
    return x + y
end

print( add( 2, 2 ) )
```

# Imports
path/to/imported.gs:
```
type Vec2_u8
    x as u8
    y as u8

    function add( rhs as Vec2_u8 ) as Vec2_u8
        def result as Vec2_u8( x + rhs.x, y + rhs.y )
        return result

        // Or you can just do
        return Vec2_u8( x + rhs.x, y + rhs.y )
    end

    function log()
        print( "Vec2_u8 x:" + x + " y:" + y )
    end
end
```
main.gs
```
import path/to/imported

def customType as Vec2_u8( 2, 2 )
customType.add( Vec2_u8( 2, 2 ) )
customType.log()
```

# Strings
Strings, for simplicity, are set to a fixed static length using an angle bracket annotation.
```
def fullyDynamicString as string        // Compiler error!
                                        // Strings are always static and fixed-length
```

```
def staticString as string<16>

function concat( x as string<16> byref, y as string<16> byref ) as string<32>
    return x + y
end

print( "Concat: " + "fff" + "aaa" ) // Strings that don't fit will still terminate
                                    // But the memory will always be used
```

If a string doesn't fit, it will be cut off.

# Arrays
Arrays are straightforward enough.
```
def x as u8[ 16 ]

x = 6                   // Compiler error, cannot assign number to array type
x[ 0 ] = 6              // Better!
```

# Inline Assembly
Dropping to assembly is as easy as using the `asm` block directive.
```
function add_inline( x as u8, y as u8 ) as u16
    def result as u16
    result = 42

    asm
        move.w  6(sp), d0
        add.w   8(sp), d0
        move.w  d0, (sp)
    end

    return result
end
```

Base-level types:
`u8`, `u16`, `u32`
`i8`, `i16`, `i32`
`string`