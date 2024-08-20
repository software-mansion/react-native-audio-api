#import <Foundation/Foundation.h>

@interface ParamChange : NSObject

@property (nonatomic) double startTime;
@property (nonatomic) double endTime;
@property (nonatomic) double startValue;
@property (nonatomic) double endValue;
@property (nonatomic, weak) id target;
@property (nonatomic) SEL calculateSelector;

- (instancetype)init:(double)startTime endTime:(double)endTime startValue:(double)startValue endValue:(double)endValue target:(id)target calculateSelector:(SEL)calculateSelector;

- (double)getValueAtTime:(double)time;

@end
