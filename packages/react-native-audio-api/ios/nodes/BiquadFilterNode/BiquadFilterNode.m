#import "BiquadFilterNode.h"
#import "AudioContext.h"
#import "Constants.h"

// https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html - math formulas for filters
// https://github.com/LabSound/LabSound/blob/main/src/internal/src/Biquad.cpp - implementation of filters on which I
// based mine

@implementation BiquadFilterNode

- (instancetype)initWithContext:(AudioContext *)context
{
  if (self = [super initWithContext:context]) {
    _frequencyParam = [[AudioParam alloc] initWithContext:context
                                                    value:350
                                                 minValue:[Constants minFilterFrequency]
                                                 maxValue:[Constants maxFilterFrequency]];
    _detuneParam = [[AudioParam alloc] initWithContext:context
                                                 value:0
                                              minValue:-[Constants maxDetune]
                                              maxValue:[Constants maxDetune]];
    _QParam = [[AudioParam alloc] initWithContext:context
                                            value:1
                                         minValue:-[Constants maxFilterQ]
                                         maxValue:[Constants maxFilterQ]];
    _gainParam = [[AudioParam alloc] initWithContext:context
                                               value:0
                                            minValue:[Constants minFilterGain]
                                            maxValue:[Constants maxGain]];
    self.numberOfInputs = 1;
    self.numberOfOutputs = 1;
    _filterType = FilterTypeLowpass;
  }

  [self setNormalizedCoefficientsB0:1.0 b1:0.0 b2:0.0 a0:1.0 a1:0.0 a2:0.0];
  [self resetCoefficients];

  return self;
}

- (void)resetCoefficients
{
  _x1 = 0;
  _x2 = 0;
  _y1 = 0;
  _y2 = 0;
}

- (void)setNormalizedCoefficientsB0:(double)b0 b1:(double)b1 b2:(double)b2 a0:(double)a0 a1:(double)a1 a2:(double)a2
{
  double a0Inverse = 1 / a0;

  self.b0 = b0 * a0Inverse;
  self.b1 = b1 * a0Inverse;
  self.b2 = b2 * a0Inverse;
  self.a1 = a1 * a0Inverse;
  self.a2 = a2 * a0Inverse;
}

- (void)cleanup
{

  _frequencyParam = nil;
  _detuneParam = nil;
  _QParam = nil;
  _gainParam = nil;
}

- (void)setType:(NSString *)type
{
  _filterType = [FilterType filterTypeFromString:type];
}

- (NSString *)getType
{
  return [FilterType stringFromFilterType:self.filterType];
}

- (void)setLowpassCoefficientsWithFrequency:(double)frequency Q:(double)Q
{
  frequency = fmax(0.0, fmin(frequency, 1.0));

  if (frequency == 1.0) {
    [self setNormalizedCoefficientsB0:1.0 b1:0.0 b2:0.0 a0:1.0 a1:0.0 a2:0.0];
    return;
  }

  if (frequency <= 0.0) {
    [self setNormalizedCoefficientsB0:0.0 b1:0.0 b2:0.0 a0:1.0 a1:0.0 a2:0.0];
    return;
  }

  Q = fmax(0.0, Q);
  double g = pow(10.0, 0.05 * Q);
  double d = sqrt((4 - sqrt(16 - 16 / (g * g))) / 2);

  double theta = M_PI * frequency;
  double sn = 0.5 * d * sin(theta);
  double beta = 0.5 * (1 - sn) / (1 + sn);
  double gamma = (0.5 + beta) * cos(theta);
  double alpha = 0.25 * (0.5 + beta - gamma);

  double b0 = 2 * alpha;
  double b1 = 2 * 2 * alpha;
  double b2 = 2 * alpha;
  double a1 = 2 * -gamma;
  double a2 = 2 * beta;

  [self setNormalizedCoefficientsB0:b0 b1:b1 b2:b2 a0:1.0 a1:a1 a2:a2];
}

