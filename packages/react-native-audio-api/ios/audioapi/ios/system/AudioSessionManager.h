#pragma once

#import <TargetConditionals.h>
#import <Foundation/Foundation.h>
#if !TARGET_OS_MACCATALYST
#import <AVFoundation/AVFoundation.h>
#else
@class AVAudioSession;
@class AVAudioSessionPortDescription;
#endif
#import <React/RCTBridgeModule.h>

@interface AudioSessionManager : NSObject

@property (nonatomic, weak) AVAudioSession *audioSession;

// State tracking
@property (nonatomic, assign) bool isActive;
@property (nonatomic, assign) bool shouldManageSession;

// Session configuration options (desired by user)
#if TARGET_OS_MACCATALYST
@property (nonatomic, copy) NSString *desiredMode;
@property (nonatomic, copy) NSString *desiredCategory;
@property (nonatomic, assign) NSUInteger desiredOptions;
#else
@property (nonatomic, assign) AVAudioSessionMode desiredMode;
@property (nonatomic, assign) AVAudioSessionCategory desiredCategory;
@property (nonatomic, assign) AVAudioSessionCategoryOptions desiredOptions;
#endif
@property (nonatomic, assign) bool allowHapticsAndSounds;
@property (nonatomic, assign) bool notifyOthersOnDeactivation;

- (instancetype)init;
+ (instancetype)sharedInstance;

- (void)cleanup;

- (void)setAudioSessionOptions:(NSString *)category
                          mode:(NSString *)mode
                       options:(NSArray *)options
                  allowHaptics:(BOOL)allowHaptics
    notifyOthersOnDeactivation:(BOOL)notifyOthersOnDeactivation;

- (bool)configureAudioSession;
- (bool)setActive:(bool)active error:(NSError **)error;
- (void)markInactive;
- (void)disableSessionManagement;

- (NSNumber *)getDevicePreferredSampleRate;
- (NSNumber *)getDevicePreferredInputChannelCount;

- (void)requestRecordingPermissions:(RCTPromiseResolveBlock)resolve
                             reject:(RCTPromiseRejectBlock)reject;
- (NSString *)requestRecordingPermissions;

- (void)checkRecordingPermissions:(RCTPromiseResolveBlock)resolve
                           reject:(RCTPromiseRejectBlock)reject;
- (NSString *)checkRecordingPermissions;

- (void)getDevicesInfo:(RCTPromiseResolveBlock)resolve reject:(RCTPromiseRejectBlock)reject;
- (NSArray<NSDictionary *> *)parseDeviceList:(NSArray<AVAudioSessionPortDescription *> *)devices;
- (void)setInputDevice:(NSString *)deviceId
               resolve:(RCTPromiseResolveBlock)resolve
                reject:(RCTPromiseRejectBlock)reject;

- (bool)isSessionActive;

@end
