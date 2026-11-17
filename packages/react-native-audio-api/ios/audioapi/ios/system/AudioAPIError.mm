#import <audioapi/ios/system/AudioAPIError.h>

NSErrorDomain const AudioAPIErrorDomain = @"AudioAPIErrorDomain";

@implementation AudioAPIError

+ (NSError *)engineHasNoGraph
{
  return [self createErrorWithCode:AudioAPIErrorCodeEngineHasNoGraph systemError:nil];
}

+ (NSError *)sessionActivationFailed:(NSError *)systemError
{
  return [self createErrorWithCode:AudioAPIErrorCodeSessionActivationFailed
                       systemError:systemError];
}

+ (NSError *)sessionActivityChangeFailed:(NSError *)systemError
{
  return [self createErrorWithCode:AudioAPIErrorCodeSessionActivityChangeFailed
                       systemError:systemError];
}

+ (NSError *)inputFormatUnavailable
{
  return [self createErrorWithCode:AudioAPIErrorCodeInputFormatUnavailable systemError:nil];
}

+ (NSError *)engineStartFailed:(NSError *)systemError
{
  return [self createErrorWithCode:AudioAPIErrorCodeEngineStartFailed systemError:systemError];
}

+ (void)fail:(NSError **)outError with:(NSError *)failure
{
  NSLog(@"%@", [self describe:failure]);

  if (outError != nil) {
    *outError = failure;
  }
}

+ (NSString *)describe:(NSError *)error
{
  NSMutableString *description = [NSMutableString string];
  for (NSError *cause = error; cause != nil; cause = cause.underlyingErrors.firstObject) {
    if (description.length > 0) {
      [description appendString:@" <- "];
    }
    [description
        appendFormat:@"%@ %@: %@", cause.domain, @(cause.code), cause.localizedDescription];
  }
  return description;
}

+ (NSError *)createErrorWithCode:(AudioAPIErrorCode)code systemError:(NSError *)systemError
{
  NSMutableDictionary *userInfo =
      [@{NSLocalizedDescriptionKey : [self getDescriptionOf:code]} mutableCopy];

  if (systemError != nil) {
    userInfo[NSUnderlyingErrorKey] = systemError;
    userInfo[@"meta"] = [self getNativeMetadataOf:systemError];
  }

  return [NSError errorWithDomain:AudioAPIErrorDomain code:code userInfo:userInfo];
}

+ (NSString *)getDescriptionOf:(AudioAPIErrorCode)code
{
  switch (code) {
    case AudioAPIErrorCodeEngineHasNoGraph:
      return @"Audio engine has no nodes to start";
    case AudioAPIErrorCodeSessionActivationFailed:
      return @"Audio session could not be activated";
    case AudioAPIErrorCodeSessionActivityChangeFailed:
      return @"Failed to set audio session active state";
    case AudioAPIErrorCodeInputFormatUnavailable:
      return @"Audio input node could not be materialized: missing live input format";
    case AudioAPIErrorCodeEngineStartFailed:
      return @"Audio engine could not be started";
  }
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
