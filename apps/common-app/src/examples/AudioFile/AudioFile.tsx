import React, { useCallback, useEffect, useState, FC } from 'react';
import {
  ActivityIndicator,
  View,
  StyleSheet,
  Text,
  Pressable,
} from 'react-native';
import { Container, Button, Spacer, Slider } from '../../components'; // <-- slider z components
import AudioPlayer from './AudioPlayer';
import { colors, layout } from '../../styles';

const URL =
  'https://software-mansion.github.io/react-native-audio-api/audio/voice/example-voice-01.mp3';

const FILTER_TYPES: BiquadFilterType[] = [
  'lowpass',
  'highpass',
  'bandpass',
  'lowshelf',
  'highshelf',
  'peaking',
  'notch',
  'allpass',
];

const AudioFile: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [positionPercentage, setPositionPercentage] = useState(0);

  const [filterType, setFilterType] = useState<BiquadFilterType>('lowpass');
  const [filterFreq, setFilterFreq] = useState(1000);

  const togglePlayPause = async () => {
    if (isPlaying) {
      await AudioPlayer.pause();
    } else {
      AudioPlayer.setOnPositionChanged((offset) => {
        setPositionPercentage(offset);
      });

      await AudioPlayer.play();
    }

    setIsPlaying((prev) => !prev);
  };

  const fetchAudioBuffer = useCallback(async () => {
    setIsLoading(true);
    await AudioPlayer.loadBuffer(URL);
    setIsLoading(false);
  }, []);

  useEffect(() => {
    fetchAudioBuffer();

    return () => {
      AudioPlayer.reset();
    };
  }, [fetchAudioBuffer]);

  // 🔹 reaguj na zmianę UI
  useEffect(() => {
    AudioPlayer.setFilterType(filterType);
  }, [filterType]);

  useEffect(() => {
    AudioPlayer.setFilterFrequency(filterFreq);
  }, [filterFreq]);

  return (
    <Container centered>
      {isLoading && <ActivityIndicator color="#FFFFFF" />}
      <Button
        title={isPlaying ? 'Stop' : 'Play'}
        onPress={togglePlayPause}
        disabled={isLoading}
      />
      <Spacer.Vertical size={20} />

      <Slider
        label="Filter Freq"
        value={filterFreq}
        onValueChange={setFilterFreq}
        min={50}
        max={5000}
        step={10}
        minLabelWidth={80}
      />
      <Spacer.Vertical size={20} />

      <View style={styles.filterTypeContainer}>
        {FILTER_TYPES.map((type) => (
          <Pressable
            key={type}
            style={({ pressed }) => [
              styles.filterButton,
              pressed
                ? styles.pressedFilterButton
                : type === filterType
                  ? styles.activeFilterButton
                  : styles.inactiveFilterButton,
            ]}
            onPress={() => setFilterType(type)}>
            <Text
              style={[
                styles.filterButtonText,
                type === filterType && styles.activeFilterButtonText,
              ]}>
              {type}
            </Text>
          </Pressable>
        ))}
      </View>

      <Spacer.Vertical size={40} />

      <View style={styles.progressContainer}>
        <View
          style={[
            styles.progressBar,
            { width: `${positionPercentage * 100}%` },
          ]}
        />
      </View>
    </Container>
  );
};

export default AudioFile;

const styles = StyleSheet.create({
  filterTypeContainer: {
    flexDirection: 'row',
    flexWrap: 'wrap',
  },
  filterButton: {
    padding: layout.spacing,
    marginHorizontal: 5,
    marginVertical: 5,
    borderWidth: 1,
    borderRadius: layout.radius,
  },
  activeFilterButton: {
    backgroundColor: colors.main,
    borderColor: colors.main,
  },
  pressedFilterButton: {
    backgroundColor: `${colors.main}88`,
    borderColor: colors.main,
  },
  inactiveFilterButton: {
    borderColor: colors.border,
  },
  filterButtonText: {
    color: colors.white,
    textTransform: 'capitalize',
  },
  activeFilterButtonText: {
    color: colors.white,
  },
  progressContainer: {
    width: '100%',
    height: 10,
    backgroundColor: '#333',
    borderRadius: 5,
    overflow: 'hidden',
    marginTop: 20,
  },
  progressBar: {
    height: '100%',
    backgroundColor: colors.main,
    borderRadius: 5,
  },
});
