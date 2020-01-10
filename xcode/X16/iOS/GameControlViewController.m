//
//	GameControlViewController.m
//	CommanderX16
//
//	; (C)2020 Matthew Pearce, License: 2-clause BSD
//

#import "GameControlViewController.h"
#import <UIKit/UIKit.h>
#include <SDL.h>
#include "SDL_keyboard.h"
#include "keyboard.h"
#include "CommanderX16-Swift.h"

@import JoyStickView;

@interface GameControlViewController () <JoystickControllerDelegate>

int SDL_SendKeyboardKey(Uint8 state, SDL_Scancode scancode);
@property (nonatomic, weak) IBOutlet UIView *holder;
@property (nonatomic, strong) JoystickController *joystickController;

-(IBAction)buttonPressed:(id)sender;
-(IBAction)closeWindow:(id)sender;

@end

@implementation GameControlViewController

@synthesize holder, joystickController;

-(void)viewDidLoad {
	[super viewDidLoad];

	self.view.translatesAutoresizingMaskIntoConstraints = YES;
	joystickController = [[JoystickController alloc] init];
	CGRect frame = holder.frame;
	JoyStickView *joystickView = [joystickController getJoyStickForDelegate:self];
	joystickView.frame = frame;
	[self.view addSubview:joystickView];
}

-(IBAction)buttonPressed:(id)sender
{
	UIButton *button = (UIButton *)sender;
	handle_keyboard(true, 0, button.tag);
}

-(IBAction)closeWindow:(id)sender
{
	[self.view removeFromSuperview];
}

-(void)joystickPositionUpdatedWithAngle:(CGFloat)angle displacement:(CGFloat)displacement {

	// reset cursor keys
	handle_keyboard(false, 0,  SDL_SCANCODE_UP);
	handle_keyboard(false, 0,  SDL_SCANCODE_DOWN);
	handle_keyboard(false, 0,  SDL_SCANCODE_RIGHT);
	handle_keyboard(false, 0,  SDL_SCANCODE_LEFT);

	if (displacement == 0)
	{
		return;
	}
	if (angle > 300 || angle <= 30 ) {
		handle_keyboard(true, 0,  SDL_SCANCODE_UP);
	}
	if (angle > 120 && angle <= 240 ) {
		handle_keyboard(true, 0,  SDL_SCANCODE_DOWN);
	}
	if (angle > 30 && angle <= 120) {
		handle_keyboard(true, 0,  SDL_SCANCODE_RIGHT);
	}
	if (angle > 240 && angle <= 300) {
		handle_keyboard(true, 0,  SDL_SCANCODE_LEFT);
	}

}

@end
