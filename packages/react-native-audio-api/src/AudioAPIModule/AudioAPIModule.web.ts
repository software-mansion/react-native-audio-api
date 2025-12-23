import type { IAudioAPIModule, IWorkletsModule } from './ModuleInterfaces';

const mockGetter = <T>(value: T) => value;

class AudioAPIModule implements IAudioAPIModule {
  public supportedWorkletsVersion = [];
  workletsModule = mockGetter(null as IWorkletsModule | null);
  canUseWorklets = mockGetter(false);
  workletsVersion = mockGetter('unknown');
  areWorkletsAvailable = mockGetter(false);
  isWorkletsVersionSupported = mockGetter(false);

  createAudioRuntime() {
    return null;
  }
}

export default new AudioAPIModule();
