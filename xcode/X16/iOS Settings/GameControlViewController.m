//
//  GameControlViewController.m
//  CommanderX16
//
//  Created by Pearce, Matthew (Senior Developer) on 03/01/2020.
//

#import "GameControlViewController.h"
#include "SDL.h"
#include "SDL_keyboard.h"

@interface GameControlViewController ()
-(IBAction)buttonPressed:(id)sender;
-(IBAction)closeWindow:(id)sender;
@end

@implementation GameControlViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
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

@end
