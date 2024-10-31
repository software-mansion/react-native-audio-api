import { maxSize } from './constants';
import type { Instrument } from './types';

export const HiHat: Instrument = {
  name: 'hi-hat',
  color: '#38ACDD',
  radius: maxSize / 2,
};

const Clap: Instrument = {
  name: 'clap',
  color: '#57B495',
  radius: (maxSize / 2) * 0.8,
};

const Kick: Instrument = {
  name: 'kick',
  color: '#FFD61E',
  radius: (maxSize / 2) * 0.6,
};

const instruments: Array<Instrument> = [HiHat, Clap, Kick];

export default instruments;
