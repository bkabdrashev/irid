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

INIT_VIDEO     : 0x00000020
INIT_EVENTS    : 0x00004000
InitFlags      : 32'#bits (INIT_VIDEO\INIT_EVENTS)
WindowFlags    : I64
Window         : type 64'#bits (opaque:"SDL_Window")
Renderer       : type 64'#bits (opaque:"SDL_Renderer")
QUIT           : 0x100
KEY_DOWN       : 0x300
EventKind      : type 32'#bits (QUIT\KEY_DOWN)
Event          : type (8*128)'#bits (kind: EventKind) // TODO: c-style union
WindowID       : U32
KeyboardID     : U32
Keycode        : U32
Keymod         : U16
Scancode       : U16 // TODO: real scancodes

KeyboardEvent : type (
  kind      : EventKind   /**< SDL_EVENT_KEY_DOWN or SDL_EVENT_KEY_UP */
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

init            : #foreign.c "SDL_Init"            type (flags: InitFlags) -> B8
CreateWindow    : #foreign.c "SDL_CreateWindow"    type (title: @I8; w: I32; h: I32; flags: WindowFlags) -> @Window
CreateRenderer  : #foreign.c "SDL_CreateRenderer"  type (window: @Window; name: @I8) -> @Renderer
GetTicks        : #foreign.c "SDL_GetTicks"        type () -> U64
PollEvent       : #foreign.c "SDL_PollEvent"       type (event: @Event) -> B8
Delay           : #foreign.c "SDL_Delay"           type (ms: U32) -> ()
DestroyRenderer : #foreign.c "SDL_DestroyRenderer" type (renderer: @Renderer) -> ()
DestroyWindow   : #foreign.c "SDL_DestroyWindow"   type (window: @Window) -> ()
Quit            : #foreign.c "SDL_Quit"            type () -> ()

RenderPresent   : #foreign.c "SDL_RenderPresent"   type (renderer: @Renderer) -> B8

print_str("Hello World!")

init(INIT_VIDEO)
window:   @Window   = CreateWindow(@("Title\0"[0]); WINDOW_WIDTH; WINDOW_HEIGHT; 0)
renderer: @Renderer = CreateRenderer(window; @("\0"[0]))
last_step: U64 = GetTicks()
event: Event

t: U64 = GetTicks()
running: B8 = 1
while running do {
  while PollEvent(@event) do {
    if event.kind == QUIT do {
      print_str("Quit")
      running = 0
    }
  }
  now: U64 = GetTicks()
  dt:  U64 = now - t
  t = now
  RenderPresent(renderer)
  Delay(10)
}
DestroyRenderer(renderer)
DestroyWindow(window)
Quit()

print_str("Bye World!")
