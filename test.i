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
  while i > 0 do {
    i = i - 1
    putchar (buf[i] + 48)
  }
  putchar 10
}

print_str:(str: Str) -> {
  i:I32 = 0
  while i < str.length do {
    putchar str[i]
    i++
  }
  putchar 10
}

putchar: #c putchar (char:I32) -> I32

Vec : (x: I32; y: I32; z: 100)
v:Vec = (123;)

print_i32(v.x + v.y + v.z)

Window : type 64'bits (opaque:"SDL_Window")
window: Window

print_i32(window.opaque)

// CreateWindow : #c SDL_CreateWindow (title: Str; w: I32; h: I32; flags: WindowFlags) -> @Window
// import sdl
// sdl.init(sdl.INIT_VIDEO | sdl.INIT_JOYSTICK)
// window := sdl.CreateWindow("Shy SDL3 Snake", WINDOW_WIDTH, WINDOW_HEIGHT, 0)
