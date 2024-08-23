#import "FilterType.h"
#import <math.h>

@implementation FilterType

+ (FilterTypeEnum)filterTypeFromString:(NSString *)type; {
    if ([type isEqualToString:@"lowpass"]) {
        return FilterTypeLowpass;
    }

    if ([type isEqualToString:@"highpass"]) {
        return FilterTypeHighpass;
    }
    
    if ([type isEqualToString:@"bandpass"]) {
        return FilterTypeBandpass;
    }
    
    if ([type isEqualToString:@"lowshelf"]) {
        return FilterTypeLowshelf;
    }
    
    if ([type isEqualToString:@"highshelf"]) {
        return FilterTypeHighshelf;
    }
    
    if ([type isEqualToString:@"peaking"]) {
        return FilterTypePeaking;
    }
    
    if ([type isEqualToString:@"notch"]) {
        return FilterTypeNotch;
    }
    
    if ([type isEqualToString:@"allpass"]) {
        return FilterTypeAllpass;
    }

    @throw [NSException exceptionWithName:@"Invalid filter type" reason:@"You have to pick correct wave type: ['lowpass', 'highpass', 'bandpass', 'lowshelf', 'highshelf', 'peaking', 'notch', 'allpass']" userInfo:nil];
}

+ (NSString *)stringFromFilterType:(FilterTypeEnum)filterType {
    switch (filterType) {
        case FilterTypeLowpass:
            return @"lowpass";
        case FilterTypeHighpass:
            return @"highpass";
        case FilterTypeBandpass:
            return @"bandpass";
        case FilterTypeLowshelf:
            return @"lowshelf";
        case FilterTypeHighshelf:
            return @"highshelf";
        case FilterTypePeaking:
            return @"peaking";
        case FilterTypeNotch:
            return @"notch";
        case FilterTypeAllpass:
            return @"allpass";
        
        default:
            @throw [NSException exceptionWithName:@"Invalid wave type" reason:@"You have to pick correct wave type: ['lowpass', 'highpass', 'bandpass', 'lowshelf', 'highshelf', 'peaking', 'notch', 'allpass']" userInfo:nil];
    }

    return nil;
}

@end
