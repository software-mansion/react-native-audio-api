import { Platform } from 'react-native';

import { metroHost } from '../../../utils/metroHost';

const TOKEN_REQUEST_TIMEOUT_MS = 10_000;

function tokenUrl(): string {
  return `http://${metroHost() ?? 'localhost'}:8787/token`;
}

/**
 * `adb reverse` gives a USB-connected Android device the laptop's `localhost`
 * for the bundle, so the token server's port needs forwarding as well.
 */
function unreachableMessage(url: string): string {
  const start = `Could not reach the token server at ${url}. Start it with \`pnpm server\` from the repository root.`;
  const viaAdbReverse =
    Platform.OS === 'android' &&
    /^http:\/\/(localhost|127\.0\.0\.1):/.test(url);
  return viaAdbReverse
    ? `${start} Over USB, also run \`adb reverse tcp:8787 tcp:8787\`.`
    : start;
}

export async function fetchEphemeralKey(
  controller: AbortController
): Promise<{ model: string; value: string }> {
  let timedOut = false;
  const timeout = setTimeout(() => {
    timedOut = true;
    controller.abort();
  }, TOKEN_REQUEST_TIMEOUT_MS);

  const url = tokenUrl();
  try {
    const response = await fetch(url, { signal: controller.signal }).catch(
      (error) => {
        if (error instanceof Error && error.name === 'AbortError') {
          throw error;
        }
        throw new Error(unreachableMessage(url));
      }
    );
    if (!response.ok) {
      const body = (await response.json().catch(() => ({}))) as {
        error?: string;
      };
      throw new Error(body.error ?? `Token server returned ${response.status}`);
    }

    const body = (await response.json()) as { model?: string; value?: string };
    if (!body.value || !body.model) {
      throw new Error('Token server returned no client secret');
    }
    return { model: body.model, value: body.value };
  } catch (error) {
    if (timedOut && error instanceof Error && error.name === 'AbortError') {
      throw new Error('Token server did not respond in time');
    }
    throw error;
  } finally {
    clearTimeout(timeout);
  }
}
