GoldScorpion Specification
==========================

# Data Types

## Primitive Data Types

Primitives are numeric types that have both a size (in bits) and a numeric format: signed, unsigned, or floating point.
  * `uint` and `int` types may be 8, 16, 32, 64, 128, 256, 512, 1024, and 2048 bits. For bits higher than the host system word size, large-integer arithmetic shall be used.
  * `float` types may be 32 and 64 bits.
  * `bool` types are traditionally 8-bit. Within a `bitfield` class type, they are equivalent to a `uint1`.

## Reference Type

### Standard References
The reference type is used for any `class` or `bitfield` object composed of one or more primitives. For the sake of illustration, `Object` will be used here, but the name of a reference can be any user-defined type.

Scorpion eschews C-style pointers for a more Java-like reference. The reference type internally is referred to by a pointer of the system's word size. To allocate data, simply assign the result of `new` to a reference.

```
Object obj = new Object();
```

Unlike Java, standard references are *NOT* garbage-collected. They are not automatically managed in any way; for automatic memory management, see use the `shared` keyword in the next section. To free the memory, you must delete them when finished using the `delete` keyword, or else the memory will be leaked.

```
delete obj;
```

When a reference is passed to `delete`, *all references to the deleted object are automatically nulled.* This is to prevent dangling pointers and double-free issues which make debugging C a tremendous headache. A standard "weak" reference may turn `null` at any time without reassignment due to use the `delete` operator.

* References do not allow pointer arithmetic. They also do not allow access to invalid or unallocated memory. To write directly to an address in RAM, you must use the `addr` data type, which carries several vital restrictions (see "Address Type").


### Strong References and the "shared" Keyword
Manually managing memory using the method above has the advantage of low overhead. Hwoever, programmers may still demand a form of automatic memory management with a profile suitable for embedded systems. Scorpion also offers strong references, which provide the programmer with a simple form of reference-counted memory management.

A *strong* reference guarantees the existence of the underlying object so long as the reference remains in scope and no reassignment is performed on the reference. For `new` assignments to strong references, the underlying object cannot be deleted manually. Instead, the object will be deleted when the last strong reference to the object falls out of scope.

A strong reference may be created by using the `shared` keyword on a standard reference declaration.
```
shared Object o = new Object();
```

`shared` references have a number of restrictions on their use.
* The `shared` keyword hints to Scorpion to manage a new object's lifecycle for you. This means that, if the result of `new` was originally assigned to a standard reference, the object cannot be reassigned to a `shared` reference. However, the inverse is possible; `shared` references are assignable to non-shared references. These references have the same caveat that they can be turned `null` at any time: in this case, the non-shared references will become `null` after the last `shared` reference to the object falls out of scope.
* `delete` cannot be called on an object that was initially assigned to a `shared` reference. This applies to calling `delete` on both kinds of references; it's the *underlying object* that matters. If the underlying object contains a reference count block, which Scorpion allocates when assigning `new` to a `shared` reference, then it cannot be manually deleted and will throw a `ReferenceOwnershipException`.
* Cyclical `shared` references will not be garbage-collected! `shared` references use a simple form of reference counting which will leak memory in the case of cyclic `shared` dependencies. To avoid this, use a standard reference in one side of the cycle.

## Address Type
Scorpion eschews pointers for a safer form of memory management. Unfortunately, this also means that certain operations essential to embedded systems are otherwise impossible to do without use of the native interface. For instance, Java makes it impossible to read or write specific addresses in memory, at least without questionable use of `sun.misc.Unsafe`.

In Ye Olde Times, languages such as BASIC provided very simple direct-addressing functionality in the form of keywords like `PEEK` and `POKE`. Scoripon adds a modern spin to this concept with the `addr` data type. In the simplest terms, the `addr` datatype is a `uint` type of the target platform's word size, that can be *indrectly addressed*. Literally, `addr` is an "**addr**-ess" and you can read and write data to that address, one word at a time. This is primarily useful to make reads and writes to MMIO ports when programming at the traditionally low level of an embedded system, without having to inline assembly or use unsafe C-style pointers.

To define an addr:
```
addr tmss = 0x00A14000;
```

Similar to pointers in C, the deference operator `*` is used to read and write to the memory location defined by an `addr`. Unlike a C-style pointer, the operation is done one word at a time:
```
*tmss = 0x53454741; // Write 'SEGA' to tmss
uint32 tmssResult = *tmss; // tmssResult = 0x53454741
```

## Arrays
An array may be defined by simply appending `[]` to any type (primitive or reference-based). Arrays may be dynamic *vectors* or statically-defined storage. To define a static array, provide a number when using the `new` operator.
```
uint8[] static = new uint8[ 42 ];
```

Static arrays are aliased to the natively-implemented type `Scorpion.Language.Array` and inherit the properties and methods contained within.

Static arrays cannot be resized, reallocated, pushed to, or removed from. To conduct these operations, you must use the *dynamic* vector type, defined with no number in the brackets.
```
uint8[] dynamic = new uint8[];
```

Dynamic arrays are aliased to the natively-implemented type `Scorpion.Language.Vector` and inherit the properties and methods contained within.
