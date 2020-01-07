//
//	MessageObserver.m
//	CommanderX16
//
//	; (C)2020 Matthew Pearce, License: 2-clause BSD
//

#import "MessageObserver.h"
#import "SettingsViewController.h"
#import "SDL.h"
#import "SDL_uikitappdelegate.h"
#include "ios_functions.h"

@interface MessageObserver ()
@property (nonatomic, retain) SettingsViewController *settings;
@end

@implementation MessageObserver

@synthesize settings;

-(id)init {

	if (self == [super init]) {
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(showSettings) name:@"ShowKeyboard" object:nil];
	}

	return self;
}

-(void)showSettings {

	if (!settings) {
		settings = [[SettingsViewController alloc] initWithNibName:@"SettingsViewController" bundle:nil];
	} else {
		[[self.rootView viewWithTag:666] removeFromSuperview];
	}

	CGPoint origin = CGPointMake(self.rootView.frame.size.width - settings.view.frame.size.width - 20, 20);
	CGRect frame = settings.view.frame;
	frame.origin = origin;
	settings.view.frame = frame;
	settings.view.tag = 666;

	[self.rootView addSubview:settings.view];
}

-(UIView *)rootView {
	SDLUIKitDelegate *delegate = [SDLUIKitDelegate sharedAppDelegate];
	UIWindow *window = [delegate window];
	return window.rootViewController.view;
}

@end
