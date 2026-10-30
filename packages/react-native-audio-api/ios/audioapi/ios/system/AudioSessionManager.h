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

- (bool)configureAudioSession:(NSError **)outError;
/// Ensures the library-managed session is active. In external owner mode this is a no-op.
- (bool)ensureActive:(bool)force error:(NSError **)error;
- (bool)setActive:(bool)active error:(NSError **)error;
/// Drops the cached active-state flag without reclaiming external ownership.
- (void)markInactive;
/// Switches the manager into external owner mode and stops all session mutations.
- (void)disableSessionManagement;

/// Asks the session for an IO buffer of @p frames (in frames of the session's own rate).
/// One buffer duration serves the whole process and nothing is tracked per caller: the latest
/// request simply replaces the previous one.
- (void)setPreferredIOBufferFrames:(int)frames;
/// Asks for one render quantum again, the default of an interactive context.
- (void)resetPreferredIOBufferFrames;

- (NSNumber *)getDevicePreferredSampleRate;
- (NSNumber *)getSystemVolume;
- (NSString *)inputDiagnosticsSnapshot;

- (void)requestRecordingPermissions:(RCTPromiseResolveBlock)resolve
                             reject:(RCTPromiseRejectBlock)reject;
- (NSString *)requestRecordingPermissions;

- (void)checkRecordingPermissions:(RCTPromiseResolveBlock)resolve
                           reject:(RCTPromiseRejectBlock)reject;
- (NSString *)checkRecordingPermissions;

- (void)getDevicesInfo:(RCTPromiseResolveBlock)resolve reject:(RCTPromiseRejectBlock)reject;
- (NSArray<NSDictionary *> *)parseDeviceList:(NSArray<AVAudioSessionPortDescription *> *)devices;
/// A nil @p deviceId clears the preferred input, so the system chooses again.
- (void)setInputDevice:(NSString *)deviceId
               resolve:(RCTPromiseResolveBlock)resolve
                reject:(RCTPromiseRejectBlock)reject;

- (double)outputLatencySeconds;
- (double)inputLatencySeconds;
- (double)ioBufferDurationSeconds;

@end
