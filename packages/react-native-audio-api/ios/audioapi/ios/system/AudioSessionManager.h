#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <React/RCTBridgeModule.h>

@interface AudioSessionManager : NSObject

@property (nonatomic, weak) AVAudioSession *audioSession;

// State tracking
@property (nonatomic, assign) bool isActive;
@property (nonatomic, assign) bool shouldManageSession;

// Session configuration options (desired by user)
@property (nonatomic, assign) AVAudioSessionMode desiredMode;
@property (nonatomic, assign) AVAudioSessionCategory desiredCategory;
@property (nonatomic, assign) AVAudioSessionCategoryOptions desiredOptions;
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
/// Ensures the library-managed session is active. In external owner mode this is a no-op.
- (bool)ensureActive:(bool)force error:(NSError **)error;
- (bool)setActive:(bool)active error:(NSError **)error;
/// Drops the cached active-state flag without reclaiming external ownership.
- (void)markInactive;
/// Switches the manager into external owner mode and stops all session mutations.
- (void)disableSessionManagement;

- (NSNumber *)getDevicePreferredSampleRate;
- (NSString *)inputDiagnosticsSnapshot;

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

@end
