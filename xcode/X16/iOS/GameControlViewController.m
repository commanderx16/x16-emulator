//
//  GameControlViewController.m
//  CommanderX16
//
//  ; (C)2020 Matthew Pearce, License: 2-clause BSD
//

#import "GameControlViewController.h"
#include "SDL.h"
#include "SDL_keyboard.h"

@interface GameControlViewController ()
int SDL_SendKeyboardKey(Uint8 state, SDL_Scancode scancode);

-(IBAction)buttonPressed:(id)sender;
-(IBAction)closeWindow:(id)sender;
@end

@implementation GameControlViewController

-(IBAction)buttonPressed:(id)sender
{
    UIButton *button = (UIButton *)sender;
    SDL_SendKeyboardKey(1, button.tag);
}

-(IBAction)closeWindow:(id)sender
{
    [self.view removeFromSuperview];
}

@end
