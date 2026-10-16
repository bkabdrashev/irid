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
Event          : #type (128*8)'#bits(QuitEvent \ KeyboardEvent \ MouseMotionEvent \ UnknownEvent) // TODO: c-style union
WindowID       : U32
KeyboardID     : U32
Keycode        : U32
Keymod         : U16

SCANCODE_RETURN    : 40
SCANCODE_ESCAPE    : 41
SCANCODE_BACKSPACE : 42
SCANCODE_TAB       : 43
SCANCODE_SPACE     : 44
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
  type: U32 MOUSE_MOTION         /**< SDL_EVENT_MOUSE_MOTION */
  reserved: U32
  timestamp: U64                /**< In nanoseconds, populated using SDL_GetTicksNS() */
  windowID: WindowID            /**< The window with mouse focus, if any */
  which: MouseID                /**< The mouse instance id in relative mode, SDL_TOUCH_MOUSEID for touch events, SDL_PEN_MOUSEID for pen events, or 0 */
  state: MouseButtonFlags       /**< The current button state */
  // x:    F32                    /**< X coordinate, relative to window */ // TODO: floats
  // y:    F32                    /**< Y coordinate, relative to window */
  // xrel: F32                    /**< The relative motion in the X direction */
  // yrel: F32                    /**< The relative motion in the Y direction */
)

init            : #foreign.c "SDL_Init"            (flags: InitFlags)->B8
CreateWindow    : #foreign.c "SDL_CreateWindow"    (title: @I8; w: I32; h: I32; flags: WindowFlags)->@Window
CreateRenderer  : #foreign.c "SDL_CreateRenderer"  (window: @Window; name: @I8)->@Renderer
GetTicks        : #foreign.c "SDL_GetTicks"        ()->U64
PollEvent       : #foreign.c "SDL_PollEvent"       (event: @Event)->B8
Delay           : #foreign.c "SDL_Delay"           (ms: U32)->()
DestroyRenderer : #foreign.c "SDL_DestroyRenderer" (renderer: @Renderer)->()
DestroyWindow   : #foreign.c "SDL_DestroyWindow"   (window: @Window)->()
Quit            : #foreign.c "SDL_Quit"            ()->()

ALPHA_OPAQUE : -127
// FRect : #type (
//   x: I32
//   y: I32
//   w: I32
//   h: I32
// )

SetRenderDrawColor : #foreign.c "SDL_SetRenderDrawColor" (renderer: @Renderer; r: I8; g: I8; b: I8; a: I8)->B8
RenderClear        : #foreign.c "SDL_RenderClear"        (renderer: @Renderer)->B8
// RenderFillRect     : #foreign.c "SDL_RenderFillRect"     (renderer: @Renderer; rect: @FRect)->B8
RenderPresent      : #foreign.c "SDL_RenderPresent"      (renderer: @Renderer)->B8

print_str("Hello World!")

BLOCK_SIZE_IN_PIXELS : 24
WINDOW_WIDTH   : BLOCK_SIZE_IN_PIXELS * game.width
WINDOW_HEIGHT  : BLOCK_SIZE_IN_PIXELS * game.height

Vec2 : #type (
  x: I32
  y: I32
)

game : (
  width   : 12
  height  : 12
  size    : width * height

  snake_p : [size]Vec2
  snake_n : I32
  running : B8
  time    : I32
)

init(INIT_VIDEO | INIT_EVENTS)
window:   @Window   = CreateWindow(@ "Title\0"[0]; WINDOW_WIDTH; WINDOW_HEIGHT; 0)
renderer: @Renderer = CreateRenderer(window; @("\0"[0]))

game.running = 1
while game.running do {
  event: Event
  while PollEvent(@event) do {
    if event.type == QUIT do {
      print_str("Event Quit")
      print_i64(event.timestamp)
      game.running = 0
    }
    else if event.type == KEY_DOWN do {
      print_i64(event.scancode)
      print_i64(event.type)
      if event.scancode == SCANCODE_ESCAPE do {
        print_str("Escape")
        game.running = 0
      }
    }
    else if event.type == MOUSE_MOTION do {
      print_i64(event.type)
    }
  }
  SetRenderDrawColor(renderer; 127; 0; 0; ALPHA_OPAQUE)
  RenderClear(renderer)

  // for i in 0..game.food_n-1 do {
  //   r.x = game.food[i].x * (r.w + 2)
  //   r.y = game.food[i].y * (r.h + 2)
  //   sdl.SetRenderDrawColor(renderer, 128, 16, 16, sdl.ALPHA_OPAQUE)
  //   sdl.RenderFillRect(renderer, r)
  // }

  // for i in 1..game.snake_n-1 do {
  //   r.x = game.snake[i].x * (r.w + 2)
  //   r.y = game.snake[i].y * (r.h + 2)
  //   sdl.SetRenderDrawColor(renderer, 0, 128, 0, sdl.ALPHA_OPAQUE)
  //   sdl.RenderFillRect(renderer, r)
  // }
  // r.x = game.snake.0.x * (r.w + 2)
  // r.y = game.snake.0.y * (r.h + 2)
  // SetRenderDrawColor(renderer; 128; 128; 0; ALPHA_OPAQUE)
  // RenderFillRect(renderer; r)

  RenderPresent(renderer)
  Delay(10)
}

DestroyRenderer(renderer)
DestroyWindow(window)
Quit()

print_str("Bye World!")
