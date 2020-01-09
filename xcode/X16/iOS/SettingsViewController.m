//
//	SettingsViewController.m
//	CommanderX16
//
//	; (C)2020 Matthew Pearce, License: 2-clause BSD
// 	icons by Icons8

#import "SettingsViewController.h"
#import "GameControlViewController.h"
#include "SDL.h"
#include "SDL_keyboard.h"
#include "ios_functions.h"
#import "SDL_uikitappdelegate.h"

@interface SettingsViewController ()
int SDL_SendKeyboardKey(Uint8 state, SDL_Scancode scancode);
-(IBAction)buttonPressed:(id)sender;
-(IBAction)closeWindow:(id)sender;
-(IBAction)toggleKeyboard:(id)sender;
-(IBAction)showGamePad:(id)sender;
-(IBAction)reset:(id)sender;

@property (nonatomic, retain) GameControlViewController *game;
@end

@implementation SettingsViewController

@synthesize game;

-(void)viewDidLoad {
	[super viewDidLoad];

	self.view.translatesAutoresizingMaskIntoConstraints = YES;
	self.view.layer.cornerRadius = 10;
	self.view.layer.masksToBounds = true;
	self.view.layer.borderWidth = 2;
	self.view.layer.borderColor = UIColor.blackColor.CGColor;

}

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

	CGRect frame = game.view.frame;

	CGPoint origin = CGPointMake(0, self.rootView.frame.size.height - game.view.frame.size.height - 10);
	frame.origin = origin;
	game.view.frame = frame;
	game.view.tag = 667;

	[self.rootView addSubview:game.view];
}

-(IBAction)reset:(id)sender {
	machine_reset();
}

-(UIView *)rootView {
	SDLUIKitDelegate *delegate = [SDLUIKitDelegate sharedAppDelegate];
	UIWindow *window = [delegate window];
	return window.rootViewController.view;
}

@end
