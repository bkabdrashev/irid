# Irid Programming Language

## Ideas
- [ ] System programming language

- [ ] When element sizes in a record are equal then this record is an array:
```irid
a = ((U32, U32), U64) // implicit [2]([2]\U32\U64)
```

- [ ] Values that could be assign only to certain type. Example: type bool, values: false/true. Need to think syntax/semantics of it.

- [ ] `a = [2]I32` is implicitly `a = @I32 and (len : 2)`
```irid
(I32, I32) implicit this -> (@this as @I32) and (len : 2)
```

- [ ] `foo : (a=1) -> a; foo(a:1)`

- [x] `@I32` defines pointer type to `I32`; `I32@` dereferences pointer

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
c = @a\@b // can only points to a or b
```

- [ ] Define constants and use them at the same time
```irid
[$n]I32 (1,2,3,4) // n: 4
[$n]$T (1,2,3,4)  // n: 4; T: 1..4
($T, $S) (1, 2)   // T: 1; S: 2
````

- [ ] $ syntax for defining templates/variables based on pattern
```irid
foo : (a: $T; b: T) -> {
  re a+b
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
foo(1\3) // 2\4
```

- [ ] Narrow a name for that scope
```irid
Vec2 : (x:I32; y:I32)
sum : Vec2 -> Vec2.x + Vec2.y
sum(1, 2) // 3
Vec2 = 3, 4
Vec2.x + Vec.y // 7
```

- [ ] {} syntax for creating records with matching field names. This conflicts with expression block syntax.
```irid
x = 1
y = 2
vec = { x, y } // (x = x; y = y)
```

- [ ] named block syntax name={}.
```irid
outer = {
  inner = {
    br outer = 10
  }
}
outer + outer
```

- [ ] pointers to named blocks
```irid
one = {
  two = {
    if 1 do ptr = @one el ptr = @two
    br ptr@
  }
  print "after two"
}
print "after two"
```

- [ ] br with if/el/wh
```irid
wh 1 {
  a = if 2 do {
    br // breaks if
  }
}
wh 1 {
  if 2 do {
    br // breaks wh
  }
}
a = wh 1 {
  wh 2 do {
    br // breaks inner wh
  }
}
```

- [ ] {} pattern syntax for extracting members from a record
```irid
{ y, x } = vec
```

- [ ] type assertion
```irid
b = 10
a = I32'b
```

- [ ] methods syntax
```irid
Vec : (x: i32; y: i32).{
  dot : (vec: Vec) -> x*vec.x + y*vec.y
}
a = Vec(1,2)
b = Vec(3,4)
c = a.dot b // 3 + 8
```

- [ ] function return value type checking
```irid
foo : () -> I32 { // error since I32 1 and I32 2.0 call different functions
  if () re 1
  el    re 2.0
}
bar : () -> I32 { // not error since both values are converted via the same I32
  if () re 1.0
  el    re 2.0
}
baz : () -> (x:I32; y:I32) { // not error
  if () re (1, 2)
  el    re (x=3, y=4)
}
zap : () -> (x:I32; y:I32) { // error since the second returned value have to swap 3 and 4
  if () re (1, 2)
  el    re (y=3, x=4)
}
```

- [ ] Function types
```irid
// Both are function types
(@I32) -> I32
(@I32\nil) -> {
  re I32
}
// Both are function literals
(ptr:@I32) -> ptr@
(ptr: @I32\nil) -> {
  if ptr re ptr@
  re 0
}
// Both are (function type and function literal)
(@I32) -> 1
(ptr:@I32) -> {
  re ptr@ * 0;
}
// Both are function literal
(ptr:@I32) -> {
  ptr@ = 1
  re ptr@
}
(ptr:@I32\nil) -> {
  re ptr@ * 0
}
```

- [ ] Optional do
```irid
if 1 do 2
if 3 do br 4
if 5 br 6
```

- [ ] Packages
```irid
package Vec, Str

str_from_vec:(v: Vec) -> Str {
  return str_from_i32(v.x) " " str_from_i32(v.y)
}
```

- [ ] Prefix Join  `a = [2]\0\1\2`

- [ ] Linear types
```irid
ptr = malloc(100) // ptr is pointer to dynamic memory #1 with size 100
free(ptr)         // dynamic memory #1 that is freed
ptr@              // compile-time error, memory #1 is freed
```

```irid
ptra = malloc(100)       // ptra is a pointer to dynamic memory #1 with size 100
ptrb = ptra              // ptrb is a pointer to dynamic memory #1
if () do ptra = malloc(200) // ptra is a pointer to dynamic memory #2 with size 200
free(ptra)               // dynamic memory #1 or dynamic memory #2 is freed
free(ptrb)               // error, memory #1 may have been freed
```

```irid
ptra = malloc(100)       // ptra is a pointer to dynamic memory #1 with size 100
ptrb = ptra              // ptrb is a pointer to dynamic memory #1
if () do {
  ptra = malloc(200)     // ptra is a pointer to dynamic memory #2 with size 200
  free(ptra)             // memory #2 is freed
}
free(ptrb)               // memory #1 is freed
```

- [ ] Python-like identation based if/while blocks, that are not lexical scopes
```irid
if () do
  a = 1
el
  a = 3
a // 1\3
```

- [ ] Declarations can be used out-of-order.
```irid
A : (xy:Vec2)
b = B(1; 2)
B : (x:I32; y:I32)
```
