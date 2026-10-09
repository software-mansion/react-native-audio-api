import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';

const libraryScript = fs.readFileSync(
  path.join(
    __dirname,
    '../src/web-core/custom/wasm-audio-bufffer-source-node-stretcher/signalsmithStretch/SignalsmithStretch.mjs'
  ),
  'utf8'
);

function deferred<T>() {
  let resolve!: (value: T) => void;
  const promise = new Promise<T>((done) => {
    resolve = done;
  });
  return { promise, resolve };
}

describe.each([
  ['library', libraryScript],
  [
    'documentation asset',
    fs.readFileSync(
      path.join(__dirname, '../../audiodocs/static/signalsmithStretch.mjs'),
      'utf8'
    ),
  ],
])('%s', (_name, script) => {
  function processorHarness() {
    const ready = deferred<Record<string, unknown>>();
    let Processor: any;
    const port = {
      onmessage: null as any,
      postMessage: jest.fn(),
      close: jest.fn(),
    };
    const scope = {
      AudioWorkletProcessor: class {
        port = port;
      },
      registerProcessor: (_key: string, value: any) => {
        Processor = value;
      },
      sampleRate: 48000,
      currentTime: 0,
      Module: () => ready.promise,
    };
    const register = script.slice(
      script.indexOf('function registerWorkletProcessor'),
      script.indexOf('/**\n\tCreates a Stretch node')
    );
    vm.runInNewContext(
      `${register}\nregisterWorkletProcessor(Module, 'test');`,
      scope
    );
    return { processor: new Processor({}), port, ready };
  }

  function clientHarness(deferModule = false) {
    const moduleReady = deferred<void>();
    let registered = !deferModule;
    const port = {
      onmessage: null as any,
      postMessage: jest.fn(),
      close: jest.fn(),
    };
    const disconnect = jest.fn();
    const context = {
      state: 'running',
      audioWorklet: { addModule: jest.fn(() => moduleReady.promise) },
      addEventListener: jest.fn(),
      removeEventListener: jest.fn(),
    };
    const scope: any = {
      SignalsmithStretch: () => {},
      AudioWorkletNode: class {
        port = port;
        disconnect = disconnect;
        constructor() {
          if (!registered) throw new Error('Module not registered');
        }
      },
      AbortController,
      DOMException,
    };
    const client = script.slice(
      script.indexOf('SignalsmithStretch = ((Module'),
      script.indexOf('// register as a CommonJS')
    );
    vm.runInNewContext(client, scope);
    scope.SignalsmithStretch.moduleUrl = '/worklet.js';
    return {
      create: scope.SignalsmithStretch,
      port,
      context,
      disconnect,
      registerModule: () => {
        registered = true;
        moduleReady.resolve();
      },
    };
  }

  describe('pitch-correction processor disposal', () => {
    it('retires before WASM readiness without running late initialization', async () => {
      const { processor, port, ready } = processorHarness();
      port.onmessage({ data: [0, 'addBuffers', [new Float32Array(128)]] });
      port.onmessage({ data: [1, 'dispose'] });
      expect(processor.process([], [[new Float32Array(128)]], {})).toBe(false);
      const main = jest.fn();
      ready.resolve({ _main: main });
      await Promise.resolve();
      expect(main).not.toHaveBeenCalled();
      expect(processor.audioBuffers).toEqual([]);
      expect(processor.wasmModule).toBeNull();
    });

    it('drops retained PCM, timeline and WASM references after readiness', async () => {
      const { processor, port, ready } = processorHarness();
      ready.resolve({
        _main: jest.fn(),
        _presetDefault: jest.fn(),
        _inputLatency: () => 128,
        _outputLatency: () => 128,
        _setBuffers: () => 0,
      });
      await Promise.resolve();
      port.onmessage({ data: [0, 'addBuffers', [new Float32Array(128)]] });
      expect(processor.audioBuffers).toHaveLength(1);
      port.onmessage({ data: [1, 'dispose'] });
      expect(processor.process([], [[new Float32Array(128)]], {})).toBe(false);
      expect(processor.audioBuffers).toEqual([]);
      expect(processor.timeMap).toEqual([]);
      expect(processor.wasmModule).toBeNull();
    });
  });

  describe('pitch-correction message client disposal', () => {
    it('settles outstanding requests and closes the port without waiting for a reply', async () => {
      const { create, port, context, disconnect } = clientHarness();
      const pendingNode = create(context);
      port.onmessage({ data: ['ready', { addBuffers: 1, stop: 1 }] });
      const node = await pendingNode;
      const request = node.addBuffers([new Float32Array(128)]);
      node.dispose();
      node.dispose();
      await expect(request).resolves.toBeUndefined();
      expect(port.close).toHaveBeenCalledTimes(1);
      expect(disconnect).toHaveBeenCalledTimes(1);
      expect(port.onmessage).toBeNull();
      const sent = port.postMessage.mock.calls.length;
      await node.stop();
      expect(port.postMessage).toHaveBeenCalledTimes(sent);
    });

    it('cancels a factory waiting for WASM readiness', async () => {
      const { create, port, context } = clientHarness();
      const controller = new AbortController();
      const pending = create(context, undefined, controller.signal);
      controller.abort();
      await expect(pending).rejects.toMatchObject({ name: 'AbortError' });
      expect(port.postMessage).toHaveBeenCalledWith([
        expect.any(Number),
        'dispose',
      ]);
      expect(port.close).toHaveBeenCalledTimes(1);
    });

    it('cancels only its own module wait, allowing another source to initialize', async () => {
      const { create, context, port, registerModule } = clientHarness(true);
      const controller = new AbortController();
      let outcome = 'pending';
      const canceled = create(context, undefined, controller.signal).catch(
        (error: Error) => {
          outcome = error.name;
        }
      );
      const other = create(context);
      controller.abort();
      for (let i = 0; i < 10; i++) await Promise.resolve();
      expect(outcome).toBe('AbortError');
      expect(context.audioWorklet.addModule).toHaveBeenCalledTimes(1);
      registerModule();
      for (let i = 0; i < 10; i++) await Promise.resolve();
      port.onmessage({ data: ['ready', {}] });
      const node = await other;
      node.dispose();
      await canceled;
    });

    it('rejects a pending module wait when the context closes', async () => {
      const { create, context } = clientHarness(true);
      let outcome = 'pending';
      const pending = create(context).catch((error: Error) => {
        outcome = error.name;
      });
      context.state = 'closed';
      context.addEventListener.mock.calls.forEach((call) => call[1]());
      for (let i = 0; i < 10; i++) await Promise.resolve();
      expect(outcome).toBe('AbortError');
      await pending;
    });

    it('does not leave readiness pending when the context closes', async () => {
      const { create, port, context } = clientHarness();
      const pending = create(context);
      context.state = 'closed';
      context.addEventListener.mock.calls[0][1]();
      await expect(pending).rejects.toMatchObject({ name: 'AbortError' });
      expect(port.close).toHaveBeenCalledTimes(1);
    });
  });
});