- (void)setHighpassCoefficientsWithFrequency:(double)frequency Q:(double)Q
{
  frequency = fmax(0.0, fmin(frequency, 1.0));

  if (frequency == 1.0) {
    [self setNormalizedCoefficientsB0:0.0 b1:0.0 b2:0.0 a0:1.0 a1:0.0 a2:0.0];
    return;
  }

  if (frequency <= 0.0) {
    [self setNormalizedCoefficientsB0:1.0 b1:0.0 b2:0.0 a0:1.0 a1:0.0 a2:0.0];
    return;
  }

  Q = fmax(0.0, Q);
  double g = pow(10.0, 0.05 * Q);
  double d = sqrt((4 - sqrt(16 - 16 / (g * g))) / 2);

  double theta = M_PI * frequency;
  double sn = 0.5 * d * sin(theta);
  double beta = 0.5 * (1 - sn) / (1 + sn);
  double gamma = (0.5 + beta) * cos(theta);
  double alpha = 0.25 * (0.5 + beta - gamma);

  double b0 = 2 * alpha;
  double b1 = 2 * -2 * alpha;
  double b2 = 2 * alpha;
  double a1 = 2 * -gamma;
  double a2 = 2 * beta;

  [self setNormalizedCoefficientsB0:b0 b1:b1 b2:b2 a0:1.0 a1:a1 a2:a2];
}

- (void)setBandpassCoefficientsWithFrequency:(double)frequency Q:(double)Q
{
  frequency = fmax(0.0, fmin(frequency, 1.0));
  Q = fmax(0.0, Q);

  if (frequency <= 0.0 || frequency >= 1.0) {
    [self setNormalizedCoefficientsB0:0.0 b1:0.0 b2:0.0 a0:1.0 a1:0.0 a2:0.0];
    return;
  }

  if (Q <= 0.0) {
    [self setNormalizedCoefficientsB0:1.0 b1:0.0 b2:0.0 a0:1.0 a1:0.0 a2:0.0];
    return;
  }

  double w0 = M_PI * frequency;
  double alpha = sin(w0) / (2 * Q);
  double k = cos(w0);

  double b0 = alpha;
  double b1 = 0;
  double b2 = -alpha;
  double a0 = 1 + alpha;
  double a1 = -2 * k;
  double a2 = 1 - alpha;

  [self setNormalizedCoefficientsB0:b0 b1:b1 b2:b2 a0:a0 a1:a1 a2:a2];
}

- (void)setLowshelfCoefficientsWithFrequency:(double)frequency gain:(double)gain
{
  frequency = fmax(0.0, fmin(frequency, 1.0));
  double A = pow(10.0, gain / 40.0);

  if (frequency == 1.0) {
    [self setNormalizedCoefficientsB0:A * A b1:0.0 b2:0.0 a0:1.0 a1:0.0 a2:0.0];
    return;
  }

  if (frequency <= 0.0) {
    [self setNormalizedCoefficientsB0:1.0 b1:0.0 b2:0.0 a0:1.0 a1:0.0 a2:0.0];
    return;
  }

  double w0 = M_PI * frequency;
  double S = 1;
  double alpha = 0.5 * sin(w0) * sqrt((A + 1 / A) * (1 / S - 1) + 2);
  double k = cos(w0);
  double k2 = 2 * sqrt(A) * alpha;
  double aPlusOne = A + 1;
  double aMinusOne = A - 1;

  double b0 = A * (aPlusOne - aMinusOne * k + k2);
  double b1 = 2 * A * (aMinusOne - aPlusOne * k);
  double b2 = A * (aPlusOne - aMinusOne * k - k2);
  double a0 = aPlusOne + aMinusOne * k + k2;
  double a1 = -2 * (aMinusOne + aPlusOne * k);
  double a2 = aPlusOne + aMinusOne * k - k2;

  [self setNormalizedCoefficientsB0:b0 b1:b1 b2:b2 a0:a0 a1:a1 a2:a2];
}

