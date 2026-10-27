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

  const url = tokenUrl();
  try {
    const response = await fetch(url, { signal: controller.signal }).catch(
      (error) => {
        if (error instanceof Error && error.name === 'AbortError') {
          throw error;
        }
        throw new Error(
          `Could not reach the token server at ${url}. Start it with \`yarn server\` from the repository root.`
        );
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
