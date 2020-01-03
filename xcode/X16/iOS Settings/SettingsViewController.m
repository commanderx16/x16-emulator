//
//  SettingsViewController.m
//  CommanderX16
//
//  Created by Pearce, Matthew (Senior Developer) on 01/01/2020.
//

#import "SettingsViewController.h"
#include "SDL.h"
#include "SDL_keyboard.h"

@interface SettingsViewController ()

@property (nonatomic, strong) IBOutlet UIButton *keyboard1;
-(IBAction)buttonPressed:(id)sender;
-(IBAction)closeWindow:(id)sender;
-(IBAction)toggleKeyboard:(id)sender;
@end

@implementation SettingsViewController

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

@end