- (void)setHighshelfCoefficientsWithFrequency:(double)frequency gain:(double)gain
{
  frequency = fmax(0.0, fmin(frequency, 1.0));
  double A = pow(10.0, gain / 40.0);

  if (frequency == 1.0) {
    [self setNormalizedCoefficientsB0:1.0 b1:0.0 b2:0.0 a0:1.0 a1:0.0 a2:0.0];
    return;
  }

  if (frequency <= 0.0) {
    [self setNormalizedCoefficientsB0:A * A b1:0.0 b2:0.0 a0:1.0 a1:0.0 a2:0.0];
    return;
  }

  double w0 = M_PI * frequency;
  double S = 1;
  double alpha = 0.5 * sin(w0) * sqrt((A + 1 / A) * (1 / S - 1) + 2);
  double k = cos(w0);
  double k2 = 2 * sqrt(A) * alpha;
  double aPlusOne = A + 1;
  double aMinusOne = A - 1;

  double b0 = A * (aPlusOne + aMinusOne * k + k2);
  double b1 = -2 * A * (aMinusOne + aPlusOne * k);
  double b2 = A * (aPlusOne + aMinusOne * k - k2);
  double a0 = aPlusOne - aMinusOne * k + k2;
  double a1 = 2 * (aMinusOne - aPlusOne * k);
  double a2 = aPlusOne - aMinusOne * k - k2;

  [self setNormalizedCoefficientsB0:b0 b1:b1 b2:b2 a0:a0 a1:a1 a2:a2];
}

- (void)setPeakingCoefficientsWithFrequency:(double)frequency Q:(double)Q gain:(double)gain
{
  frequency = fmax(0.0, fmin(frequency, 1.0));
  Q = fmax(0.0, Q);
  double A = pow(10.0, gain / 40.0);

  if (frequency <= 0.0 || frequency >= 1.0) {
    [self setNormalizedCoefficientsB0:1.0 b1:0.0 b2:0.0 a0:1.0 a1:0.0 a2:0.0];
    return;
  }

  if (Q <= 0.0) {
    [self setNormalizedCoefficientsB0:A * A b1:0.0 b2:0.0 a0:1.0 a1:0.0 a2:0.0];
    return;
  }

  double w0 = M_PI * frequency;
  double alpha = sin(w0) / (2 * Q);
  double k = cos(w0);

  double b0 = 1 + alpha * A;
  double b1 = -2 * k;
  double b2 = 1 - alpha * A;
  double a0 = 1 + alpha / A;
  double a1 = -2 * k;
  double a2 = 1 - alpha / A;

  [self setNormalizedCoefficientsB0:b0 b1:b1 b2:b2 a0:a0 a1:a1 a2:a2];
}

- (void)setNotchCoefficientsWithFrequency:(double)frequency Q:(double)Q
{
  frequency = fmax(0.0, fmin(frequency, 1.0));
  Q = fmax(0.0, Q);

  if (frequency <= 0.0 || frequency >= 1.0) {
    [self setNormalizedCoefficientsB0:1.0 b1:0.0 b2:0.0 a0:1.0 a1:0.0 a2:0.0];
    return;
  }

  if (Q <= 0.0) {
    [self setNormalizedCoefficientsB0:0.0 b1:0.0 b2:0.0 a0:1.0 a1:0.0 a2:0.0];
    return;
  }

  double w0 = M_PI * frequency;
  double alpha = sin(w0) / (2 * Q);
  double k = cos(w0);

  double b0 = 1;
  double b1 = -2 * k;
  double b2 = 1;
  double a0 = 1 + alpha;
  double a1 = -2 * k;
  double a2 = 1 - alpha;

  [self setNormalizedCoefficientsB0:b0 b1:b1 b2:b2 a0:a0 a1:a1 a2:a2];
}

