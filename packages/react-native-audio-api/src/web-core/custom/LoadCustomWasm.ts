export const globalTag = '__rnaaCstStretch';
const eventTitle = 'rnaaCstStretchLoaded';

export let globalWasmPromise: Promise<void> | null = null;

const LoadCustomWasm = async (pathPrefix: string = '') => {
  if (typeof window === 'undefined') {
    return null;
  }

  if (globalWasmPromise) {
    return globalWasmPromise;
  }

  globalWasmPromise = new Promise<void>((resolve) => {
    const loadScript = document.createElement('script');
    document.head.appendChild(loadScript);
    loadScript.type = 'module';

    loadScript.textContent = `
      import SignalsmithStretch from '${pathPrefix}/signalsmithStretch.mjs';
      window.${globalTag} = SignalsmithStretch;
      window.postMessage('${eventTitle}');
    `;

    function onScriptLoaded(event: MessageEvent<string>) {
      if (event.data !== eventTitle) {
        return;
      }

      resolve();
      window.removeEventListener('message', onScriptLoaded);
    }

    window.addEventListener('message', onScriptLoaded);
  });
};

export default LoadCustomWasm;
