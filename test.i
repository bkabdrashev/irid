putchar: #foreign.c "putchar" (char:I32)->I32

print_i64:(n:I64) -> {
  if n == 0 do {
    putchar 48
    putchar 10
    return
  }
  if n < 0 do {
    putchar 45 // -
    n = -1 * n
  }
  buf: [20](0..9)
  i:I8 = 0
  while n > 0 do {
    digit := n % 10
    buf[i] = digit
    n = n / 10
    i = i + 1
  }
  while i > 0 do {
    i = i - 1
    putchar(buf[i] + 48)
  }
  putchar 10
}

print_f32:(n:F32) -> {
  if n < 0.0 do {
    putchar 45 // -
    n = -n
  }

  whole: I64 = I64 n
  frac: I32 = I32 ((n - F32 whole) * 1000.0 + 0.5) // three decimal places
  if frac >= 1000 do {
    frac = 0
    whole = whole + 1
  }

  if whole == 0 do
    putchar 48
  else {
    buf: [20](0..9)
    i:I8 = 0
    while whole > 0 do {
      buf[i] = whole % 10
      whole = whole / 10
      i = i + 1
    }
    while i > 0 do {
      i = i - 1
      putchar(buf[i] + 48)
    }
  }

  putchar 46 // .

  div: I32 = 100
  while div > 0 do {
    putchar(frac / div % 10 + 48)
    div = div / 10
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

// TODO: recursion
// factorial : (n: I32) -> {
//   if n < 2 return 1
//   return n * factorial(n-1)
// }
// print_i64(factorial(5))

// TODO: namespace sdl
INIT_VIDEO     : 0x00000020
INIT_EVENTS    : 0x00004000
InitFlags      : U32
WindowFlags    : I64
Window         : #type 64'#bits(opaque:"SDL_Window")
Renderer       : #type 64'#bits(opaque:"SDL_Renderer")
QUIT           : 0x100
KEY_DOWN       : 0x300
MOUSE_MOTION   : 0x400
Event          : #type (128*8)'#bits(QuitEvent \ KeyboardEvent \ MouseMotionEvent \ UnknownEvent)
WindowID       : U32
KeyboardID     : U32
Keycode        : U32
Keymod         : U16

SCANCODE_RETURN    : 40
SCANCODE_ESCAPE    : 41
SCANCODE_BACKSPACE : 42
SCANCODE_TAB       : 43
SCANCODE_SPACE     : 44
SCANCODE_A : 4
SCANCODE_B : 5
SCANCODE_C : 6
SCANCODE_D : 7
SCANCODE_E : 8
SCANCODE_F : 9
SCANCODE_G : 10
SCANCODE_H : 11
SCANCODE_I : 12
SCANCODE_J : 13
SCANCODE_K : 14
SCANCODE_L : 15
SCANCODE_M : 16
SCANCODE_N : 17
SCANCODE_O : 18
SCANCODE_P : 19
SCANCODE_Q : 20
SCANCODE_R : 21
SCANCODE_S : 22
SCANCODE_T : 23
SCANCODE_U : 24
SCANCODE_V : 25
SCANCODE_W : 26
SCANCODE_X : 27
SCANCODE_Y : 28
SCANCODE_Z : 29

Scancode : U16 // TODO: enum

UnknownEvent : #type (
  type      : U32 0
)

QuitEvent : #type (
  type      : U32 QUIT
  reserved  : U32
  timestamp : U64   /**< In nanoseconds, populated using SDL_GetTicksNS() */
)

KeyboardEvent : #type (
  type      : U32 KEY_DOWN /**< SDL_EVENT_KEY_DOWN or SDL_EVENT_KEY_UP */
  reserved  : U32
  timestamp : U64         /**< In nanoseconds, populated using SDL_GetTicksNS() */
  windowID  : WindowID     /**< The window with keyboard focus, if any */
  which     : KeyboardID   /**< The keyboard instance id, or 0 if unknown or virtual */
  scancode  : Scancode     /**< SDL physical key code */
  key       : Keycode      /**< SDL virtual key code */
  mod       : Keymod       /**< current key modifiers */
  raw       : U16          /**< The platform dependent scancode for this event */
  down      : B8           /**< true if the key is pressed */
  repeat    : B8           /**< true if this is a key repeat */
)

MouseButtonFlags: U32
MouseID: U32

MouseMotionEvent : #type (
  type:      U32 MOUSE_MOTION       /**< SDL_EVENT_MOUSE_MOTION */
  reserved:  U32
  timestamp: U64                    /**< In nanoseconds, populated using SDL_GetTicksNS() */
  windowID:  WindowID               /**< The window with mouse focus, if any */
  which:     MouseID                /**< The mouse instance id in relative mode, SDL_TOUCH_MOUSEID for touch events, SDL_PEN_MOUSEID for pen events, or 0 */
  state:     MouseButtonFlags       /**< The current button state */
  x:         F32                    /**< X coordinate, relative to window */
  y:         F32                    /**< Y coordinate, relative to window */
  xrel:      F32                    /**< The relative motion in the X direction */
  yrel:      F32                    /**< The relative motion in the Y direction */
)

