//
//  SettingsViewController.m
//  CommanderX16
//
//  Created by Pearce, Matthew (Senior Developer) on 01/01/2020.
//

#import "SettingsViewController.h"
#import "GameControlViewController.h"
#include "SDL.h"
#include "SDL_keyboard.h"

@interface SettingsViewController ()

-(IBAction)buttonPressed:(id)sender;
-(IBAction)closeWindow:(id)sender;
-(IBAction)toggleKeyboard:(id)sender;
-(IBAction)showGamePad:(id)sender;

@property (nonatomic, retain) GameControlViewController *game;
@end

@implementation SettingsViewController

@synthesize game;

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
}

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

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
