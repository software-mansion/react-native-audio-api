#ifdef RCT_NEW_ARCH_ENABLED
#import <React/RCTInitializing.h>
#if REACT_NATIVE_MINOR_VERSION >= 74
#import <React/RCTRuntimeExecutorModule.h>
#import <ReactCommon/RCTRuntimeExecutor.h>
#endif // REACT_NATIVE_MINOR_VERSION >= 74
#else // RCT_NEW_ARCH_ENABLED
#import <React/RCTBridgeModule.h>
#endif // RCT_NEW_ARCH_ENABLED

#ifndef HAVE_ACCELERATE
#define HAVE_ACCELERATE
#endif

#import <React/RCTEventEmitter.h>

@interface AudioAPIModule : RCTEventEmitter
#ifdef RCT_NEW_ARCH_ENABLED
                            <RCTInitializing
#if REACT_NATIVE_MINOR_VERSION >= 74
                             ,
                             RCTRuntimeExecutorModule
#endif // REACT_NATIVE_MINOR_VERSION >= 74
#else
                            <RCTBridgeModule
#endif // RCT_NEW_ARCH_ENABLED
                             >
@end
