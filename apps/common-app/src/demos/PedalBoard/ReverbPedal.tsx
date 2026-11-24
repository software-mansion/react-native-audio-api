import React, { useEffect, useState } from 'react';
import { StyleSheet, View } from 'react-native';

import { SegmentedControl } from '../../components';
import { colors } from '../../styles';
import { makeReverbCurve } from './curves';
import PedalShell, { PedalProps } from './PedalShell';
import { usePedalRouting } from './usePedalRouting';

const PEDAL_COLOR = colors.blue;
const MAKEUP_GAIN = 2;

/** Impulse response lengths, in seconds, for each selectable space. */
const SPACE_DURATIONS = { Room: 0.4, Hall: 0.7, Cathedral: 5 } as const;

type Space = keyof typeof SPACE_DURATIONS;

const SPACES = (Object.keys(SPACE_DURATIONS) as Space[]).map((space) => ({
  value: space,
  label: space,
}));

export default function ReverbPedal(props: PedalProps) {
  const { context } = props;
  const [space, setSpace] = useState<Space>('Hall');
  const [chain] = useState(() => [context.createConvolver()] as const);
  const [convolver] = chain;
  const power = usePedalRouting(props, chain, MAKEUP_GAIN);

  useEffect(() => {
    convolver.buffer = makeReverbCurve(SPACE_DURATIONS[space], context);
  }, [convolver, context, space]);

  return (
    <PedalShell name="REVERB" color={PEDAL_COLOR} {...power}>
      <View style={styles.spaces}>
        <SegmentedControl
          options={SPACES}
          value={space}
          onChange={setSpace}
          color={PEDAL_COLOR}
        />
      </View>
    </PedalShell>
  );
}

const styles = StyleSheet.create({
  spaces: {
    flex: 1,
  },
});
