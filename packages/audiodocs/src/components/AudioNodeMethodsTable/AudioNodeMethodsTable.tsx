import React, { memo } from 'react';
import Link from '@docusaurus/Link';

import AudioNodeInheritedSection from '@site/src/components/AudioNodeInheritedSection';

const AudioNodeMethodsTable = () => {
  return (
    <AudioNodeInheritedSection info="methods">
      <thead>
        <tr>
          <th>Name</th>
          <th>Description</th>
        </tr>
      </thead>
      <tbody>
        <tr>
          <td>
            <Link to="/docs/core/audio-node#connect">
              <code>connect</code>
            </Link>
          </td>
          <td>
            Connects one of the node's outputs to an{' '}
            <Link to="/docs/core/audio-node">
              <code>AudioNode</code>
            </Link>{' '}
            or{' '}
            <Link to="/docs/core/audio-param">
              <code>AudioParam</code>
            </Link>{' '}
            destination.
          </td>
        </tr>
        <tr>
          <td>
            <Link to="/docs/core/audio-node#disconnect">
              <code>disconnect</code>
            </Link>
          </td>
          <td>
            Disconnects one or more outgoing connections. With no arguments,
            disconnects from all destinations.
          </td>
        </tr>
      </tbody>
    </AudioNodeInheritedSection>
  );
};

export default memo(AudioNodeMethodsTable);
