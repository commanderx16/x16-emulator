#if __APPLE__
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
#include "ios_functions.h"
#endif
#endif

#include <SDL.h>

void handle_keyboard(bool down, SDL_Keycode sym, SDL_Scancode scancode);

