'use strict';

const fs = require('node:fs');
const path = require('node:path');

function createXMLHttpRequest(testsPath) {
  return class XMLHttpRequest {
    constructor() {
      this.readyState = 0;
      this.status = 0;
      this.response = null;
      this.responseText = '';
      this.responseType = '';
      this.onreadystatechange = null;
      this.onload = null;
      this.onerror = null;
      this._url = '';
    }

    open(method, url) {
      this._method = method;
      this._url = String(url);
      this.readyState = 1;
      this.#emitReadyStateChange();
    }

    send() {
      try {
        const normalized = this._url.replace(/^https?:\/\/[^/]+\//, '');
        const filePath = path.join(testsPath, '..', normalized);
        const body = fs.readFileSync(filePath);
        this.status = 200;
        this.readyState = 4;
        if (this.responseType === 'arraybuffer') {
          this.response = body.buffer.slice(body.byteOffset, body.byteOffset + body.byteLength);
        } else {
          this.responseText = body.toString('utf8');
          this.response = this.responseText;
        }
        this.#emitReadyStateChange();
        if (typeof this.onload === 'function') {
          this.onload();
        }
      } catch (error) {
        this.status = 404;
        if (typeof this.onerror === 'function') {
          this.onerror(error);
        }
      }
    }

    #emitReadyStateChange() {
      if (typeof this.onreadystatechange === 'function') {
        this.onreadystatechange();
      }
    }
  };
}

module.exports = createXMLHttpRequest;
