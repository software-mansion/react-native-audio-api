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
          <th style={{ textAlign: 'center' }}>Name</th>
          <th style={{ textAlign: 'center' }}>Type</th>
          <th style={{ textAlign: 'center' }}>Value</th>
          <th style={{ textAlign: 'left' }}>Description</th>
          <th />
        </tr>
      </thead>
      <tbody>
        <tr>
          <td style={{ textAlign: 'center' }}>
            <code>numberOfInputs</code>
          </td>
          <td style={{ textAlign: 'center' }}>
            <code>number</code>
          </td>
          <td style={{ textAlign: 'center' }}>{renderValue(numberOfInputs)}</td>
          <td style={{ textAlign: 'left' }}>
            Number of input connections for the node.
          </td>
          <td>
            <ReadOnly />
          </td>
        </tr>
        <tr>
          <td style={{ textAlign: 'center' }}>
            <code>numberOfOutputs</code>
          </td>
          <td style={{ textAlign: 'center' }}>
            <code>number</code>
          </td>
          <td style={{ textAlign: 'center' }}>{renderValue(numberOfOutputs)}</td>
          <td style={{ textAlign: 'left' }}>
            Number of output connections for the node.
          </td>
          <td>
            <ReadOnly />
          </td>
        </tr>
        <tr>
          <td style={{ textAlign: 'center' }}>
            <code>channelCount</code>
          </td>
          <td style={{ textAlign: 'center' }}>
            <code>number</code>
          </td>
          <td style={{ textAlign: 'center' }}>{renderValue(channelCount)}</td>
          <td style={{ textAlign: 'left' }}>
            Number of channels used when up-mixing or down-mixing the node's
            inputs.
          </td>
          <td>
            <ReadOnly />
          </td>
        </tr>
        <tr>
          <td style={{ textAlign: 'center' }}>
            <code>channelCountMode</code>
          </td>
          <td style={{ textAlign: 'center' }}>
            <Link to="/docs/types/channel-count-mode">
              <code>ChannelCountMode</code>
            </Link>
          </td>
          <td style={{ textAlign: 'center' }}>
            <code>{channelCountMode}</code>
          </td>
          <td style={{ textAlign: 'left' }}>
            How channels are mixed between the node's inputs and outputs.
          </td>
          <td>
            <ReadOnly />
          </td>
        </tr>
        <tr>
          <td style={{ textAlign: 'center' }}>
            <code>channelInterpretation</code>
          </td>
          <td style={{ textAlign: 'center' }}>
            <Link to="/docs/types/channel-interpretation">
              <code>ChannelInterpretation</code>
            </Link>
          </td>
          <td style={{ textAlign: 'center' }}>
            <code>{channelInterpretation}</code>
          </td>
          <td style={{ textAlign: 'left' }}>
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
