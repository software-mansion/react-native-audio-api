import React from 'react';

type ChannelCountMode = 'max' | 'clamped-max' | 'explicit';
type ChannelInterpretation = 'speakers' | 'discrete';

interface AudioNodePropsTableProps {
  numberOfInputs: number;
  numberOfOutputs: number;
  channelCount: number;
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
    { label: "Number of inputs", value: numberOfInputs },
    { label: "Number of outputs", value: numberOfOutputs },
    { label: "Channel count", value: channelCount },
    { label: "Channel count mode", value: channelCountMode },
    { label: "Channel interpretation", value: channelInterpretation },
  ];

  return (
    <table
      className="audio-node-props"
      style={{
        borderCollapse: "collapse",
        width: "100%",
      }}
    >
      <tbody>
        {props.map((prop, index) => (
          <tr key={index}>
            <td
              style={{
                textAlign: "center",
              }}
            >
              {prop.label}
            </td>
            <td
              style={{
                textAlign: "center"
              }}
            >
              {prop.value}
            </td>
          </tr>
        ))}
      </tbody>
    </table>
  );
};

export default AudioNodePropsTable;
