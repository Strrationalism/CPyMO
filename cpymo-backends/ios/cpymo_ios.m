#import <Foundation/Foundation.h>

const char* get_ios_directory() {
    NSString *homeDir = NSHomeDirectory();
    NSLog(@"homeDir=%@", homeDir);
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *docDir = [paths objectAtIndex:0];
    return [docDir UTF8String];
}
