#import <OscillatorNode.h>

@implementation OscillatorNode

- (instancetype)init:(AudioContext *)context {
    if (self != [super init:context]) {
      return self;
    }
    
    _frequency = 440;
    _detune = 0;
    _sampleRate = 44100;
    _waveType = WaveTypeSine;
    self.numberOfOutputs = 1;

    _playerNode = [[AVAudioPlayerNode alloc] init];
    _playerNode.volume = 0.5;

    _format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:self.sampleRate channels:1];
    AVAudioFrameCount bufferFrameCapacity = (AVAudioFrameCount)self.sampleRate;
    _buffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:self.format frameCapacity:bufferFrameCapacity];
    _buffer.frameLength = bufferFrameCapacity;

    [self.context.audioEngine attachNode:self.playerNode];
    [self.context.audioEngine connect:self.playerNode to:self.context.audioEngine.mainMixerNode format: self.format];
    [self setBuffer];

    if (!self.context.audioEngine.isRunning) {
        NSError *error = nil;
        if (![self.context.audioEngine startAndReturnError:&error]) {
            NSLog(@"Error starting audio engine: %@", [error localizedDescription]);
        }
    }

    [self.context connectOscillator:self];

    return self;
}

- (void)start {
    [self process:_buffer playerNode:_playerNode];
    [self.playerNode scheduleBuffer:_buffer atTime:nil options:AVAudioPlayerNodeBufferLoops completionHandler:nil];
    [self.playerNode play];
}

- (void)stop {
    [self.playerNode stop];
}

- (void)setBuffer {
    AVAudioFrameCount bufferFrameCapacity = (AVAudioFrameCount)_sampleRate;
    
    // Convert cents to HZ
    double detuneRatio = pow(2.0, _detune / OCTAVE_IN_CENTS);
    double detunedFrequency = detuneRatio * _frequency;
    double phaseIncrement = FULL_CIRCLE_RADIANS * detunedFrequency / _sampleRate;
    double phase = 0.0;
    float *audioBufferPointer = _buffer.floatChannelData[0];

    for (int frame = 0; frame < bufferFrameCapacity; frame++) {
        audioBufferPointer[frame] = [WaveType valueForWaveType:_waveType atPhase:phase];

        phase += phaseIncrement;
        if (phase > FULL_CIRCLE_RADIANS) {
            phase -= FULL_CIRCLE_RADIANS;
        }
    }
}

- (void)setFrequency:(float)frequency {
    _frequency = frequency;
    [self setBuffer];
}

- (float)getFrequency {
    return _frequency;
}

- (void)setDetune:(float)detune {
    _detune = detune;
    [self setBuffer];
}

- (float)getDetune {
    return _detune;
}

- (void)setType:(NSString *)type {
    _waveType = [WaveType waveTypeFromString:type];
    [self setBuffer]; // Update the buffer with the new wave type
}

- (NSString *)getType {
    return [WaveType stringFromWaveType:self.waveType];
}

@end
