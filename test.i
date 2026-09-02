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
  // FIX: i is considered to be 0 here, even though it's incrementing inside a loop
    i = i + 1
  }
  while i > 0 do {
    i = i - 1
    putchar (buf[i] + 48)
  }
  putchar 10
}

putchar: #c putchar (char:I32) -> I32

print_i32(Game.width * Game.heigth)

// sdl.init(sdl.INIT_VIDEO | sdl.INIT_JOYSTICK)
