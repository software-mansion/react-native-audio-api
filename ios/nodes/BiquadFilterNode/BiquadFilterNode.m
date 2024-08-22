#import "BiquadFilterNode.h"
#import "AudioContext.h"
#import "Constants.h"

// https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html - math formulas for filters
// https://github.com/LabSound/LabSound/blob/main/src/internal/src/Biquad.cpp - implementation of filters on which I based mine

@implementation BiquadFilterNode

- (instancetype)initWithContext:(AudioContext *)context {
    if (self = [super initWithContext:context]) {
        _frequencyParam = [[AudioParam alloc] initWithContext:context value:350 minValue:[Constants minFilterFrequency] maxValue:[Constants maxFilterFrequency]];
        _detuneParam = [[AudioParam alloc] initWithContext:context value:0 minValue:-[Constants maxDetune] maxValue:[Constants maxDetune]];
        _QParam = [[AudioParam alloc] initWithContext:context value:1 minValue:-[Constants maxFilterQ] maxValue:[Constants maxFilterQ]];
        _gainParam = [[AudioParam alloc] initWithContext:context value:0 minValue:[Constants minFilterGain] maxValue:[Constants maxGain]];
        self.numberOfInputs = 1;
        self.numberOfOutputs = 1;
        _filterType = FilterTypeLowpass;
    }
    
    [self resetCoefficients];
    
    return self;
}

- (void)resetCoefficients {
    _x1 = 0;
    _x2 = 0;
    _y1 = 0;
    _y2 = 0;
    
    _a0 = 1;
    _a1 = 0;
    _a2 = 0;
    _b0 = 1;
    _b1 = 0;
    _b2 = 0;
}

- (void)setNormalizedCoefficientsA0:(double)a0 a1:(double)a1 a2:(double)a2 b0:(double)b0 b1:(double)b1 b2:(double)b2 {
    self.a1 = a1 / a0;
    self.a2 = a2 / a0;
    self.b0 = b0 / a0;
    self.b1 = b1 / a0;
    self.b2 = b2 / a0;
}

- (void)cleanup {
    _frequencyParam = nil;
    _detuneParam = nil;
    _QParam = nil;
    _gainParam = nil;
}

- (void)setType:(NSString *)type {
    _filterType = [FilterType filterTypeFromString:type];
}

- (NSString *)getType {
    return [FilterType stringFromFilterType:self.filterType];
}

- (void)setLowpassCoefficientsWithFrequency:(double)frequency resonance:(double)resonance {
    double normalizedFrequency = fmax(0.0, fmin(frequency, 1.0));
    double Q = fmax(0.0, resonance);

    if (normalizedFrequency == 1.0) {
        [self setNormalizedCoefficientsA0:1.0 a1:0.0 a2:0.0 b0:1.0 b1:0.0 b2:0.0];
        return;
    }

    if (normalizedFrequency <= 0.0) {
        [self setNormalizedCoefficientsA0:1.0 a1:0.0 a2:0.0 b0:0.0 b1:0.0 b2:0.0];
        return;
    }

    double w0 = 2.0 * M_PI * normalizedFrequency;
    double alpha = sin(w0) / (2.0 * Q);

    double b0 = (1.0 - cos(w0)) / 2.0;
    double b1 = 1.0 - cos(w0);
    double b2 = (1.0 - cos(w0)) / 2.0;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cos(w0);
    double a2 = 1.0 - alpha;

    [self setNormalizedCoefficientsA0:a0 a1:a1 a2:a2 b0:b0 b1:b1 b2:b2];
}

- (void)setHighpassCoefficientsWithFrequency:(double)frequency resonance:(double)resonance {
    double normalizedFrequency = fmax(0.0, fmin(frequency, 1.0));
    double Q = fmax(0.0, resonance);

    if (normalizedFrequency == 1.0) {
        [self setNormalizedCoefficientsA0:1.0 a1:0.0 a2:0.0 b0:0.0 b1:0.0 b2:0.0];
        return;
    }

    if (normalizedFrequency <= 0.0) {
        [self setNormalizedCoefficientsA0:1.0 a1:0.0 a2:0.0 b0:1.0 b1:0.0 b2:0.0];
        return;
    }

    double w0 = 2.0 * M_PI * normalizedFrequency;
    double alpha = sin(w0) / (2.0 * Q);

    double b0 = (1.0 + cos(w0)) / 2.0;
    double b1 = -(1.0 + cos(w0));
    double b2 = (1.0 + cos(w0)) / 2.0;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cos(w0);
    double a2 = 1.0 - alpha;

    [self setNormalizedCoefficientsA0:a0 a1:a1 a2:a2 b0:b0 b1:b1 b2:b2];
}

