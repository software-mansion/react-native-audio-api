#pragma once

typedef NS_ENUM(NSInteger, FilterTypeEnum) {
    FilterTypeLowpass,
    FilterTypeHighpass,
    FilterTypeBandpass,
    FilterTypeLowshelf,
    FilterTypeHighshelf,
    FilterTypePeaking,
    FilterTypeNotch,
    FilterTypeAllpass
};

@interface FilterType : NSObject

+ (FilterTypeEnum)filterTypeFromString:(NSString *)type;
+ (NSString *)stringFromFilterType:(FilterTypeEnum)filterType;

@end
