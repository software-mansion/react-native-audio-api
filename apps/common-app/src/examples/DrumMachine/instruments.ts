import { colors } from '../../styles';
import { maxSize } from './constants';
import type { Instrument } from '../../types';

export const HiHat: Instrument = {
  name: 'hi-hat',
  color: colors.yellow,
  radius: maxSize / 2,
};

const Clap: Instrument = {
  name: 'clap',
  color: colors.green,
  radius: (maxSize / 2) * 0.8,
};

const Kick: Instrument = {
  name: 'kick',
  color: colors.blue,
  radius: (maxSize / 2) * 0.6,
};

const instruments: Array<Instrument> = [HiHat, Clap, Kick];

export default instruments;