- (void)setBandpassCoefficientsWithFrequency:(double)frequency resonance:(double)resonance {
    double normalizedFrequency = fmax(0.0, fmin(frequency, 1.0));
    double Q = fmax(0.0, resonance);
    
    if (normalizedFrequency <= 0.0 || normalizedFrequency >= 1.0) {
        [self setNormalizedCoefficientsA0:1.0 a1:0.0 a2:0.0 b0:0.0 b1:0.0 b2:0.0];
        return;
    }
    
    if (Q <= 0.0) {
        [self setNormalizedCoefficientsA0:1.0 a1:0.0 a2:0.0 b0:1.0 b1:0.0 b2:0.0];
        return;
    }
    
    double w0 = 2.0 * M_PI * normalizedFrequency;
    double alpha = sin(w0) / (2.0 * Q);
    
    double b0 = alpha * Q;
    double b1 = 0.0;
    double b2 = -alpha * Q;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cos(w0);
    double a2 = 1.0 - alpha;
    
    [self setNormalizedCoefficientsA0:a0 a1:a1 a2:a2 b0:b0 b1:b1 b2:b2];
}

- (void)setLowshelfCoefficientsWithFrequency:(double)frequency gain:(double)gain {
    double normalizedFrequency = fmax(0.0, fmin(frequency, 1.0));
    double A = pow(10.0, gain / 40.0);

    if (normalizedFrequency == 1.0) {
        [self setNormalizedCoefficientsA0:1.0 a1:0.0 a2:0.0 b0:A * A b1:0.0 b2:0.0];
        return;
    }

    if (normalizedFrequency <= 0.0) {
        [self setNormalizedCoefficientsA0:1.0 a1:0.0 a2:0.0 b0:1.0 b1:0.0 b2:0.0];
        return;
    }

    double w0 = 2.0 * M_PI * normalizedFrequency;
    double S = 1.0;
    double alpha = sin(w0) / 2.0 * sqrt((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0);

    double b0 = A * ((A + 1.0) - (A - 1.0) * cos(w0) + 2.0 * sqrt(A) * alpha);
    double b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cos(w0));
    double b2 = A * ((A + 1.0) - (A - 1.0) * cos(w0) - 2.0 * sqrt(A) * alpha);
    double a0 = (A + 1.0) + (A - 1.0) * cos(w0) + 2.0 * sqrt(A) * alpha;
    double a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cos(w0));
    double a2 = (A + 1.0) + (A - 1.0) * cos(w0) - 2.0 * sqrt(A) * alpha;

    [self setNormalizedCoefficientsA0:a0 a1:a1 a2:a2 b0:b0 b1:b1 b2:b2];
}

- (void)setHighshelfCoefficientsWithFrequency:(double)frequency gain:(double)gain {
    double normalizedFrequency = fmax(0.0, fmin(frequency, 1.0));
    double A = pow(10.0, gain / 40.0);

    if (normalizedFrequency == 1.0) {
        [self setNormalizedCoefficientsA0:1.0 a1:0.0 a2:0.0 b0:1.0 b1:0.0 b2:0.0];
        return;
    }

    if (normalizedFrequency <= 0.0) {
        [self setNormalizedCoefficientsA0:1.0 a1:0.0 a2:0.0 b0:A * A b1:0.0 b2:0.0];
        return;
    }

    double w0 = M_PI * normalizedFrequency;
    double S = 1.0;
    double alpha = sin(w0) / 2.0 * sqrt((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0);

    double b0 = A * ((A + 1.0) - (A - 1.0) * cos(w0) + 2.0 * sqrt(A) * alpha);
    double b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cos(w0));
    double b2 = A * ((A + 1.0) + (A - 1.0) * cos(w0) - 2.0 * sqrt(A) * alpha);
    double a0 = (A + 1.0) - (A - 1.0) * cos(w0) + 2.0 * sqrt(A) * alpha;
    double a1 = 2.0 * ((A - 1.0) + (A + 1.0) * cos(w0));
    double a2 = (A + 1.0) - (A - 1.0) * cos(w0) - 2.0 * sqrt(A) * alpha;

    [self setNormalizedCoefficientsA0:a0 a1:a1 a2:a2 b0:b0 b1:b1 b2:b2];
}

- (void)setPeakingCoefficientsWithFrequency:(double)frequency resonance:(double)resonance gain:(double)gain {
    double normalizedFrequency = fmax(0.0, fmin(frequency, 1.0));
    double Q = fmax(0.0, resonance);
    double A = pow(10.0, gain / 40.0);

    if (normalizedFrequency <= 0.0 || normalizedFrequency >= 1.0) {
        [self setNormalizedCoefficientsA0:1.0 a1:0.0 a2:0.0 b0:1.0 b1:0.0 b2:0.0];
        return;
    }

    if (Q <= 0.0) {
        [self setNormalizedCoefficientsA0:1.0 a1:0.0 a2:0.0 b0:1.0 b1:0.0 b2:0.0];
        return;
    }

    double w0 = 2.0 * M_PI * normalizedFrequency;
    double alpha = sin(w0) / (2.0 * Q);

    double b0 = 1.0 + alpha * A;
    double b1 = -2.0 * cos(w0);
    double b2 = 1.0 - alpha * A;
    double a0 = 1.0 + alpha / A;
    double a1 = -2.0 * cos(w0);
    double a2 = 1.0 - alpha / A;

    [self setNormalizedCoefficientsA0:a0 a1:a1 a2:a2 b0:b0 b1:b1 b2:b2];
}

