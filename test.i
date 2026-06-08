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
  wh n > 0 do {
    digit := n % 10
    putchar (digit + 48)
    n = n / 10
  }
  putchar 10
}


putchar: #c putchar (char:I32) -> I32

print_i32(Game.width)
print_i32(Game.height)
