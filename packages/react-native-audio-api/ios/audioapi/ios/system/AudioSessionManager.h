#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <React/RCTBridgeModule.h>

@interface AudioSessionManager : NSObject

@property (nonatomic, weak) AVAudioSession *audioSession;

@property (nonatomic, assign) bool isActive;
@property (nonatomic, assign) bool hasDirtySettings;
@property (nonatomic, assign) AVAudioSessionMode sessionMode;
@property (nonatomic, assign) AVAudioSessionCategory sessionCategory;
@property (nonatomic, assign) AVAudioSessionCategoryOptions sessionOptions;
@property (nonatomic, assign) bool allowHapticsAndSystemSoundsDuringRecording;
@property (nonatomic, assign) bool shouldManageSession;

- (instancetype)init;
- (void)cleanup;
- (bool)configureAudioSession;
- (bool)reconfigureAudioSession;
- (void)markSettingsAsDirty;

- (NSNumber *)getDevicePreferredSampleRate;
- (void)setAudioSessionOptions:(NSString *)category
                          mode:(NSString *)mode
                       options:(NSArray *)options
                  allowHaptics:(BOOL)allowHaptics;
- (bool)setActive:(bool)active;
- (void)disableSessionManagement;

- (void)requestRecordingPermissions:(RCTPromiseResolveBlock)resolve reject:(RCTPromiseRejectBlock)reject;
- (void)checkRecordingPermissions:(RCTPromiseResolveBlock)resolve reject:(RCTPromiseRejectBlock)reject;

- (void)getDevicesInfo:(RCTPromiseResolveBlock)resolve reject:(RCTPromiseRejectBlock)reject;
- (NSArray<NSDictionary *> *)parseDeviceList:(NSArray<AVAudioSessionPortDescription *> *)devices;

@end
