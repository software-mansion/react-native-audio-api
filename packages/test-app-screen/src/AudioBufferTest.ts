import { AudioContext, AudioBuffer } from "react-native-audio-api";
import { PCM_DATA } from "./constants";

const SUPPORTED_FORMATS = ['mp3', 'wav', 'aac', 'flac', 'ogg', 'opus', 'm4a', 'mp4'];
const EXPECTED_BUFFER_DURATION = 16;
const EXPECTED_CHANNELS = 2;

const CHANNELS_MAP: Map<number, string> = new Map([
  [1, 'https://dl.espressif.com/dl/audio/gs-16b-1c-44100hz.mp3'],
  [2, 'https://dl.espressif.com/dl/audio/gs-16b-2c-44100hz.mp3'],
  [4, 'https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/Samples/SoundCardAttrition/drmapan.wav'],
  [6, 'https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/Samples/Microsoft/6_Channel_ID.wav']
]);

const DURATIONS_MAP: Map<number, number> = new Map([
  [1, 16],
  [2, 16],
  [4, 4.78],
  [6, 5.84]
]);

export const audioBufferFormatsTest = async (audioContextRef: React.RefObject<AudioContext | null>, setTestingInfo: (value: React.SetStateAction<string>) => void) => {
  let buffers: AudioBuffer[] = [];
  for (const format of SUPPORTED_FORMATS) {
    const url = 'https://dl.espressif.com/dl/audio/gs-16b-2c-44100hz.' + format;
    setTestingInfo(`Loading audio buffer: ${format}`);
    await fetch(url, {
      headers: {
        'User-Agent': 'Mozilla/5.0 (Android; Mobile; rv:122.0) Gecko/122.0 Firefox/122.0',
      }
    })
      .then(response => response.arrayBuffer())
      .then(async (arrayBuffer) => {
        try {
          const audioBuffer = await audioContextRef.current!.decodeAudioData(arrayBuffer);
          console.log(`Decoded ${format} buffer:`, audioBuffer);
          if (Math.abs(audioBuffer.duration - EXPECTED_BUFFER_DURATION) > 0.3) {
            throw new Error(`Unexpected buffer duration: ${audioBuffer.duration}`);
          }
          if (audioBuffer.numberOfChannels !== EXPECTED_CHANNELS) {
            throw new Error(`Unexpected number of channels: ${audioBuffer.numberOfChannels}`);
          }
          buffers.push(audioBuffer);
        } catch (error) {
          setTestingInfo(`Error decoding audio buffer: ${format} - ${error}`);
        }
      })
  }
  for (let i = 0; i < buffers.length; i++) {
    setTestingInfo(`Playing ${SUPPORTED_FORMATS[i]} buffer`);
    const bufferSource = audioContextRef.current!.createBufferSource();
    bufferSource.buffer = buffers[i];
    bufferSource.connect(audioContextRef.current!.destination);
    bufferSource.start();
    await new Promise(resolve => setTimeout(resolve, 4000));
    if (i === buffers.length - 1) {
      bufferSource.onEnded = () => {
        setTestingInfo('Audio buffer test completed.');
      };
    }
    bufferSource.stop();
  }
}

export const audioBufferChannelsTest = async (audioContextRef: React.RefObject<AudioContext | null>, setTestingInfo: (value: React.SetStateAction<string>) => void) => {
  for (const channelsStr in CHANNELS_MAP) {
    const channels = parseInt(channelsStr, 10);
    const url = CHANNELS_MAP.get(channels)!;
    const expectedDuration = DURATIONS_MAP.get(channels)!;
    setTestingInfo(`Loading audio buffer with ${channels} channels`);
    await fetch(url, {
      headers: {
        'User-Agent': 'Mozilla/5.0 (Android; Mobile; rv:122.0) Gecko/122.0 Firefox/122.0',
      }
    })
      .then(response => response.arrayBuffer())
      .then(async (arrayBuffer) => {
        try {
          const audioBuffer = await audioContextRef.current!.decodeAudioData(arrayBuffer);
          console.log(`Decoded buffer with ${channels} channels:`, audioBuffer);
          if (Math.abs(audioBuffer.duration - expectedDuration) > 0.3) {
            throw new Error(`Unexpected buffer duration: ${audioBuffer.duration}`);
          }
          if (audioBuffer.numberOfChannels !== channels) {
            throw new Error(`Unexpected number of channels: ${audioBuffer.numberOfChannels}`);
          } else {
            setTestingInfo(`Playing buffer with ${channels} channels`);
            const bufferSource = audioContextRef.current!.createBufferSource();
            bufferSource.buffer = audioBuffer;
            bufferSource.connect(audioContextRef.current!.destination);
            bufferSource.start();
            await new Promise(resolve => setTimeout(resolve, 4000));
            if (channels === Object.keys(CHANNELS_MAP).length) {
              bufferSource.onEnded = () => {
                setTestingInfo('Audio buffer channels test completed.');
              };
            }
            bufferSource.stop();
          }
        } catch (error) {
          setTestingInfo(`Error decoding audio buffer with ${channels} channels - ${error}`);
        }
      })
  }
}

export const audioBufferBase64Test = async (audioContextRef: React.RefObject<AudioContext | null>, setTestingInfo: (value: React.SetStateAction<string>) => void) => {
  const audioBuffer = await audioContextRef.current!.decodePCMInBase64(PCM_DATA, 48000, 1, true);
  const bufferSource = audioContextRef.current!.createBufferSource();
  bufferSource.buffer = audioBuffer;
  bufferSource.connect(audioContextRef.current!.destination);
  bufferSource.start();
  bufferSource.stop(audioContextRef.current!.currentTime + 5);
  setTestingInfo('Playing audio buffer decoded from Base64 PCM data');
  await new Promise(resolve => setTimeout(resolve, 5000));
  setTestingInfo('Audio buffer Base64 test completed.');
}
