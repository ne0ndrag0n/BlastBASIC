GoldScorpion Scratch Samples
============================

# Expressions
```
// Expressions
def a as integer

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


Base-level types:
`u8`, `u16`, `u32`
`i8`, `i16`, `i32`
`real`
`bool`
`string`
