# Irid Programming Language

## Ideas
- [ ] System programming language

- [ ] When element sizes in a record are equal then this record is an array:
```irid
a = ((U32, U32), U64) // implicit [2]([2]U32 or U64)
```

- [ ] Values that could be assign only to certain type. Example: type bool, values: false/true. Need to think syntax/semantics of it.

- [ ] `a = [2]I32` is implicitly `a = I32< and (len : 2)`
```irid
(I32, I32) implicit this -> (this< as I32<) and (len : 2)
```

- [ ] `foo : (a=1) -> a; foo(a:1)`

- [ ] `@I32` defines pointer type to `I32`; `I32@` dereferences pointer

- [ ] Ranges `..`

- [ ] For loop

- [ ] Bits, bytes, align specifiers:
```irid
alignof, align
bitsof, bits
sizeof, bytes
```

- [ ] Remove char \' syntax, instead:
```irid
char : (str = string) -> {
  str:len is 1
  -> str.0
}
char "a"
```

- [ ] Foreigh functions should be metaprogram instead of #c syntax:
```irid
Init : (foreign:c : "SDL_Init") and (flags = InitFlags) -> b8
```

- [ ] Pointer types can incorporate pointer location:
  ```irid
  a = 10
  b = 20
  c = @a or @b // can only points to a or b
  ```

- [ ] Define constants and use them at the same time `[$n]I32 (1,2,3,4)` // n=4

- [ ] $ syntax for defining templates/variables based on pattern
```irid
foo : (a: $T; b: T) -> {
  return a+b
}
size : [$N]$T -> N * sizeof T
length : [$N]$ -> N;
length(1, 2) // 2
$foo(1, 2)
```

- [ ] Subtypes $:
```irid
// here a and b are the same type T that is subtype of U32
(a = U32 $ T, b = T, c = I32) -> (a+b, c)
a:$T = I32 1
b:T = 2

```

- [ ] Implicit casting:
```irid
a = 12 implicit F32
sqrt.F32(a) // auto casts a to F32
```

- [ ] implicit function call:
```irid
entity:(
  object : (position = Vec3; health = U32)
  length : 1024
  pool   : [length]object
  get    : (id = handle) -> pool[id]@
  handle : U32 implicit get
  new    : object -> handle
  udpate : (e = object) -> {}
)

e = entity.new((1,1,1), 100)
entity.update(e) // update(get(e))
entity.get(e)    // doesn't implicit call since expected type is handle
```

- [ ] static maps based on ad-hoc polymorphism of functions:
```irid
foo : (1 -> 2) and (3 -> 4)
foo(1) // 2
foo(3) // 4
foo(1 or 3) // 2 or 4
```

- [ ] Implicit function parameter. This could be confusing, but some langauges provide implicit context or rely on globals. This could be a nice feature that create thead locals on the stack and passes implicitly to functions. This achieves global-like convinience but is also thread safe.
```irid
parse_int : () -> {
  import parser from stack;
  scan_int(parser.lex);
}

main:() -> {
  parser = Parser
  parse_int()
}
```
There could be a problem with shadowing:
```irid
parse_int : () -> {
  import parser from stack; // should report shadowing error
  scan_int(parser.lex);
}

parse_exp:() -> {
  parser = Parser
  parse_int()
}
main:() -> {
  parser = Parser
  parse_exp()
}
```
Maybe instead of this feature closures should be considered.

- [ ] Narrow a name for that scope
```irid
Vec2 : (x:I32; y:I32)
sum : Vec2 -> Vec2.x + Vec2.y
sum(1, 2) // 3
Vec2 = 3, 4
Vec2.x + Vec.y // 7
```

- [ ] {} syntax for creating records with matching field names. This conflicts with expressoin block syntax.
```irid
x = 1;
y = 2
vec = { x, y } // (x = x; y = y)
```

- [ ] {} pattern syntax for extracting members from a record
```irid
{ y, x } = vec
```
