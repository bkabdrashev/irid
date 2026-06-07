putchar: #c putchar (char:I32) -> I32

a:I32 = 70
b:@I32 = @a
if 1 do
  b@ = 71
putchar a
putchar 10
