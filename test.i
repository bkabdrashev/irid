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
    re
  }
  buf: [10](0..9)
  i:I32 = 0
  wh n > 0 do {
    digit := n % 10
    buf[i] = digit
    n = n / 10
    i = i + 1
  }
  wh i > 0 do {
    i = i - 1
    putchar (buf[i] + 48)
  }
  putchar 67
  putchar 10
}

putchar: #c putchar (char:I32) -> I32

putchar 65
putchar 10

print_i32(123)
