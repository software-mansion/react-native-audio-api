#import <OscillatorNode.h>

@implementation OscillatorNode {
    float deltaTime;
    float time;
    float lastTime;
}

- (instancetype)initWithContext:(AudioContext *)context {
    if (self != [super initWithContext:context]) {
      return self;
    }
    
    _frequencyParam = [[AudioParam alloc] initWithContext:context value:440 minValue:0 maxValue:1600];
    _detuneParam = [[AudioParam alloc] initWithContext:context value:0 minValue:-100 maxValue:100];
    _waveType = WaveTypeSine;
    _isPlaying = NO;
    deltaTime = 1 / self.context.sampleRate;
    
    self.numberOfOutputs = 1;
    self.numberOfInputs = 0;

    _playerNode = [[AVAudioPlayerNode alloc] init];
    _playerNode.volume = 0.5;

    _format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:self.context.sampleRate channels:1];
    AVAudioFrameCount bufferFrameCapacity = (AVAudioFrameCount)self.context.sampleRate;
    _buffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:self.format frameCapacity:bufferFrameCapacity];
    _buffer.frameLength = bufferFrameCapacity;

    [self.context.audioEngine attachNode:self.playerNode];
    [self.context.audioEngine connect:self.playerNode to:self.context.audioEngine.mainMixerNode format: self.format];

    if (!self.context.audioEngine.isRunning) {
        NSError *error = nil;
        if (![self.context.audioEngine startAndReturnError:&error]) {
            NSLog(@"Error starting audio engine: %@", [error localizedDescription]);
        }
    }

    [self.context connectOscillator:self];

    return self;
}

- (void)start:(double)time {
    if (_isPlaying) {
        return;
    }

    _isPlaying = YES;
    [self process:_buffer playerNode:_playerNode];
    [NSThread detachNewThreadSelector:@selector(setBuffer) toTarget:self withObject:nil];
    
    double delay = time - [self.context getCurrentTime];
    if (delay <= 0) {
        [self startPlayback];
    } else {
        [NSTimer scheduledTimerWithTimeInterval:delay target:self selector:@selector(startPlayback) userInfo:nil repeats:NO];
    }
}

- (void)startPlayback {
    [self.playerNode scheduleBuffer:_buffer atTime:nil options:AVAudioPlayerNodeBufferLoops completionHandler:nil];
    [self.playerNode play];
}

- (void)stop:(double)time {
    if (!_isPlaying) {
        return;
    }
    
    double delay = time - [self.context getCurrentTime];
    if (delay <= 0) {
        [self stopPlayback];
    } else {
        [NSTimer scheduledTimerWithTimeInterval:delay target:self selector:@selector(stopPlayback) userInfo:nil repeats:NO];
    }
}

- (void)stopPlayback {
    [self.playerNode stop];
    _isPlaying = NO;
}

- (void)process:(AVAudioPCMBuffer *)buffer playerNode:(AVAudioPlayerNode *)playerNode {
    [super process:buffer playerNode:playerNode];
}

- (void)setBuffer {
    time = lastTime = [self.context getCurrentTime];
    
    while (YES) {
        if (![self isPlaying]) {
            break;
        }
        
        AVAudioFrameCount bufferFrameCapacity = (AVAudioFrameCount)self.context.sampleRate;
        
        float currentTime = [self.context getCurrentTime];
        
        double phase = 0.0;
        float *audioBufferPointer = _buffer.floatChannelData[0];

        for (int frame = 0; frame < bufferFrameCapacity; frame++) {
            // Convert cents to HZ
            double detuneRatio = pow(2.0, [_detuneParam getValue] / OCTAVE_IN_CENTS);
            double detunedFrequency = round(detuneRatio * [_frequencyParam getValueAtTime:time]);
            double phaseIncrement = FULL_CIRCLE_RADIANS * detunedFrequency / self.context.sampleRate;
            audioBufferPointer[frame] = [WaveType valueForWaveType:_waveType atPhase:phase];
            [self process:_buffer playerNode:_playerNode];
            
            time += deltaTime;

            phase += phaseIncrement;
            if (phase > FULL_CIRCLE_RADIANS) {
                phase -= FULL_CIRCLE_RADIANS;
            }
        }
        
        if (currentTime > time) {
            lastTime = time;
        }
        
        time = lastTime;
    }
}

- (Boolean)getIsPlaying {
    return _isPlaying;
}

- (void)setType:(NSString *)type {
    _waveType = [WaveType waveTypeFromString:type];
}

- (NSString *)getType {
    return [WaveType stringFromWaveType:self.waveType];
}

@end
