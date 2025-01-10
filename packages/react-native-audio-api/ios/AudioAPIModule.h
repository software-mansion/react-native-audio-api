#ifdef RCT_NEW_ARCH_ENABLED
#import <rnaudioapi/rnaudioapi.h>
#import <React/RCTCallInvokerModule.h>
#else // RCT_NEW_ARCH_ENABLED
#import <React/RCTBridgeModule.h>
#endif // RCT_NEW_ARCH_ENABLED

#import <React/RCTEventEmitter.h>

@interface AudioAPIModule : RCTEventEmitter
#ifdef RCT_NEW_ARCH_ENABLED
<RCTCallInvokerModule>
#else
<RCTBridgeModule>
#endif // RCT_NEW_ARCH_ENABLED
@end
