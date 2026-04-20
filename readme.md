# Irid Programming Language

## Ideas
- [ ] System programming language

- [ ] When element sizes in a record are equal then this record is an array:
```irid
a = ((u32, u32), u64) // implicit [2]([2]u32 or u64)
```

- [ ] Values that could be assign only to certain type. Example: type bool, values: false/true. Need to think syntax/semantics of it.

- [ ] `a = [2]s32` is implicitly `a = s32< and (len : 2)`
```irid
(s32, s32) implicit this -> (this< as s32<) and (len : 2)
```

- [ ] `foo : (a=1) -> a; foo(a:1)`

- [ ] `@s32` defines pointer type to `s32`; `s32@` dereferences pointer

- [ ] Ranges `..`

- [ ] For loop

- [ ] Bits, bytes, align specifiers:
```irid
alignof, align
bitsof, bits
bytesof, bytes
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

- [ ] Define constants and use them at the same time `[$n]s32 (1,2,3,4)` // n=4
- [ ] Type variables:
```irid
// here a and b are the same type T that is subtype of u32
(a = u32 $ T, b = T, c = S) -> (a+b, c)
```

- [ ] Type variables can specify alias:
```irid
// here a and b can alias, but c cannot
foo : (a = @u32 $ T, b = T, c = @u32) -> a@ + b@ + c@
```

- [ ] Implicit casting:
```irid
a = 12 implicit f32
sqrt.f32(a) // auto casts a to f32
```

- [ ] implicit function call:
```irid
entity:(
  object : (position = vec3, health = u32)
  length : 1024
  pool   : [length]object
  get    : (id = handle) -> pool[id]<
  handle : u32 implicit get
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
Vec2 : (x:S32; y:S32)
sum : Vec2 -> Vec2.x + Vec2.y
sum(1, 2) // 3
Vec2 = 3, 4
Vec2.x + Vec.y // 7
```
