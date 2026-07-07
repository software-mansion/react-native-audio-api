'use strict';

const fs = require('node:fs/promises');
const path = require('node:path');

function createFetch(wptRootPath) {
  return async function fetch(input) {
    const url = String(input);
    const normalized = url.replace(/^https?:\/\/[^/]+\//, '');
    const filePath = path.join(wptRootPath, normalized);
    const body = await fs.readFile(filePath);

    return {
      ok: true,
      status: 200,
      text: async () => body.toString('utf8'),
      arrayBuffer: async () =>
        body.buffer.slice(body.byteOffset, body.byteOffset + body.byteLength),
      json: async () => JSON.parse(body.toString('utf8')),
    };
  };
}

module.exports = createFetch;