init            : #foreign.c "SDL_Init"            (flags: InitFlags)->B8
CreateWindow    : #foreign.c "SDL_CreateWindow"    (title: @I8; w: I32; h: I32; flags: WindowFlags)->@Window
CreateRenderer  : #foreign.c "SDL_CreateRenderer"  (window: @Window; name: @I8)->@Renderer
GetTicks        : #foreign.c "SDL_GetTicks"        ()->U64
rand            : #foreign.c "SDL_rand"            (n: I32)->I32
PollEvent       : #foreign.c "SDL_PollEvent"       (event: @Event)->B8
Delay           : #foreign.c "SDL_Delay"           (ms: U32)->()
DestroyRenderer : #foreign.c "SDL_DestroyRenderer" (renderer: @Renderer)->()
DestroyWindow   : #foreign.c "SDL_DestroyWindow"   (window: @Window)->()
Quit            : #foreign.c "SDL_Quit"            ()->()

ALPHA_OPAQUE : -127
FRect : #type (
  x: F32
  y: F32
  w: F32
  h: F32
)

SetRenderDrawColor : #foreign.c "SDL_SetRenderDrawColor" (renderer: @Renderer; r: I8; g: I8; b: I8; a: I8)->B8
RenderClear        : #foreign.c "SDL_RenderClear"        (renderer: @Renderer)->B8
RenderFillRect     : #foreign.c "SDL_RenderFillRect"     (renderer: @Renderer; rect: @FRect)->B8
RenderPresent      : #foreign.c "SDL_RenderPresent"      (renderer: @Renderer)->B8

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
  SetRenderDrawColor(renderer; 15; 15; 15; ALPHA_OPAQUE)
  RenderClear(renderer)

  {
    r: FRect = (0.0; 0.0; BLOCK_PIXELS - 2.0; BLOCK_PIXELS - 2.0)
    i: I32 = 0
  while i < game.food_n do
    r.x = game.food_p[i].x * (r.w + 2.0)
    r.y = game.food_p[i].y * (r.h + 2.0)
    SetRenderDrawColor(renderer; 127; 16; 16; ALPHA_OPAQUE)
    RenderFillRect(renderer; @r)
    i = i + 1
  }

  {
    r: FRect = (0.0; 0.0; BLOCK_PIXELS - 2.0; BLOCK_PIXELS - 2.0)
    i: I32 = 0
  while i < game.snake_n do
    r.x = game.snake_p[i].x * (r.w + 2.0)
    r.y = game.snake_p[i].y * (r.h + 2.0)
    SetRenderDrawColor(renderer; 16; 127; 16; ALPHA_OPAQUE)
    RenderFillRect(renderer; @r)
    i = i + 1
  r.x = game.snake_p[0].x * (r.w + 2.0)
  r.y = game.snake_p[0].y * (r.h + 2.0)
  SetRenderDrawColor(renderer; 127; 127; 16; ALPHA_OPAQUE)
  RenderFillRect(renderer; @r)
  }


  RenderPresent(renderer)
}

init(INIT_VIDEO | INIT_EVENTS)
window:   @Window   = CreateWindow(@ "Title\0"[0]; WINDOW_WIDTH; WINDOW_HEIGHT; 0)
renderer: @Renderer = CreateRenderer(window; @ "\0"[0])

game.running = 1
game.snake_n = 1
game.snake_p[0].x = game.width  / 2.0 - 1.0
game.snake_p[0].y = game.height / 2.0 - 1.0
game.snake_v[0].x = 0.0
game.snake_v[0].y = 0.0
game.next_v.x = 0.0
game.next_v.y = 0.0
game.last_tick = F32 GetTicks()
game.last_time = 0.0
new_food(); new_food()
new_food(); new_food()

while game.running do {
  event: Event
  while PollEvent(@event) do {
    if event.type == QUIT do {
      print_str("Event Quit")
      print_i64(event.timestamp)
      game.running = 0
    }
    else if event.type == KEY_DOWN do {
      if event.scancode == SCANCODE_ESCAPE do {
        print_str("Escape")
        game.running = 0
      }
      /* FIX: && breaks the join records narrowing. It thinks .scancode doesn't appear all records
      else if event.scancode == SCANCODE_H && equal(game.snake_v[0].x; 0.0) do
      else if event.scancode == SCANCODE_L do && equal(game.snake_v[0].x; 0.0) do
      */
      else if event.scancode == SCANCODE_H do {
        if equal(game.snake_v[0].x; 0.0) do {
          game.next_v.x = -BLOCKS_PER_MS
          game.next_v.y = 0.0
        }
      }
      else if event.scancode == SCANCODE_L do {
        if equal(game.snake_v[0].x; 0.0) do {
          game.next_v.x = +BLOCKS_PER_MS
          game.next_v.y = 0
        }
      }
      else if event.scancode == SCANCODE_K do {
        if equal(game.snake_v[0].y; 0.0) do {
          game.next_v.x = 0
          game.next_v.y = -BLOCKS_PER_MS
        }
      }
      else if event.scancode == SCANCODE_J do {
        if equal(game.snake_v[0].y; 0.0) do {
          game.next_v.x = 0
          game.next_v.y = +BLOCKS_PER_MS
        }
      }
    }
    else if event.type == MOUSE_MOTION do {
    }
  }

  update()
  draw()
  Delay(10)
  // game.running = 0;
}

DestroyRenderer(renderer)
DestroyWindow(window)
Quit()

print_str("Bye World!")
