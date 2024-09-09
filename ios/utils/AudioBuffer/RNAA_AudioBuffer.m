#import "RNAA_AudioBuffer.h"

@implementation RNAA_AudioBuffer

- (instancetype)initWithSampleRate:(int)sampleRate
                            length:(int)length
                  numberOfChannels:(int)numberOfChannels {
    self = [super init];
    if (self) {
        _sampleRate = sampleRate;
        _length = length;
        _numberOfChannels = numberOfChannels;
        _duration = (double)length / (double)sampleRate;

        // Allocate memory for channel data
        _channels = (float **)malloc(numberOfChannels * sizeof(float *));
        for (int i = 0; i < numberOfChannels; i++) {
            _channels[i] = (float *)malloc(length * sizeof(float));
        }
    }
    return self;
}

- (float *)getChannelDataForChannel:(int)channel {
    if (channel < 0 || channel >= self.numberOfChannels) {
        @throw [NSException exceptionWithName:NSInvalidArgumentException
                                       reason:@"Channel index out of bounds"
                                     userInfo:nil];
    }
    return self.channels[channel];
}

- (void)setChannelData:(int)channel data:(float *)data length:(int)length {
    if (channel < 0 || channel >= self.numberOfChannels) {
        @throw [NSException exceptionWithName:NSInvalidArgumentException
                                       reason:@"Channel index out of bounds"
                                     userInfo:nil];
    }
    if (length != self.length) {
        @throw [NSException exceptionWithName:NSInvalidArgumentException
                                       reason:@"Data length does not match buffer length"
                                     userInfo:nil];
    }
    memcpy(self.channels[channel], data, length * sizeof(float));
}

- (RNAA_AudioBuffer *)copyBuffer {
    RNAA_AudioBuffer *outputBuffer = [[RNAA_AudioBuffer alloc] initWithSampleRate:self.sampleRate
                                                                           length:self.length
                                                                 numberOfChannels:self.numberOfChannels];

    for (int i = 0; i < self.numberOfChannels; i++) {
        float *copiedData = (float *)malloc(self.length * sizeof(float));
        memcpy(copiedData, self.channels[i], self.length * sizeof(float));
        [outputBuffer setChannelData:i data:copiedData length:self.length];
        free(copiedData);
    }
    return outputBuffer;
}

- (RNAA_AudioBuffer *)mixWithOutputNumberOfChannels:(int)outputNumberOfChannels {
    if (self.numberOfChannels == outputNumberOfChannels) {
        return self;
    }

    switch (self.numberOfChannels) {
        case 1:
            if (outputNumberOfChannels == 2) {
                RNAA_AudioBuffer *outputBuffer = [[RNAA_AudioBuffer alloc] initWithSampleRate:self.sampleRate
                                                                                      length:self.length
                                                                            numberOfChannels:2];
                [outputBuffer setChannelData:0 data:self.channels[0] length:self.length];
                [outputBuffer setChannelData:1 data:self.channels[0] length:self.length];
                return outputBuffer;
            }
            break;
        case 2:
            if (outputNumberOfChannels == 1) {
                RNAA_AudioBuffer *outputBuffer = [[RNAA_AudioBuffer alloc] initWithSampleRate:self.sampleRate
                                                                                      length:self.length
                                                                            numberOfChannels:1];
                float *outputData = (float *)malloc(self.length * sizeof(float));

                for (int i = 0; i < self.length; i++) {
                    outputData[i] = (self.channels[0][i] + self.channels[1][i]) / 2;
                }

                [outputBuffer setChannelData:0 data:outputData length:self.length];
                free(outputData);
                return outputBuffer;
            }
            break;
    }

    @throw [NSException exceptionWithName:NSInvalidArgumentException
                                   reason:@"Unsupported number of channels"
                                 userInfo:nil];
}

- (void)cleanup {
    if (self.channels) {
        for (int i = 0; i < self.numberOfChannels; i++) {
            if (self.channels[i]) {
                free(self.channels[i]);
            }
        }
        free(self.channels);
        self.channels = NULL;
    }
}

- (void)dealloc {
    [self cleanup];
}

@end
