#pragma once

#import <Foundation/Foundation.h>

FOUNDATION_EXPORT NSErrorDomain const AudioAPIErrorDomain;

// TODO: add more granular lint rules selection depending on source type
// NOLINTNEXTLINE(modernize-use-using, cppcoreguidelines-use-enum-class, performance-enum-size)
typedef NS_ERROR_ENUM(AudioAPIErrorDomain, AudioAPIErrorCode){
    AudioAPIErrorCodeEngineHasNoGraph = 1,
    AudioAPIErrorCodeSessionActivationFailed,
    AudioAPIErrorCodeSessionActivityChangeFailed,
    AudioAPIErrorCodeInputFormatUnavailable,
    AudioAPIErrorCodeEngineStartFailed,
};

/// @brief Builds the errors the iOS layer reports itself, one constructor per failure, so a code
/// always comes with the same description.
@interface AudioAPIError : NSObject

+ (NSError *)engineHasNoGraph;

/// @param systemError The error AVAudioSession returned while activating the session.
+ (NSError *)sessionActivationFailed:(NSError *)systemError;

/// @param systemError The error AVAudioSession returned from `setActive:`, for either direction.
+ (NSError *)sessionActivityChangeFailed:(NSError *)systemError;

+ (NSError *)inputFormatUnavailable;

/// @param systemError The error AVAudioEngine returned from `startAndReturnError:`.
+ (NSError *)engineStartFailed:(NSError *)systemError;

/// @brief Logs `failure` and hands it to the caller, so a method reports a failure in one line:
/// `return [AudioAPIError fail:outError with:...];`.
/// @param outError Receives `failure`; may be nil when the caller does not want the error.
/// @param failure The error to log and report.
/// @return Always NO.
/// @note Call this where the failure happens. A method relaying a callee's error passes its own
/// `outError` to the callee instead, so each error is logged once.
+ (BOOL)fail:(NSError **)outError with:(NSError *)failure;

/// @brief Formats `error` and each error under it as `domain code: description`, outermost first,
/// joined by ` <- `.
/// @return The text the recorder sends to JS as its error message.
+ (NSString *)describe:(NSError *)error;

@end
