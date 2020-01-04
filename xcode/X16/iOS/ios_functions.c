//
//  ios_functions.c
//  CommanderX16
//
//  Created by Pearce, Matthew (Senior Developer) on 04/01/2020.
//

#include "ios_functions.h"
#if __APPLE__
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
// iOS device
#define OBJC_OLD_DISPATCH_PROTOTYPES 1

#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
#elif TARGET_OS_MAC
// Other kinds of Mac OS
#else
#   error "Unknown Apple platform"
#endif
#endif

void createIosMessageObserver(void) {
    SEL alloc = sel_registerName("alloc");
    SEL init = sel_registerName("init");
    id MessageObserver = (id)objc_getClass("MessageObserver");
    id tmp = objc_msgSend(MessageObserver, alloc);
    objc_msgSend(tmp, init);
}

void sendNotification(const char *notification_name) {
    
    SEL alloc = sel_registerName("alloc");
    id NSString = (id)objc_getClass("NSString");
    SEL initWithCString_encoding = sel_registerName("initWithCString:encoding:");
    int NSUTF8StringEncoding = 4;
    id tmp2 = objc_msgSend(NSString, alloc);
    id notificationName = objc_msgSend(tmp2, initWithCString_encoding,
                                       notification_name, NSUTF8StringEncoding);
    id NSNotificationCenter = (id)objc_getClass("NSNotificationCenter");
    
    SEL defaultCenter = sel_registerName("defaultCenter");
    SEL postNotificationName = sel_registerName("postNotificationName:object:");
    
    id tmp = objc_msgSend(NSNotificationCenter, defaultCenter);
    
    objc_msgSend(tmp, postNotificationName, notificationName, NULL);
    
}
