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
	SDL_SendKeyboardKey(1, button.tag);
}

-(IBAction)closeWindow:(id)sender
{
	[self.view removeFromSuperview];
}

-(void)joystickPositionUpdatedWithAngle:(CGFloat)angle displacement:(CGFloat)displacement {
	if (displacement == 0)
		{
			return;
		}
		if (angle > 300 || angle <= 30 ) {
			SDL_SendKeyboardKey(1, SDL_SCANCODE_UP);
			SDL_SendKeyboardKey(0, SDL_SCANCODE_UP);
		}

		if (angle > 120 && angle <= 240 ) {
			SDL_SendKeyboardKey(1, SDL_SCANCODE_DOWN);
			SDL_SendKeyboardKey(0, SDL_SCANCODE_DOWN);
		}

		if (angle > 30 && angle <= 120) {
			SDL_SendKeyboardKey(1, SDL_SCANCODE_RIGHT);
			SDL_SendKeyboardKey(0, SDL_SCANCODE_RIGHT);
		}

		if (angle > 240 && angle <= 300) {
			SDL_SendKeyboardKey(1, SDL_SCANCODE_LEFT);
			SDL_SendKeyboardKey(0, SDL_SCANCODE_LEFT);
		}
}

@end
