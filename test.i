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

putchar: #foreign.c "putchar" (char:I32) -> I32

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
  // x:    F32                    /**< X coordinate, relative to window */
  // y:    F32                    /**< Y coordinate, relative to window */
  // xrel: F32                    /**< The relative motion in the X direction */
  // yrel: F32                    /**< The relative motion in the Y direction */
)

init            : #foreign.c "SDL_Init"            (flags: InitFlags) -> B8
CreateWindow    : #foreign.c "SDL_CreateWindow"    (title: @I8; w: I32; h: I32; flags: WindowFlags) -> @Window
CreateRenderer  : #foreign.c "SDL_CreateRenderer"  (window: @Window; name: @I8) -> @Renderer
GetTicks        : #foreign.c "SDL_GetTicks"        () -> U64
PollEvent       : #foreign.c "SDL_PollEvent"       (event: @Event) -> B8
Delay           : #foreign.c "SDL_Delay"           (ms: U32) -> ()
DestroyRenderer : #foreign.c "SDL_DestroyRenderer" (renderer: @Renderer) -> ()
DestroyWindow   : #foreign.c "SDL_DestroyWindow"   (window: @Window) -> ()
Quit            : #foreign.c "SDL_Quit"            () -> ()

RenderPresent   : #foreign.c "SDL_RenderPresent"   (renderer: @Renderer) -> B8

print_str("Hello World!")

init(INIT_VIDEO | INIT_EVENTS)
window:   @Window   = CreateWindow(@("Title\0"[0]); WINDOW_WIDTH; WINDOW_HEIGHT; 0)
renderer: @Renderer = CreateRenderer(window; @("\0"[0]))
event:     Event

running: B8 = 1
while running do {
  while PollEvent(@event) do {
    if event.type == QUIT do {
      print_str("Event Quit")
      running = 0
    }
    else if event.type == KEY_DOWN do {
      print_i32(event.scancode) // TODO: llvm generator doesn't know the correct offset for scancode
      print_i32(event.type)
      if event.scancode == SCANCODE_ESCAPE do {
        print_str("Escape")
        running = 0
      }
    }
    else if event.type == MOUSE_MOTION do {
      print_i32(event.type)
    }
  }
  RenderPresent(renderer)
  Delay(10)
}

DestroyRenderer(renderer)
DestroyWindow(window)
Quit()

print_str("Bye World!")
