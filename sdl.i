INIT_AUDIO    : 0x00000010 /**< `SDL_INIT_AUDIO` implies `SDL_INIT_EVENTS` */
INIT_VIDEO    : 0x00000020 /**< `SDL_INIT_VIDEO` implies `SDL_INIT_EVENTS`, should be initialized on the main thread */
INIT_JOYSTICK : 0x00000200 /**< `SDL_INIT_JOYSTICK` implies `SDL_INIT_EVENTS`, should be initialized on the same thread as SDL_INIT_VIDEO on Windows if you don't set SDL_HINT_JOYSTICK_THREAD */
INIT_HAPTIC   : 0x00001000
INIT_GAMEPAD  : 0x00002000 /**< `SDL_INIT_GAMEPAD` implies `SDL_INIT_JOYSTICK` */
INIT_EVENTS   : 0x00004000
INIT_SENSOR   : 0x00008000 /**< `SDL_INIT_SENSOR` implies `SDL_INIT_EVENTS` */
INIT_CAMERA   : 0x00010000 /**< `SDL_INIT_CAMERA` implies `SDL_INIT_EVENTS` */

InitFlags : 32'bits (INIT_AUDIO\INIT_VIDEO\INIT_JOYSTICK\INIT_HAPTIC\INIT_GAMEPAD\INIT_EVENTS\INIT_SENSOR\INIT_CAMERA)
init : #c SDL_Init (flags: InitFlags) -> B8

Window :  64'bits (opaque:"SDL_Window")
WindowFlags : 64'bits
CreateWindow : #c SDL_CreateWindow (title: Str; w: I32; h: I32; flags: WindowFlags) -> @Window

Renderer : 64'bits (opaque:"SDL_Renderer")
CreateRenderer : #c SDL_CreateRenderer (window: @Window; name: Str) -> @Renderer
GetTicks : #c SDL_GetTicks () -> I64
