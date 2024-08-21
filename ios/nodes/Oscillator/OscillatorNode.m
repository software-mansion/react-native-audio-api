#import <OscillatorNode.h>

@implementation OscillatorNode

- (instancetype)initWithContext:(AudioContext *)context {
    if (self != [super initWithContext:context]) {
      return self;
    }
    
    _frequencyParam = [[AudioParam alloc] initWithContext:context value:440 minValue:0 maxValue:1600];
    _detuneParam = [[AudioParam alloc] initWithContext:context value:0 minValue:-100 maxValue:100];
    _waveType = WaveTypeSine;
    _isPlaying = NO;
    _deltaTime = 1 / self.context.sampleRate;
    
    self.numberOfOutputs = 1;
    self.numberOfInputs = 0;

    _format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:self.context.sampleRate channels:1];
    _phase = 0.0;

    __weak typeof(self) weakSelf = self;
    _sourceNode = [[AVAudioSourceNode alloc] initWithFormat:_format renderBlock:^OSStatus(BOOL *isSilence, const AudioTimeStamp *timestamp, AVAudioFrameCount frameCount, AudioBufferList *outputData) {
        return [weakSelf renderCallbackWithIsSilence:isSilence timestamp:timestamp frameCount:frameCount outputData:outputData];
    }];

    [self.context.audioEngine attachNode:self.sourceNode];
    [self.context.audioEngine connect:self.sourceNode to:self.context.audioEngine.mainMixerNode format:self.format];

    if (!self.context.audioEngine.isRunning) {
        NSError *error = nil;
        if (![self.context.audioEngine startAndReturnError:&error]) {
            NSLog(@"Error starting audio engine: %@", [error localizedDescription]);
        }
    }

    return self;
}

- (void)cleanup {
    if (_isPlaying) {
        [self stopPlayback];
    }
    
    [self.context.audioEngine detachNode:self.sourceNode];
    
    _frequencyParam = nil;
    _detuneParam = nil;
    _format = nil;
}

- (void)start:(double)time {
    if (_isPlaying) {
        return;
    }
    
    double delay = time - [self.context getCurrentTime];
    if (delay <= 0) {
        [self startPlayback];
    } else {
        [NSTimer scheduledTimerWithTimeInterval:delay target:self selector:@selector(startPlayback) userInfo:nil repeats:NO];
    }
}

- (void)startPlayback {
    _isPlaying = YES;
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
    _isPlaying = NO;
}

- (OSStatus)renderCallbackWithIsSilence:(BOOL *)isSilence timestamp:(const AudioTimeStamp *)timestamp frameCount:(AVAudioFrameCount)frameCount outputData:(AudioBufferList *)outputData {
    float *audioBufferPointer = (float *)outputData->mBuffers[0].mData;
    
    float time = [self.context getCurrentTime];
    for (int frame = 0; frame < frameCount; frame++) {
        // Convert cents to HZ
        if (!_isPlaying) {
            audioBufferPointer[frame] = 0;
            continue;
        }
       
        double detuneRatio = pow(2.0, [_detuneParam getValue] / OCTAVE_IN_CENTS);
        double detunedFrequency = round(detuneRatio * [_frequencyParam getValueAtTime:time]);
        double phaseIncrement = FULL_CIRCLE_RADIANS * detunedFrequency / self.context.sampleRate;
        audioBufferPointer[frame] = [WaveType valueForWaveType:_waveType atPhase:_phase];
                    
        time += _deltaTime;

        _phase += phaseIncrement;
        if (_phase > FULL_CIRCLE_RADIANS) {
            _phase -= FULL_CIRCLE_RADIANS;
        }
    }
    
    // Could change the process way to process single frame instead of looping over it again.
    [self process:audioBufferPointer frameCount:frameCount];
    
    return noErr;
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
