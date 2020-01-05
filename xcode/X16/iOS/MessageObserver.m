//
//  MessageObserver.m
//  CommanderX16
//
//  Created by Pearce, Matthew (Senior Developer) on 04/01/2020.
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
        
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(loadFile:) name:@"FileSaved" object:nil];
    }
    
    return self;
}

-(int)loadFile:(NSNotification *)notification {
    NSString *filePath = (NSString *)notification.object;
    
    //bin file
    if ([[filePath uppercaseString] rangeOfString:@".BIN"].location != NSNotFound) {
        return 0;
    }
    
    if ([[filePath uppercaseString] rangeOfString:@".PRG"].location == NSNotFound) {
        if ([[filePath uppercaseString] rangeOfString:@".BAS"].location == NSNotFound) {
            return 1;
        } else {
            return loadBasFile([filePath cStringUsingEncoding:NSUTF8StringEncoding]);
        }
    } else {
      return loadPrgFile([filePath cStringUsingEncoding:NSUTF8StringEncoding]);
    }
    
    return 1;
}

-(void)showSettings {
    
    if (!settings) {
        settings = [[SettingsViewController alloc] initWithNibName:@"SettingsViewController" bundle:nil];
    } else {
        [[[self rootView] viewWithTag:666] removeFromSuperview];
    }
    
    CGRect rect = settings.view.frame;
    
    CGPoint origin = CGPointMake(0, 0);
    rect.origin = origin;
    settings.view.frame = rect;
    
    settings.view.tag = 666;
    
    [[self rootView] addSubview:settings.view];
}

-(UIView *)rootView {
    SDLUIKitDelegate *delegate = [SDLUIKitDelegate sharedAppDelegate];
    UIWindow *window = [delegate window];
    return [window rootViewController].view;
}


@end
