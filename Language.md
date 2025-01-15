# The rbc language

## Goals

- Manual Memory Management
- Rich Typesystem (ocaml like enums)
- Non Painfull error handling -> the typesystem should take care of that
    - No error propagation, it should be almost the same work to propagate an error or handle it correctly
- Almost everything is a expression
- Batteries included standard library
    - Reduce the need for any external usecases
    - Provide common bindings to different libraries
- Iterators
- Allocators
- Shy away from complexity, prever simplicity and clearaty


## Language Features

### Literals

`"a basic string"`  a string literal

`'d'`               a char, represents a utf-8 codepoint, is by default a rune (a.k.a. u32), but can also be converted to a sequence of u8

`69`                a integer literal, integer literal have by default the type of i32,
                    but will be converted to the expected type in a expression, will error if not representable and will require a cast

`69.0`              same as a integer literal but instead of a i32, it is a f32

`true`              a boolean literal with the value of true
`false`             a boolean literal with the value of false


### Types

#### Base Integer types

i8, i16, i32, i64
u8, u16, u32, u64
rune - a alias for u32, can contain any valid utf-8 codepoint

#### Slices



, strings are implicitly utf-8 and are a sequence of bytes
### Functions

```rbc
// A function with no parameters and return type is declared like this:
fn main() = {};

// This function returns a i32
fn main() i32 = 69;

// The return type can also be deduced
fn main() = 69; // The function has a type of "fn () i32"
```
