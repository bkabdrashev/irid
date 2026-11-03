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
