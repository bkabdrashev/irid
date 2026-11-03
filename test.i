use import "basic"

sdl : import "sdl"

print_str("Hello World!")

BLOCK_PIXELS   : 64
BLOCKS_PER_MS  : 0.004
WINDOW_WIDTH   : BLOCK_PIXELS * game.width
WINDOW_HEIGHT  : BLOCK_PIXELS * game.height

min : (a:F32; b:F32) -> if a < b do a else b
max : (a:F32; b:F32) -> if a > b do a else b
round : (a: F32) -> if a < 0.0 do F32(I32(a-0.5)) else F32(I32(a+0.5))
equal : (a: F32; b: F32) -> B8 {
  epsilon:F32 = 0.00001
  result:B8 = a+epsilon >= b && a-epsilon <= b
  return result
}


intersection : (x0: F32; y0: F32; x1: F32; y1: F32) -> {
  xa: F32 = 1.0 + min(x0; x1) - max(x0; x1)
  ya: F32 = 1.0 + min(y0; y1) - max(y0; y1)
  xOverlap: F32 = max(0.0; xa)
  yOverlap: F32 = max(0.0; ya)
  return xOverlap * yOverlap
}

Vec2 : #type (
  x: F32
  y: F32
)

game : (
  width   : 8
  height  : 8
  size    : width * height

  snake_p : [size]Vec2
  snake_v : [size]Vec2
  next_v  : Vec2
  snake_n : I32

  food_p : [size]Vec2
  food_n : I32

  running : B8
  last_tick : F32
  last_time : F32
)

new_food : () -> {
  empty_n: I32 = game.size - game.snake_n - game.food_n
  if empty_n > 0 do {
    board: [game.size]B8
    { i:I32 = 0
    while i < game.size do
      board[i] = 1
      i = i + 1
    }

    { i:I32 = 0
    while i < game.snake_n do
      k:I32 = game.snake_p[i].x'I32 + game.snake_p[i].y'I32 * game.width
      board[k] = 0
      i = i + 1
    }

    { i:I32 = 0
    while i < game.food_n do
      k:I32 = game.food_p[i].x'I32 + game.food_p[i].y'I32 * game.width
      board[k] = 0
      i = i + 1
    }

    {
      empty: [game.size]Vec2
      e: I32 = 0
      i: I32 = 0

      while i < game.size do
        if board[i] do {
          empty[e].x = F32 (i % game.width)
          empty[e].y = F32 (i / game.height)
          e = e + 1
        }
        i = i + 1

      r: I32 = rand(empty_n)
      game.food_p[game.food_n].x = empty[r].x
      game.food_p[game.food_n].y = empty[r].y
      game.food_n = game.food_n + 1
    }
  }
}

update : () -> {
  now: F32 = F32 GetTicks()
  dt:  F32 = now - game.last_tick
  game.last_tick = now

  {
    i: I32 = 0
  while i < game.snake_n do
    game.snake_p[i].x = game.snake_p[i].x + game.snake_v[i].x * dt
    game.snake_p[i].y = game.snake_p[i].y + game.snake_v[i].y * dt
    i = i + 1
  }

  game.last_time = game.last_time + dt
  if (game.last_time * BLOCKS_PER_MS) >= 1.0 do { // one block is passed
    {
      i: I32 = 0
    while i < game.snake_n do
      game.snake_p[i].x = round(game.snake_p[i].x)
      game.snake_p[i].y = round(game.snake_p[i].y)
      i = i + 1
    }
    {
      i: I32 = 1
    while i < game.snake_n do
      if equal(game.snake_p[0].x; game.snake_p[i].x)
      && equal(game.snake_p[0].y; game.snake_p[i].y) do {
        print_str("lost")
        game.running = 0
      }
      i = i + 1
    }

    if game.snake_n >= game.size do { // game is won
      print_str("won")
      game.running = 0
    }


    if game.snake_p[0].x < 0.0
    || game.snake_p[0].y < 0.0
    || game.snake_p[0].x >= F32(game.width)
    || game.snake_p[0].y >= F32(game.height) do { // game over
      print_str("lost")
      game.running = 0
    }

    { // is food eaten?
      i: I32 = 0
    while i < game.food_n do
      if intersection(game.snake_p[0].x; game.snake_p[0].y; game.food_p[i].x; game.food_p[i].y) > 0.2 do {
        game.food_p[i].x = game.food_p[game.food_n-1].x
        game.food_p[i].y = game.food_p[game.food_n-1].y
        game.food_n = game.food_n - 1
        game.snake_n = game.snake_n + 1
        new_food()
        game.snake_p[game.snake_n-1].x = game.snake_p[game.snake_n-2].x
        game.snake_p[game.snake_n-1].y = game.snake_p[game.snake_n-2].y
      }
      i = i + 1
    }

    game.snake_v[0].x = game.next_v.x
    game.snake_v[0].y = game.next_v.y

    { i: I32 = 1
    while i < game.snake_n do
      dx: F32 = game.snake_p[i-1].x - game.snake_p[i].x
      dy: F32 = game.snake_p[i-1].y - game.snake_p[i].y
      game.snake_v[i].x = BLOCKS_PER_MS * dx
      game.snake_v[i].y = BLOCKS_PER_MS * dy
      i = i + 1
    }

    game.last_time = 0
  }
}

