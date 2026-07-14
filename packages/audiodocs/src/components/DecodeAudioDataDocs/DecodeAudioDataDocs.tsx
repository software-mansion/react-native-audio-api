import React, { memo } from 'react';
import Link from '@docusaurus/Link';
import Admonition from '@theme/Admonition';
import CodeBlock from '@theme/CodeBlock';

import { MobileOnly, Optional } from '@site/src/components/Badges';

type DecodeAudioDataDocsVariant = 'standalone' | 'context';

interface DecodeAudioDataDocsProps {
  variant: DecodeAudioDataDocsVariant;
}

const arrayBufferLink =
  'https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/ArrayBuffer';
const requestInitLink =
  'https://github.com/facebook/react-native/blob/ac06f3bdc76a9fd7c65ab899e82bff5cad9b94b6/packages/react-native/src/types/globals.d.ts#L265';
const staticResourcesLink =
  'https://reactnative.dev/docs/images#static-non-image-resources';

const DecodeAudioDataDocs = ({ variant }: DecodeAudioDataDocsProps) => {
  const isStandalone = variant === 'standalone';

  const exampleCode = isStandalone
    ? `import { decodeAudioData } from 'react-native-audio-api';

const url = ... // url to an audio

const buffer = await decodeAudioData(url);`
    : `const url = ... // url to an audio

const buffer = await audioContext.decodeAudioData(url);`;

  return (
    <>
      <p>
        Decodes audio data from a file path, an{' '}
        <Link to={arrayBufferLink}>
          <code>ArrayBuffer</code>
        </Link>
        , or a bundled app asset (a <code>number</code> module id returned by{' '}
        <code>require('./audio.mp3')</code>).
        {isStandalone ? (
          <>
            {' '}
            The optional <code>sampleRate</code> parameter lets you resample the
            decoded audio; if not provided, the original sample rate from the file
            is used.
          </>
        ) : (
          <>
            {' '}
            The decoded audio is automatically resampled to match the audio
            context&apos;s <code>sampleRate</code>.
          </>
        )}
      </p>

      {!isStandalone && (
        <p>
          <strong>
            For the list of supported formats visit{' '}
            <Link to="/docs/utils/decoding">this page</Link>.
          </strong>
        </p>
      )}

      <table>
        <thead>
          <tr>
            <th style={{ textAlign: 'center' }}>Parameter</th>
            <th style={{ textAlign: 'center' }}>Type</th>
            <th style={{ textAlign: 'left' }}>Description</th>
          </tr>
        </thead>
        <tbody>
          <tr>
            <td rowSpan={3} style={{ textAlign: 'center' }}>
              <code>input</code>
            </td>
            <td style={{ textAlign: 'center' }}>
              <Link to={arrayBufferLink}>
                <code>ArrayBuffer</code>
              </Link>
            </td>
            <td style={{ textAlign: 'left' }}>
              <Link to={arrayBufferLink}>
                <code>ArrayBuffer</code>
              </Link>{' '}
              with encoded audio data.
            </td>
          </tr>
          <tr>
            <td style={{ textAlign: 'center' }}>
              <code>string</code>
            </td>
            <td style={{ textAlign: 'left' }}>
              Path to a remote or local audio file.
            </td>
          </tr>
          <tr>
            <td style={{ textAlign: 'center' }}>
              <code>number</code> <MobileOnly />
            </td>
            <td style={{ textAlign: 'left' }}>
              React Native asset module id (for example, the value returned by{' '}
              <code>require('./audio.mp3')</code>).
            </td>
          </tr>
          {isStandalone && (
            <tr>
              <td style={{ textAlign: 'center' }}>
                <code>sampleRate</code>
                <Optional />
              </td>
              <td style={{ textAlign: 'center' }}>
                <code>number</code>
              </td>
              <td style={{ textAlign: 'left' }}>
                Target sample rate for the decoded audio.
              </td>
            </tr>
          )}
          <tr>
            <td style={{ textAlign: 'center' }}>
              <code>fetchOptions</code>
              <Optional />
            </td>
            <td style={{ textAlign: 'center' }}>
              <Link to={requestInitLink}>
                <code>RequestInit</code>
              </Link>
            </td>
            <td style={{ textAlign: 'left' }}>
              Additional fetch options when <code>input</code> is a remote URL
              (for example, auth headers).
            </td>
          </tr>
        </tbody>
      </table>

      <Admonition type="caution">
        If you pass a <code>number</code> as <code>input</code>, decoding resolves
        the bundled asset through React Native&apos;s{' '}
        <Link to="https://reactnative.dev/docs/image">
          <code>Image</code>
        </Link>{' '}
        component. By default, only <code>.mp3</code>, <code>.wav</code>,{' '}
        <code>.mp4</code>, <code>.m4a</code>, and <code>.aac</code> assets are
        supported. To use other formats, see{' '}
        <Link to={staticResourcesLink}>
          static non-image resources
        </Link>
        .
      </Admonition>

      <p>
        <strong>Returns:</strong> <code>Promise&lt;AudioBuffer&gt;</code>
      </p>

      <details>
        <summary>
          Example decoding {isStandalone ? 'remote URL' : 'audio'}
        </summary>
        <CodeBlock language="tsx">{exampleCode}</CodeBlock>
      </details>
    </>
  );
};

export default memo(DecodeAudioDataDocs);
