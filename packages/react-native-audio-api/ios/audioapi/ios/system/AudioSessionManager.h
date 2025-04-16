#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

@interface AudioSessionManager : NSObject

@property (nonatomic, weak) AVAudioSession *audioSession;

@property (nonatomic, assign) AVAudioSessionMode sessionMode;
@property (nonatomic, assign) AVAudioSessionCategory sessionCategory;
@property (nonatomic, assign) AVAudioSessionCategoryOptions sessionOptions;

+ (instancetype)sharedInstance;
- (void)cleanup;
- (bool)configureAudioSession;

- (NSNumber *)getDevicePreferredSampleRate;
- (void)setAudioSessionOptions:(NSString *)category mode:(NSString *)mode options:(NSArray *)options;
- (bool)setActive:(bool)active error:(NSError **)error;

@end
