import type { ImageSourcePropType } from 'react-native';

export interface Track {
  title: string;
  artist: string;
  cover: ImageSourcePropType;
  /** A bundled asset (`require`) or a URL. */
  audio: number | string;
  /**
   * Gain that brings the track to about -14 LUFS (integrated loudness measured
   * with ffmpeg's ebur128 filter), so a crossfade doesn't jump in volume.
   */
  levelDb: number;
}

const docsMusic =
  'https://docs.swmansion.com/react-native-audio-api/audio/music';

/**
 * Cover 6 is generated abstract art. Tracks 3 and 4 carry no tags, so their
 * titles are descriptive. The last two are the docs' example music, streamed
 * instead of bundled a second time.
 */
export const TRACKS: Track[] = [
  {
    title: 'Up-Beat',
    artist: 'Lo-Fi Boy',
    cover: require('./images/image_1.jpeg'),
    audio: require('./tracks/track1.mp3'),
    levelDb: -5.8,
  },
  {
    title: 'Chill',
    artist: 'Lo-Fi Boy',
    cover: require('./images/image_2.jpg'),
    audio: require('./tracks/track2.mp3'),
    levelDb: -1.5,
  },
  {
    title: 'Daybreak',
    artist: 'Unknown artist',
    cover: require('./images/image_3.jpg'),
    audio: require('./tracks/track3.mp3'),
    levelDb: 2.0,
  },
  {
    title: 'Drift',
    artist: 'Unknown artist',
    cover: require('./images/image_4.jpg'),
    audio: require('./tracks/track4.mp3'),
    levelDb: 0.7,
  },
  {
    title: 'Swimming Pool Ketchup',
    artist: 'Jimit',
    cover: require('./images/image_5.jpg'),
    audio: `${docsMusic}/example-music-05.mp3`,
    levelDb: -2.6,
  },
  {
    title: 'Pass the Mayo',
    artist: 'Amit Katzengold',
    cover: require('./images/image_6.jpg'),
    audio: `${docsMusic}/pass-the-mayo.mp3`,
    levelDb: -1.3,
  },
];
