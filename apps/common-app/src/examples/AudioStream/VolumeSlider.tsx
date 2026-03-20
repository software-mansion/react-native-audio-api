import { useContext } from 'react';
import { AudioComponentContext } from 'react-native-audio-api/development/react';
import { Slider } from '../../components';
import { View } from 'react-native';

const VolumeSlider: React.FC = () => {
  const { volume, setVolume } = useContext(AudioComponentContext);

  return (
    <View style={{ width: 200 }}>
    <Slider
      value={volume}
      onValueChange={setVolume}
      min={0}
      max={1}
      step={0.01}
    />
    </View>
  );
};

export default VolumeSlider;
