import { createServer } from 'node:http';
import { pathToFileURL } from 'node:url';

const port = Number(process.env.TOKEN_SERVER_PORT ?? 8787);
const model = 'gpt-realtime-2.1-mini';

const session = {
  type: 'realtime',
  model,
  output_modalities: ['audio'],
  instructions:
    'You are a concise English-only voice assistant in a technical demo. Keep answers to one or two short sentences unless asked for detail. Do not switch languages because of an accent, filler words, or uncertain audio. Do not answer audio that is not addressed to you or is unintelligible.',
  audio: {
    input: {
      format: { type: 'audio/pcm', rate: 24000 },
      noise_reduction: { type: 'near_field' },
      transcription: {
        model: 'gpt-4o-mini-transcribe',
        language: 'en',
      },
      turn_detection: {
        type: 'semantic_vad',
        eagerness: 'low',
        create_response: false,
        interrupt_response: true,
      },
    },
    output: {
      format: { type: 'audio/pcm', rate: 24000 },
      voice: 'marin',
    },
  },
};

function sendJson(response, status, body) {
  response.writeHead(status, {
    'Content-Type': 'application/json',
    'Cache-Control': 'no-store',
  });
  response.end(JSON.stringify(body));
}

export const server = createServer(async (request, response) => {
  if (request.method !== 'GET') {
    sendJson(response, 405, { error: 'Method not allowed' });
    return;
  }

  if (request.url === '/health') {
    sendJson(response, 200, { ok: true, model });
    return;
  }

  if (request.url !== '/token') {
    sendJson(response, 404, { error: 'Not found' });
    return;
  }

  const apiKey = process.env.OPENAI_API_KEY;
  if (!apiKey) {
    sendJson(response, 500, {
      error: 'Missing OPENAI_API_KEY on the token server',
    });
    return;
  }

  try {
    const upstream = await fetch(
      'https://api.openai.com/v1/realtime/client_secrets',
      {
        method: 'POST',
        headers: {
          'Authorization': `Bearer ${apiKey}`,
          'Content-Type': 'application/json',
          'OpenAI-Safety-Identifier': 'fabric-example-voice-demo',
        },
        body: JSON.stringify({ session }),
        signal: AbortSignal.timeout(8_000),
      }
    );
    const body = await upstream.json();
    if (!upstream.ok) {
      console.error('OpenAI client-secret request failed:', upstream.status);
      sendJson(response, upstream.status, {
        error: 'Could not create a Realtime client secret',
      });
      return;
    }
    if (typeof body.value !== 'string' || !body.value) {
      throw new Error('Missing client secret');
    }

    sendJson(response, 200, {
      value: body.value,
      expires_at: body.expires_at,
      model,
    });
  } catch {
    console.error('OpenAI client-secret request failed');
    sendJson(response, 500, {
      error: 'Could not create a Realtime client secret',
    });
  }
});

if (
  process.argv[1] &&
  import.meta.url === pathToFileURL(process.argv[1]).href
) {
  server.listen(port, '0.0.0.0', () => {
    console.log(`Voice token server listening on http://0.0.0.0:${port}`);
  });
}
