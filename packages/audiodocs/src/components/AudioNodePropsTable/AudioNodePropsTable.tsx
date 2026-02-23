import React, { memo } from 'react';

import DetailBox from '@site/src/ui/DetailBox';

import styles from './styles.module.css';

type ChannelCountMode = 'max' | 'clamped-max' | 'explicit';
type ChannelInterpretation = 'speakers' | 'discrete';

interface AudioNodePropsTableProps {
  numberOfInputs: number;
  numberOfOutputs: number;
  channelCount: number | string;
  channelCountMode: ChannelCountMode;
  channelInterpretation: ChannelInterpretation;
}

const AudioNodePropsTable = ({
  numberOfInputs,
  numberOfOutputs,
  channelCount,
  channelCountMode,
  channelInterpretation,
}: AudioNodePropsTableProps) => {
  const props = [
    { label: 'Number of inputs', value: numberOfInputs },
    { label: 'Number of outputs', value: numberOfOutputs },
    { label: 'Channel count', value: channelCount },
    { label: 'Channel count mode', value: channelCountMode },
    { label: 'Channel interpretation', value: channelInterpretation },
  ];

  return (
    <DetailBox
      tag="AudioNode"
      info="properties"
      startOpen={false}
      className={styles.propsDetailBox}
    >
      <table className={styles.audioNodeProps} style={{ width: '100%', display: 'table' }}>
        <tbody>
          {props.map((prop) => (
            <tr key={prop.label}>
              <td
                style={{
                  textAlign: 'left',
                  whiteSpace: 'nowrap',
                }}
              >
                {prop.label}
              </td>
              <td
                style={{
                  textAlign: 'left',
                  width: '99%',
                }}
              >
                {prop.value}
              </td>
            </tr>
          ))}
        </tbody>
      </table>
    </DetailBox>
  );
};
export default memo(AudioNodePropsTable);
