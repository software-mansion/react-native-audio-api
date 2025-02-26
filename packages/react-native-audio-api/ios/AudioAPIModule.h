#ifdef RCT_NEW_ARCH_ENABLED
#import <React/RCTCallInvokerModule.h>
#import <rnaudioapi/rnaudioapi.h>
#else // RCT_NEW_ARCH_ENABLED
#import <React/RCTBridgeModule.h>
#endif // RCT_NEW_ARCH_ENABLED

#import <React/RCTEventEmitter.h>

@interface AudioAPIModule : RCTEventEmitter
#ifdef RCT_NEW_ARCH_ENABLED
                            <NativeAudioAPIModuleSpec, RCTCallInvokerModule>
#else
                            <RCTBridgeModule>
#endif // RCT_NEW_ARCH_ENABLED
@end
