BLOCK_SIZE_IN_PIXELS : 24
WINDOW_WIDTH   : BLOCK_SIZE_IN_PIXELS * Game.width
WINDOW_HEIGHT  : BLOCK_SIZE_IN_PIXELS * Game.height

Game : (
  width : 12
  height: 12
)

print_i32:(n:I32) -> {
  if n == 0 do {
    putchar 48
    putchar 10
    return
  }
  if n < 0 do {
    putchar 45 // -
    n = -1 * n
  }
  buf: [10](0..9)
  i:I32 = 0
  while n > 0 do {
    digit := n % 10
    buf[i] = digit
    n = n / 10
    i = i + 1
  }
  putchar 10
  while i > 0 do {
    i = i - 1
    putchar(buf[i] + 48)
  }
  putchar 10
}

print_str:(str: Str) -> {
  i:I32 = 0
  while i < str.len do {
    putchar(str[i])
    i = i + 1
  }
  putchar 10
}

putchar: #foreign.c "putchar" type (char:I32) -> I32

Vec : (x: I32; y: I32; z: 100)
v:Vec = (123;)

print_i32(v.x + v.z)

WindowFlags : I64
Window : type 64'#bits (opaque:"SDL_Window")

// print_str(window@.opaque)

print_str("Hello World!")

CreateWindow : #foreign.c "SDL_CreateWindow" type (title: @I8; w: I32; h: I32; flags: WindowFlags) -> @Window
window: @Window = CreateWindow(@("Title\0"[0]); WINDOW_WIDTH; WINDOW_HEIGHT; 0)

// import sdl
// sdl.init(sdl.INIT_VIDEO | sdl.INIT_JOYSTICK)
//
// Renderer : type 64'bits (opaque:"SDL_Renderer")
// CreateRenderer : #foreign.c "SDL_CreateRenderer" type (window: @Window; name: Cstr) -> @Renderer
// GetTicks : #foreign.c SDL_GetTicks () -> U64
