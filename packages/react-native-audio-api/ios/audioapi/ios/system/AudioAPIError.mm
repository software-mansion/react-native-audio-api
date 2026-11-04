#import <audioapi/ios/system/AudioAPIError.h>

NSErrorDomain const AudioAPIErrorDomain = @"AudioAPIErrorDomain";

@implementation AudioAPIError

+ (NSError *)engineHasNoGraph
{
  return [self createErrorWithCode:AudioAPIErrorCodeEngineHasNoGraph
                       description:@"Audio engine has no nodes to start"
                       systemError:nil];
}

+ (NSError *)sessionActivationFailed:(NSError *)systemError
{
  return [self createErrorWithCode:AudioAPIErrorCodeSessionActivationFailed
                       description:@"Audio session could not be activated"
                       systemError:systemError];
}

+ (NSError *)sessionActivityChangeFailed:(NSError *)systemError
{
  return [self createErrorWithCode:AudioAPIErrorCodeSessionActivityChangeFailed
                       description:@"Failed to set audio session active state"
                       systemError:systemError];
}

+ (NSError *)inputFormatUnavailable
{
  return [self
      createErrorWithCode:AudioAPIErrorCodeInputFormatUnavailable
              description:@"Audio input node could not be materialized: missing live input format"
              systemError:nil];
}

+ (NSError *)engineStartFailed:(NSError *)systemError
{
  return [self createErrorWithCode:AudioAPIErrorCodeEngineStartFailed
                       description:@"Audio engine could not be started"
                       systemError:systemError];
}

+ (BOOL)fail:(NSError **)outError with:(NSError *)failure
{
  NSLog(@"%@", [self describe:failure]);

  if (outError != nil) {
    *outError = failure;
  }

  return NO;
}

+ (NSString *)describe:(NSError *)error
{
  NSMutableArray<NSString *> *descriptions = [NSMutableArray array];

  for (NSError *cause = error; cause != nil; cause = cause.underlyingErrors.firstObject) {
    [descriptions addObject:[NSString stringWithFormat:@"%@ %@: %@",
                                                       cause.domain,
                                                       @(cause.code),
                                                       cause.localizedDescription]];
  }

  return [descriptions componentsJoinedByString:@" <- "];
}

+ (NSError *)createErrorWithCode:(AudioAPIErrorCode)code
                     description:(NSString *)description
                     systemError:(NSError *)systemError
{
  NSMutableDictionary *userInfo = [@{NSLocalizedDescriptionKey : description} mutableCopy];

  if (systemError != nil) {
    userInfo[NSUnderlyingErrorKey] = systemError;
    userInfo[@"meta"] = [self getNativeMetadataOf:systemError];
  }

  return [NSError errorWithDomain:AudioAPIErrorDomain code:code userInfo:userInfo];
}

// parseNativeError in src/system/errors.ts reads these keys.
+ (NSDictionary<NSString *, id> *)getNativeMetadataOf:(NSError *)error
{
  return @{
    @"nativeCode" : @(error.code),
    @"nativeDomain" : error.domain,
    @"nativeDesc" : error.description,
  };
}

@end
