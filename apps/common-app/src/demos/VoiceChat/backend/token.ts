import { NativeModules } from 'react-native';

const TOKEN_REQUEST_TIMEOUT_MS = 10_000;

function tokenUrl(): string {
  const scriptUrl = NativeModules.SourceCode?.getConstants?.().scriptURL;
  if (typeof scriptUrl === 'string') {
    const match = scriptUrl.match(/^https?:\/\/(\[[^\]]+\]|[^/:]+)/);
    if (match?.[1]) {
      const host = match[1];
      return `http://${host}:8787/token`;
    }
  }
  return 'http://localhost:8787/token';
}

export async function fetchEphemeralKey(
  controller: AbortController
): Promise<{ model: string; value: string }> {
  let timedOut = false;
  const timeout = setTimeout(() => {
    timedOut = true;
    controller.abort();
  }, TOKEN_REQUEST_TIMEOUT_MS);

  try {
    const response = await fetch(tokenUrl(), { signal: controller.signal });
    if (!response.ok) {
      throw new Error(`Token server returned ${response.status}`);
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
