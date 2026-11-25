import { NativeModules } from 'react-native';

/**
 * Host of the Metro server that served the JS bundle, which is usually the
 * laptop running the example's companion servers. Null in a release build.
 */
export function metroHost(): string | null {
  const scriptUrl = NativeModules.SourceCode?.getConstants?.().scriptURL;
  if (typeof scriptUrl !== 'string') {
    return null;
  }
  return scriptUrl.match(/^https?:\/\/(\[[^\]]+\]|[^/:]+)/)?.[1] ?? null;
}
