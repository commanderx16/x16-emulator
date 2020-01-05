//
//  ios_functions.c
//  CommanderX16
//
//  ; (C)2020 Matthew Pearce, License: 2-clause BSD
//

#include "ios_functions.h"
#if __APPLE__
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
// iOS device
#define OBJC_OLD_DISPATCH_PROTOTYPES 1
#import <Foundation/Foundation.h>
#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
#import <UIKit/UIKit.h>
#include "SDL.h"
#include "SDL_uikitappdelegate.h"

#import "MessageObserver.h"
#elif TARGET_OS_MAC
// Other kinds of Mac OS
#else
#   error "Unknown Apple platform"
#endif
#endif
#include "utf8_encode.h"
#include <string.h>
#include <stdlib.h>

void createIosMessageObserver(void) {
    SEL alloc = sel_registerName("alloc");
    SEL init = sel_registerName("init");
    id messageObserver = (id)objc_getClass("MessageObserver");
    id tmp = objc_msgSend(messageObserver, alloc);
    objc_msgSend(tmp, init);
    
    SDLUIKitDelegate *delegate = [SDLUIKitDelegate sharedAppDelegate];
    UIWindow *window = [delegate window];
    UIView *rootView = [window rootViewController].view;
    
    MessageObserver *observer = (MessageObserver *)tmp;
    UILongPressGestureRecognizer *longPress = [[UILongPressGestureRecognizer alloc] initWithTarget:observer action:@selector(showSettings)];
    [rootView addGestureRecognizer: longPress];
}

void sendNotification(const char *notification_name) {
    NSString *notificationName = [NSString stringWithCString:notification_name encoding:NSUTF8StringEncoding];
    [[NSNotificationCenter defaultCenter] postNotificationName:notificationName object:nil];
}

void receiveFile(char* dropped_filedir) {
    
    NSURL *fileURL = [NSURL URLWithString:[NSString stringWithCString:dropped_filedir encoding:NSUTF8StringEncoding]];
    
    NSString *path = NSHomeDirectory();
    path = [path stringByAppendingPathComponent:@"Documents"];
    path = [path stringByAppendingPathComponent:fileURL.lastPathComponent];
    
    NSError *error;
    
    //remove old file.
    [[NSFileManager defaultManager] removeItemAtPath:path error:&error];
    [[NSFileManager defaultManager] moveItemAtPath:fileURL.path toPath:path error:&error];
    
    //bin file
    
    NSString *filePath = [path uppercaseString];
    if ([filePath rangeOfString:@".BIN"].location != NSNotFound) {
        return;
    }
    
    if ([filePath rangeOfString:@".PRG"].location == NSNotFound) {
        if ([filePath rangeOfString:@".BAS"].location == NSNotFound) {
            return;
        } else {
            loadBasFile([path cStringUsingEncoding:NSUTF8StringEncoding]);
        }
    } else {
        loadPrgFile([path cStringUsingEncoding:NSUTF8StringEncoding]);
    }
}

int loadBasFile(const char *bas_path) {
    bool pasting_bas = false;
    char paste_text_data[65536];
    char *paste_text = NULL;
    
    FILE *bas_file = fopen(bas_path, "r");
    if (!bas_file) {
        printf("Cannot open %s!\n", bas_path);
        exit(1);
    }
    paste_text = paste_text_data;
    size_t paste_size = fread(paste_text, 1, sizeof(paste_text_data) - 1, bas_file);
    paste_text[paste_size] = 0;

    fclose(bas_file);
    
    if (paste_text) {
        // ...paste BASIC code into the keyboard buffer
        pasting_bas = true;
    }
    
    while (pasting_bas) {
        uint32_t c;
        int e = 0;

    while (pasting_bas && RAM[NDX] < 10) {

        if (paste_text[0] == '\\' && paste_text[1] == 'X' && paste_text[2] && paste_text[3]) {
            uint8_t hi = strtol((char[]){paste_text[2], 0}, NULL, 16);
            uint8_t lo = strtol((char[]){paste_text[3], 0}, NULL, 16);
            c = hi << 4 | lo;
            paste_text += 4;
        } else {
            paste_text = utf8_decode(paste_text, &c, &e);
            
            c = iso8859_15_from_unicode(c);
        }
        if (c && !e) {
            RAM[KEYD + RAM[NDX]] = c;
            RAM[NDX]++;
        } else {
            pasting_bas = false;
            paste_text = NULL;
        }
        }
    }
    
    return 0;
}

int loadPrgFile(const char *prg_path) {
    bool pasting_bas = false;
    char paste_text_data[65536];
    char *paste_text = NULL;
    
    if (!prg_path) {
        return 1;
    }
    
    FILE *prg_file ;
    int prg_override_start = -1;
    
    char *comma = strchr(prg_path, ',');
    if (comma) {
        prg_override_start = (uint16_t)strtol(comma + 1, NULL, 16);
        *comma = 0;
    }

    prg_file = fopen(prg_path, "rb");
    if (!prg_file) {
        printf("Cannot open %s!\n", prg_path);
        return 1;
    }
    
    uint8_t start_lo = fgetc(prg_file);
    uint8_t start_hi = fgetc(prg_file);
    uint16_t start;
    if (prg_override_start >= 0) {
        start = prg_override_start;
    } else {
        start = start_hi << 8 | start_lo;
    }
    uint16_t end = start + fread(RAM + start, 1, 65536-start, prg_file);
    fclose(prg_file);
    prg_file = NULL;
    if (start == 0x0801) {
        // set start of variables
        
        RAM[VARTAB] = end & 0xff;
        RAM[VARTAB + 1] = end >> 8;
        
        paste_text = "RUN";
    } else {
        paste_text = paste_text_data;
        snprintf(paste_text, sizeof(paste_text_data), "SYS$%04X", start);
    }

    if (paste_text) {
        // ...paste BASIC code into the keyboard buffer
        pasting_bas = true;
    }
    
    while (pasting_bas && RAM[NDX] < 10) {
        uint32_t c;
        int e = 0;

        if (paste_text[0] == '\\' && paste_text[1] == 'X' && paste_text[2] && paste_text[3]) {
            uint8_t hi = strtol((char[]){paste_text[2], 0}, NULL, 16);
            uint8_t lo = strtol((char[]){paste_text[3], 0}, NULL, 16);
            c = hi << 4 | lo;
            paste_text += 4;
        } else {
            paste_text = utf8_decode(paste_text, &c, &e);
            c = iso8859_15_from_unicode(c);
        }
        if (c && !e) {
            RAM[KEYD + RAM[NDX]] = c;
            RAM[NDX]++;
        } else {
            pasting_bas = false;
            paste_text = NULL;
        }
    }
    
    return 0;
}

UIView* rootView(void) {
    SDLUIKitDelegate *delegate = [SDLUIKitDelegate sharedAppDelegate];
    UIWindow *window = [delegate window];
    return [window rootViewController].view;
}