draw : () -> {
  sdl.SetRenderDrawColor(renderer; 15; 15; 15; ALPHA_OPAQUE)
  sdl.RenderClear(renderer)

  r: FRect = (0.0; 0.0; BLOCK_PIXELS - 2.0; BLOCK_PIXELS - 2.0)
  {
    i: I32 = 0
  while i < game.food_n do
    r.x = game.food_p[i].x * (r.w + 2.0)
    r.y = game.food_p[i].y * (r.h + 2.0)
    sdl.SetRenderDrawColor(renderer; 127; 16; 16; ALPHA_OPAQUE)
    sdl.RenderFillRect(renderer; @r)
    i = i + 1
  }

  {
    i: I32 = 0
  while i < game.snake_n do
    r.x = game.snake_p[i].x * (r.w + 2.0)
    r.y = game.snake_p[i].y * (r.h + 2.0)
    sdl.SetRenderDrawColor(renderer; 16; 127; 16; ALPHA_OPAQUE)
    sdl.RenderFillRect(renderer; @r)
    i = i + 1
  }
  r.x = game.snake_p[0].x * (r.w + 2.0)
  r.y = game.snake_p[0].y * (r.h + 2.0)
  sdl.SetRenderDrawColor(renderer; 127; 127; 16; ALPHA_OPAQUE)
  sdl.RenderFillRect(renderer; @r)


  sdl.RenderPresent(renderer)
}

sdl.init(INIT_VIDEO | INIT_EVENTS)
window:   @Window   = sdl.CreateWindow(@ "Title\0"[0]; WINDOW_WIDTH; WINDOW_HEIGHT; 0)
renderer: @Renderer = sdl.CreateRenderer(window; @ "\0"[0])

game.running = 1
game.snake_n = 1
game.snake_p[0].x = game.width  / 2.0 - 1.0
game.snake_p[0].y = game.height / 2.0 - 1.0
game.snake_v[0].x = 0.0
game.snake_v[0].y = 0.0
game.next_v.x = 0.0
game.next_v.y = 0.0
game.last_tick = F32 sdl.GetTicks()
game.last_time = 0.0
new_food(); new_food()
new_food(); new_food()

while game.running do {
  event: sdl.Event
  while sdl.PollEvent(@event) do {
    if event.type == sdl.QUIT do {
      print_str("Event Quit")
      print_i64(event.timestamp)
      game.running = 0
    }
    else if event.type == sdl.KEY_DOWN do {
      if event.scancode == sdl.SCANCODE_ESCAPE do {
        print_str("Escape")
        game.running = 0
      }
      else if event.scancode == sdl.SCANCODE_H && equal(game.snake_v[0].x; 0.0) do {
        game.next_v.x = -BLOCKS_PER_MS
        game.next_v.y = 0.0
      }
      else if event.scancode == sdl.SCANCODE_L && equal(game.snake_v[0].x; 0.0) do {
        game.next_v.x = +BLOCKS_PER_MS
        game.next_v.y = 0
      }
      else if event.scancode == sdl.SCANCODE_K && equal(game.snake_v[0].y; 0.0) do {
        game.next_v.x = 0
        game.next_v.y = -BLOCKS_PER_MS
      }
      else if event.scancode == sdl.SCANCODE_J && equal(game.snake_v[0].y; 0.0) do {
        game.next_v.x = 0
        game.next_v.y = +BLOCKS_PER_MS
      }
    }
    else if event.type == sdl.MOUSE_MOTION do {
    }
  }

  update()
  draw()
  sdl.Delay(10)
  // game.running = 0;
}

sdl.DestroyRenderer(renderer)
sdl.DestroyWindow(window)
sdl.Quit()

print_str("Bye World!")
