#if __APPLE__
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
#include "SDL.h"
#include "ios_functions.h"

#elif TARGET_OS_MAC
#include <SDL.h>
#else
#   error "Unknown Apple platform"
#endif
#else
#include <SDL.h>
#endif

void handle_keyboard(bool down, SDL_Keycode sym, SDL_Scancode scancode);