- (void)setNotchCoefficientsWithFrequency:(double)frequency resonance:(double)resonance {
    double normalizedFrequency = fmax(0.0, fmin(frequency, 1.0));
    double Q = fmax(0.0, resonance);

    if (normalizedFrequency <= 0.0 || normalizedFrequency >= 1.0) {
        [self setNormalizedCoefficientsA0:1.0 a1:0.0 a2:0.0 b0:1.0 b1:0.0 b2:0.0];
        return;
    }

    if (Q <= 0.0) {
        [self setNormalizedCoefficientsA0:1.0 a1:0.0 a2:0.0 b0:1.0 b1:0.0 b2:0.0];
        return;
    }

    double w0 = 2.0 * M_PI * normalizedFrequency;
    double alpha = sin(w0) / (2.0 * Q);

    double b0 = 1.0;
    double b1 = -2.0 * cos(w0);
    double b2 = 1.0;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cos(w0);
    double a2 = 1.0 - alpha;

    [self setNormalizedCoefficientsA0:a0 a1:a1 a2:a2 b0:b0 b1:b1 b2:b2];
}

- (void)setAllpassCoefficientsWithFrequency:(double)frequency resonance:(double)resonance {
    double normalizedFrequency = fmax(0.0, fmin(frequency, 1.0));
    double Q = fmax(0.0, resonance);

    if (normalizedFrequency <= 0.0 || normalizedFrequency >= 1.0) {
        [self setNormalizedCoefficientsA0:1.0 a1:0.0 a2:0.0 b0:1.0 b1:0.0 b2:0.0];
        return;
    }

    if (Q <= 0.0) {
        [self setNormalizedCoefficientsA0:1.0 a1:0.0 a2:0.0 b0:1.0 b1:0.0 b2:0.0];
        return;
    }

    double w0 = 2.0 * M_PI * normalizedFrequency;
    double alpha = sin(w0) / (2.0 * Q);

    double b0 = 1.0 - alpha;
    double b1 = -2.0 * cos(w0);
    double b2 = 1.0 + alpha;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cos(w0);
    double a2 = 1.0 - alpha;

    [self setNormalizedCoefficientsA0:a0 a1:a1 a2:a2 b0:b0 b1:b1 b2:b2];
}

- (void)applyFilter {
    double currentTime = [self.context getCurrentTime];
    double normalizedFrequency = [self.frequencyParam getValueAtTime:currentTime] / [Constants nyquistFrequency];
    double detuneValue = [self.detuneParam getValueAtTime:currentTime];
    
    if (detuneValue != 0.0) {
        normalizedFrequency *= pow(2.0, detuneValue / 1200.0);
    }

    switch (self.filterType) {
        case FilterTypeLowpass:
            [self setLowpassCoefficientsWithFrequency:normalizedFrequency resonance:[self.QParam getValueAtTime:currentTime]];
            break;
        case FilterTypeHighpass:
            [self setHighpassCoefficientsWithFrequency:normalizedFrequency resonance:[self.QParam getValueAtTime:currentTime]];
            break;
        case FilterTypeBandpass:
            [self setBandpassCoefficientsWithFrequency:normalizedFrequency resonance:[self.QParam getValueAtTime:currentTime]];
            break;
        case FilterTypeLowshelf:
            [self setLowshelfCoefficientsWithFrequency:normalizedFrequency gain:[self.gainParam getValueAtTime:currentTime]];
            break;
        case FilterTypeHighshelf:
            [self setHighshelfCoefficientsWithFrequency:normalizedFrequency gain:[self.gainParam getValueAtTime:currentTime]];
            break;
        case FilterTypePeaking:
            [self setPeakingCoefficientsWithFrequency:normalizedFrequency resonance:[self.QParam getValueAtTime:currentTime] gain:[self.gainParam getValueAtTime:currentTime]];
            break;
        case FilterTypeNotch:
            [self setNotchCoefficientsWithFrequency:normalizedFrequency resonance:[self.QParam getValueAtTime:currentTime]];
            break;
        case FilterTypeAllpass:
            [self setAllpassCoefficientsWithFrequency:normalizedFrequency resonance:[self.QParam getValueAtTime:currentTime]];
            break;
        default:
            break;
    }
}

- (void)processWithParameters:(PlaybackParameters *)parameters {
    [self resetCoefficients];
    [self applyFilter];
    
    for (int frame = 0; frame < parameters.frameCount; frame++) {
        double input = parameters.leftBuffer[frame];
        double output = _b0 * input + _b1 * _x1 + _b2 * _x2 - _a1 * _y1 - _a2 * _y2;
        
        _x2 = _x1;
        _x1 = input;
        _y2 = _y1;
        _y1 = output;
        
        parameters.leftBuffer[frame] = output;
        parameters.rightBuffer[frame] = output;
    }

    [super processWithParameters:parameters];
}

@end