- (void)setAllpassCoefficientsWithFrequency:(double)frequency Q:(double)Q
{
  frequency = fmax(0.0, fmin(frequency, 1.0));
  Q = fmax(0.0, Q);

  if (frequency <= 0.0 || frequency >= 1.0) {
    [self setNormalizedCoefficientsB0:1.0 b1:0.0 b2:0.0 a0:1.0 a1:0.0 a2:0.0];
    return;
  }

  if (Q <= 0.0) {
    [self setNormalizedCoefficientsB0:-1.0 b1:0.0 b2:0.0 a0:1.0 a1:0.0 a2:0.0];
    return;
  }

  double w0 = M_PI * frequency;
  double alpha = sin(w0) / (2 * Q);
  double k = cos(w0);

  double b0 = 1 - alpha;
  double b1 = -2 * k;
  double b2 = 1 + alpha;
  double a0 = 1 + alpha;
  double a1 = -2 * k;
  double a2 = 1 - alpha;

  [self setNormalizedCoefficientsB0:b0 b1:b1 b2:b2 a0:a0 a1:a1 a2:a2];
}

- (void)applyFilter
{
  double currentTime = [self.context getCurrentTime];
  double normalizedFrequency = [self.frequencyParam getValueAtTime:currentTime] / [Constants nyquistFrequency];
  double detuneValue = [self.detuneParam getValueAtTime:currentTime];

  if (detuneValue != 0.0) {
    normalizedFrequency *= pow(2.0, detuneValue / 1200.0);
  }

  switch (self.filterType) {
    case FilterTypeLowpass:
      [self setLowpassCoefficientsWithFrequency:normalizedFrequency Q:[self.QParam getValueAtTime:currentTime]];
      break;
    case FilterTypeHighpass:
      [self setHighpassCoefficientsWithFrequency:normalizedFrequency Q:[self.QParam getValueAtTime:currentTime]];
      break;
    case FilterTypeBandpass:
      [self setBandpassCoefficientsWithFrequency:normalizedFrequency Q:[self.QParam getValueAtTime:currentTime]];
      break;
    case FilterTypeLowshelf:
      [self setLowshelfCoefficientsWithFrequency:normalizedFrequency gain:[self.gainParam getValueAtTime:currentTime]];
      break;
    case FilterTypeHighshelf:
      [self setHighshelfCoefficientsWithFrequency:normalizedFrequency gain:[self.gainParam getValueAtTime:currentTime]];
      break;
    case FilterTypePeaking:
      [self setPeakingCoefficientsWithFrequency:normalizedFrequency
                                              Q:[self.QParam getValueAtTime:currentTime]
                                           gain:[self.gainParam getValueAtTime:currentTime]];
      break;
    case FilterTypeNotch:
      [self setNotchCoefficientsWithFrequency:normalizedFrequency Q:[self.QParam getValueAtTime:currentTime]];
      break;
    case FilterTypeAllpass:
      [self setAllpassCoefficientsWithFrequency:normalizedFrequency Q:[self.QParam getValueAtTime:currentTime]];
      break;
    default:
      break;
  }
}

- (void)process:(AVAudioFrameCount)frameCount bufferList:(AudioBufferList *)bufferList
{
  [self resetCoefficients];
  [self applyFilter];

  float *bufferL = (float *)bufferList->mBuffers[0].mData;
  float *bufferR = (float *)bufferList->mBuffers[1].mData;

  double x1 = _x1;
  double x2 = _x2;
  double y1 = _y1;
  double y2 = _y2;

  double b0 = _b0;
  double b1 = _b1;
  double b2 = _b2;
  double a1 = _a1;
  double a2 = _a2;

  for (int frame = 0; frame < frameCount; frame++) {
    double input = bufferL[frame];
    double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

    bufferL[frame] = output;
    bufferR[frame] = output;

    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
  }

  self.x1 = x1;
  self.x2 = x2;
  self.y1 = y1;
  self.y2 = y2;

  self.b0 = b0;
  self.b1 = b1;
  self.b2 = b2;
  self.a1 = a1;

  [super process:frameCount bufferList:bufferList];
}

@end
