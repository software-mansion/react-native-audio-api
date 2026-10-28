import assert from 'node:assert/strict';
import { once } from 'node:events';
import { after, before, test } from 'node:test';
import { server } from './realtime-token-server.mjs';

const originalFetch = globalThis.fetch;
const originalKey = process.env.OPENAI_API_KEY;
let baseUrl;

before(async () => {
  server.listen(0, '127.0.0.1');
  await once(server, 'listening');
  baseUrl = `http://127.0.0.1:${server.address().port}`;
});

after(async () => {
  globalThis.fetch = originalFetch;
  if (originalKey === undefined) {
    delete process.env.OPENAI_API_KEY;
  } else {
    process.env.OPENAI_API_KEY = originalKey;
  }
  await new Promise((resolve, reject) => {
    server.close((error) => (error ? reject(error) : resolve()));
    server.closeAllConnections();
  });
});

test('health succeeds without an API key; token requests require one', async () => {
  delete process.env.OPENAI_API_KEY;
  const health = await originalFetch(`${baseUrl}/health`);
  assert.equal(health.status, 200);
  assert.equal((await health.json()).ok, true);

  const token = await originalFetch(`${baseUrl}/token`);
  assert.equal(token.status, 500);
  assert.match((await token.json()).error, /Missing OPENAI_API_KEY/);
});

test('mints only an ephemeral secret using the fixed voice policy', async () => {
  process.env.OPENAI_API_KEY = 'server-only-test-key';
  globalThis.fetch = async (url, options) => {
    assert.equal(url, 'https://api.openai.com/v1/realtime/client_secrets');
    assert.equal(options.headers.Authorization, 'Bearer server-only-test-key');
    const { session } = JSON.parse(options.body);
    assert.equal(session.model, 'gpt-realtime-2.1-mini');
    assert.equal(session.audio.output.voice, 'marin');
    assert.deepEqual(session.audio.input.format, {
      type: 'audio/pcm',
      rate: 24000,
    });
    assert.equal(session.audio.input.transcription.language, 'en');
    assert.deepEqual(session.audio.input.turn_detection, {
      type: 'semantic_vad',
      eagerness: 'low',
      create_response: false,
      interrupt_response: true,
    });
    assert.deepEqual(session.audio.input.noise_reduction, {
      type: 'near_field',
    });
    return Response.json({ value: 'ephemeral-test-secret', expires_at: 123 });
  };

  const response = await originalFetch(`${baseUrl}/token`);
  assert.equal(response.headers.get('cache-control'), 'no-store');
  assert.deepEqual(await response.json(), {
    value: 'ephemeral-test-secret',
    expires_at: 123,
    model: 'gpt-realtime-2.1-mini',
  });
});

test('upstream failures never forward credentials or upstream bodies', async () => {
  globalThis.fetch = async () =>
    Response.json(
      { error: { message: 'server-only-test-key' } },
      { status: 401 }
    );
  const response = await originalFetch(`${baseUrl}/token`);
  assert.equal(response.status, 401);
  assert.doesNotMatch(await response.text(), /server-only-test-key/);
});
