import React, { useState, FC } from 'react';
import { AudioBuffer, AudioContext } from 'react-native-audio-api';
import { ActivityIndicator, TextInput, StyleSheet } from 'react-native';

import { Container, Button, Spacer } from '../../components';
import Env from '../../utils/env';
import { colors } from '../../styles';

async function getOpenAIResponse(input: string, voice: string = 'alloy') {
  return await fetch('https://api.openai.com/v1/audio/speech', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Authorization': `Bearer ${Env.openAiToken}`,
    },
    body: JSON.stringify({
      model: 'tts-1-hd',
      voice: voice,
      input: input,
      response_format: 'pcm',
    }),
  }).then((response) => response.arrayBuffer());
}

function goofyResample(
  audioContext: AudioContext,
  input: Int16Array
): AudioBuffer {
  const outputBuffer = audioContext.createBuffer(2, input.length * 2, 48000);
  const processingChannel: Array<number> = [];
  const upSampleChannel: Array<number> = [];

  for (let i = 0; i < input.length; i += 1) {
    processingChannel[i] = input[i] / 32768.0;
  }

  for (let i = 0; i < input.length; i += 1) {
    const isLast = i === input.length - 1;
    const currentSample = processingChannel[i];
    const nextSample = isLast ? currentSample : processingChannel[i + 1];

    upSampleChannel[2 * i] = currentSample;
    upSampleChannel[2 * i + 1] = (currentSample + nextSample) / 2;
  }

  outputBuffer.copyToChannel(upSampleChannel, 0);
  outputBuffer.copyToChannel(upSampleChannel, 1);

  return outputBuffer;
}

const OpenAI: FC = () => {
  const [isLoading, setIsLoading] = useState(false);
  const [textToRead, setTextToRead] = useState('');

  const onTestOpenAI = async () => {
    if (isLoading) {
      return;
    }

    const aCtx = new AudioContext();

    setIsLoading(true);
    const results = await getOpenAIResponse(textToRead, 'alloy');
    setIsLoading(false);

    const audioBuffer = goofyResample(aCtx, new Int16Array(results));
    const sourceNode = aCtx.createBufferSource();
    const duration = audioBuffer.duration;
    const now = aCtx.currentTime;

    sourceNode.buffer = audioBuffer;

    sourceNode.connect(aCtx.destination);

    sourceNode.start(now);
    sourceNode.stop(now + duration);
  };

  return (
    <Container style={styles.container}>
      <Spacer.Vertical size={60} />
      <TextInput
        value={textToRead}
        onChangeText={setTextToRead}
        style={styles.textInput}
        multiline
      />
      <Spacer.Vertical size={24} />
      <Button onPress={onTestOpenAI} title="Test OpenAI" />
      <Spacer.Vertical size={24} />
      {isLoading && <ActivityIndicator />}
    </Container>
  );
};

export default OpenAI;

const styles = StyleSheet.create({
  container: {
    alignItems: 'center',
  },
  textInput: {
    backgroundColor: 'transparent',
    borderColor: colors.border,
    color: colors.white,
    borderWidth: 1,
    fontSize: 16,
    padding: 16,
    width: 280,
    height: 200,
    borderRadius: 6,
  },
});
