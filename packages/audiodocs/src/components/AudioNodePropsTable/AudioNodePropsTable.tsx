import React, { memo } from 'react';
import Link from '@docusaurus/Link';

import AudioNodeInheritedSection from '@site/src/components/AudioNodeInheritedSection';
import { ReadOnly } from '@site/src/components/Badges';

type ChannelCountMode = 'max' | 'clamped-max' | 'explicit';
type ChannelInterpretation = 'speakers' | 'discrete';

interface AudioNodePropsTableProps {
  numberOfInputs: number;
  numberOfOutputs: number;
  channelCount: number | string;
  channelCountMode: ChannelCountMode;
  channelInterpretation: ChannelInterpretation;
}

const renderValue = (value: number | string) =>
  typeof value === 'number' ? <code>{value}</code> : value;

const AudioNodePropsTable = ({
  numberOfInputs,
  numberOfOutputs,
  channelCount,
  channelCountMode,
  channelInterpretation,
}: AudioNodePropsTableProps) => {
  return (
    <AudioNodeInheritedSection info="properties">
      <thead>
        <tr>
          <th>Name</th>
          <th>Type</th>
          <th>Value</th>
          <th>Description</th>
          <th />
        </tr>
      </thead>
      <tbody>
        <tr>
          <td>
            <code>numberOfInputs</code>
          </td>
          <td>
            <code>number</code>
          </td>
          <td>{renderValue(numberOfInputs)}</td>
          <td>Number of input connections for the node.</td>
          <td>
            <ReadOnly />
          </td>
        </tr>
        <tr>
          <td>
            <code>numberOfOutputs</code>
          </td>
          <td>
            <code>number</code>
          </td>
          <td>{renderValue(numberOfOutputs)}</td>
          <td>Number of output connections for the node.</td>
          <td>
            <ReadOnly />
          </td>
        </tr>
        <tr>
          <td>
            <code>channelCount</code>
          </td>
          <td>
            <code>number</code>
          </td>
          <td>{renderValue(channelCount)}</td>
          <td>
            Number of channels used when up-mixing or down-mixing the node's
            inputs.
          </td>
          <td>
            <ReadOnly />
          </td>
        </tr>
        <tr>
          <td>
            <code>channelCountMode</code>
          </td>
          <td>
            <Link to="/docs/types/channel-count-mode">
              <code>ChannelCountMode</code>
            </Link>
          </td>
          <td>
            <code>{channelCountMode}</code>
          </td>
          <td>
            How channels are mixed between the node's inputs and outputs.
          </td>
          <td>
            <ReadOnly />
          </td>
        </tr>
        <tr>
          <td>
            <code>channelInterpretation</code>
          </td>
          <td>
            <Link to="/docs/types/channel-interpretation">
              <code>ChannelInterpretation</code>
            </Link>
          </td>
          <td>
            <code>{channelInterpretation}</code>
          </td>
          <td>
            How input channels are mapped to output channels when their counts
            differ.
          </td>
          <td>
            <ReadOnly />
          </td>
        </tr>
      </tbody>
    </AudioNodeInheritedSection>
  );
};

export default memo(AudioNodePropsTable);
