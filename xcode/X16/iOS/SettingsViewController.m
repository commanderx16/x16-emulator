//
//  SettingsViewController.m
//  CommanderX16
//
//  ; (C)2020 Matthew Pearce, License: 2-clause BSD
//

#import "SettingsViewController.h"
#import "GameControlViewController.h"
#include "SDL.h"
#include "SDL_keyboard.h"

@interface SettingsViewController ()
int SDL_SendKeyboardKey(Uint8 state, SDL_Scancode scancode);

-(IBAction)buttonPressed:(id)sender;
-(IBAction)closeWindow:(id)sender;
-(IBAction)toggleKeyboard:(id)sender;
-(IBAction)showGamePad:(id)sender;

@property (nonatomic, retain) GameControlViewController *game;
@end

@implementation SettingsViewController

@synthesize game;

-(IBAction)toggleKeyboard:(id)sender {

    SDL_StartTextInput();
}

-(IBAction)buttonPressed:(id)sender
{
    UIButton *button = (UIButton *)sender;
    SDL_SendKeyboardKey(1, button.tag);
}

-(IBAction)closeWindow:(id)sender
{
    [self.view removeFromSuperview];
}

-(IBAction)showGamePad:(id)sender
{
    if (!game) {
        game = [[GameControlViewController alloc] initWithNibName:@"GameControlViewController" bundle:nil];
    }
    
    UIView *sdlView = [self.view superview];
    
    CGRect rect = game.view.frame;
    
    CGPoint origin = CGPointMake(sdlView.frame.size.width - 400, sdlView.frame.size.height - 200);
    rect.origin = origin;
    game.view.frame = rect;
    
    game.view.tag = 667;
    
    [sdlView addSubview:game.view];
}

@end
