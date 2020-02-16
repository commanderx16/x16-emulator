//
//	InputViewController.m
//	CommanderX16
//
//	; (C)2020 Matthew Pearce, License: 2-clause BSD
//	icons by Icons8

#import "InputViewController.h"
#import <GameController/GameController.h>
#import "GameControlViewController.h"
#include "SDL.h"
#include "keyboard.h"
#include "ios_functions.h"
#import "SDL_uikitappdelegate.h"

@interface InputViewController ()
int SDL_SendKeyboardKey(Uint8 state, SDL_Scancode scancode);
-(IBAction)buttonPressed:(id)sender;
-(IBAction)closeWindow:(id)sender;
-(IBAction)toggleKeyboard:(id)sender;
-(IBAction)showGamePad:(id)sender;
-(IBAction)reset:(id)sender;
-(IBAction)startControllerSearch:(id)sender;

@property (nonatomic, retain) GameControlViewController *game;
@property (nonatomic, retain) GCController *controller;
@end

@implementation InputViewController

@synthesize game, controller;



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


//MARk: Wireless controller

-(void)deviceConnected {
	[GCController stopWirelessControllerDiscovery];
	[self dismissViewControllerAnimated:true completion:nil];

	controller = [GCController.controllers firstObject];
	NSString *message = [NSString stringWithFormat:@"%@ connected", controller.vendorName];

	UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Wireless Controller" message:message preferredStyle:UIAlertControllerStyleAlert];
	UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction * action) {
	}];

	[alert addAction:defaultAction];
	[self presentViewController:alert animated:YES completion:nil];

	if (controller.extendedGamepad) {
		[self.controller.extendedGamepad.buttonA setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
			handle_keyboard(pressed, 0, 224);
		}];
		[self.controller.extendedGamepad.buttonB setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
			handle_keyboard(pressed, 0, 226);
		}];
		[self.controller.extendedGamepad.buttonX setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
			handle_keyboard(pressed, 0, 40); //start
		}];
		[self.controller.extendedGamepad.buttonY setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
			handle_keyboard(pressed, 0, 44); //select
		}];

		[self.controller.extendedGamepad.dpad.left setValueChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed) {
			handle_keyboard(pressed, 0, SDL_SCANCODE_LEFT);
		}];
		[self.controller.extendedGamepad.dpad.right setValueChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed) {
			handle_keyboard(pressed, 0, SDL_SCANCODE_RIGHT);
		}];
		[self.controller.extendedGamepad.dpad.up setValueChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed) {
			handle_keyboard(pressed, 0, SDL_SCANCODE_UP);
		}];
		[self.controller.extendedGamepad.dpad.down setValueChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed) {
			handle_keyboard(pressed, 0, SDL_SCANCODE_DOWN);
		}];

	} else if (controller.microGamepad) {

		[self.controller.microGamepad.buttonA setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
			handle_keyboard(pressed, 0, 224);
		}];
		[self.controller.microGamepad.buttonX setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
			handle_keyboard(pressed, 0, 40); //start
		}];
		[self.controller.microGamepad.dpad.left setValueChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed) {
			handle_keyboard(pressed, 0, SDL_SCANCODE_LEFT);
		}];
		[self.controller.microGamepad.dpad.right setValueChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed) {
			handle_keyboard(pressed, 0, SDL_SCANCODE_RIGHT);
		}];
		[self.controller.microGamepad.dpad.up setValueChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed) {
			handle_keyboard(pressed, 0, SDL_SCANCODE_UP);
		}];
		[self.controller.microGamepad.dpad.down setValueChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed) {
			handle_keyboard(pressed, 0, SDL_SCANCODE_DOWN);
		}];
	}
}

-(void)deviceDisonnected {

	UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Wireless Controller" message:@"Controller Disconnected\n\n\n" preferredStyle:UIAlertControllerStyleAlert];
	UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction * action) {
	}];

	[alert addAction:defaultAction];
	[self presentViewController:alert animated:YES completion:nil];
}

-(IBAction)startControllerSearch:(id)sender {

	if (GCController.controllers.count > 0) {
		[self deviceConnected];
		return;
	}

	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(deviceConnected) name:GCControllerDidConnectNotification object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(deviceDisconnected) name:GCControllerDidDisconnectNotification object:nil];

	UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Wireless Controller" message:@"Searching for connected controllers\n\n\n" preferredStyle:UIAlertControllerStyleAlert];

	UIActivityIndicatorView* indicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge];
	indicator.color = [UIColor blackColor];
	indicator.translatesAutoresizingMaskIntoConstraints=NO;
	[alert.view addSubview:indicator];
	NSDictionary * views = @{@"alert" : alert.view, @"indicator" : indicator};

	NSArray * constraintsVertical = [NSLayoutConstraint constraintsWithVisualFormat:@"V:[indicator]-(50)-|" options:0 metrics:nil views:views];
	NSArray * constraintsHorizontal = [NSLayoutConstraint constraintsWithVisualFormat:@"H:|[indicator]|" options:0 metrics:nil views:views];
	NSArray * constraints = [constraintsVertical arrayByAddingObjectsFromArray:constraintsHorizontal];
	[alert.view addConstraints:constraints];
	[indicator setUserInteractionEnabled:NO];
	[indicator startAnimating];

	UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:^(UIAlertAction * action) {
		[GCController stopWirelessControllerDiscovery];
	}];

	[alert addAction:defaultAction];
	[self presentViewController:alert animated:YES completion:nil];

	[GCController startWirelessControllerDiscoveryWithCompletionHandler:^{
		[self dismissViewControllerAnimated:true completion:nil];
	}];

}

@end
