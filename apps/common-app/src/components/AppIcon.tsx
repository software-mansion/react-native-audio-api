import type { ComponentProps } from 'react';
import { createNanoIconSet } from 'react-native-nano-icons';

import glyphMap from '../../assets/icons/nanoicons/app.glyphmap.json';

/**
 * Renders an icon from `assets/icons/app` as a native font glyph. After adding
 * an SVG there, run `npx react-native-nano-icons` in `apps/fabric-example` to
 * regenerate the font and link it into the native projects.
 */
const AppIcon = createNanoIconSet(glyphMap);

export type AppIconName = ComponentProps<typeof AppIcon>['name'];

export default AppIcon;
