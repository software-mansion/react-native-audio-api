#import "NativeAudioWorkletsModuleProvider.h"

#import <ReactCommon/CallInvoker.h>
#import <ReactCommon/TurboModule.h>
#import <audioworklets/NativeAudioWorkletsModule.h>

@implementation NativeAudioWorkletsModuleProvider

- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
    (const facebook::react::ObjCTurboModule::InitParams &)params
{
  return std::make_shared<facebook::react::NativeAudioWorkletsModule>(params.jsInvoker);
}

@end
