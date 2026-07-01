import { isFfmpegEnabled } from '../src/utils/flags';
import { loadRemoteHttpSource } from '../src/utils/remoteHttpSource';

jest.mock('../src/utils/flags', () => ({
  isFfmpegEnabled: jest.fn(),
}));

describe('loadRemoteHttpSource', () => {
  const fetchMock = jest.fn();
  const isFfmpegEnabledMock = isFfmpegEnabled as jest.MockedFunction<
    typeof isFfmpegEnabled
  >;

  beforeEach(() => {
    fetchMock.mockReset();
    isFfmpegEnabledMock.mockReset();
    global.fetch = fetchMock as typeof fetch;
  });

  it('downloads when FFmpeg is disabled', async () => {
    isFfmpegEnabledMock.mockReturnValue(false);
    const buffer = new ArrayBuffer(8);
    fetchMock.mockResolvedValue({
      ok: true,
      arrayBuffer: async () => buffer,
    });

    await expect(
      loadRemoteHttpSource('https://example.com/song.mp3')
    ).resolves.toBe(buffer);

    expect(fetchMock).toHaveBeenCalledTimes(1);
    expect(fetchMock).toHaveBeenCalledWith('https://example.com/song.mp3', {
      headers: undefined,
    });
  });

  it('streams via URL when FFmpeg is enabled and byte ranges work', async () => {
    isFfmpegEnabledMock.mockReturnValue(true);
    fetchMock.mockResolvedValue({
      ok: true,
      headers: { get: () => 'bytes' },
    });

    await expect(
      loadRemoteHttpSource('https://example.com/song.mp3', {
        Authorization: 'token',
      })
    ).resolves.toBe('https://example.com/song.mp3');

    expect(fetchMock).toHaveBeenCalledWith('https://example.com/song.mp3', {
      method: 'HEAD',
      headers: { Authorization: 'token' },
    });
  });

  it('downloads when forceDownload is true even if byte ranges work', async () => {
    isFfmpegEnabledMock.mockReturnValue(true);
    const buffer = new ArrayBuffer(8);
    fetchMock.mockResolvedValue({
      ok: true,
      arrayBuffer: async () => buffer,
    });

    await expect(
      loadRemoteHttpSource(
        'https://example.com/song.mp3',
        { Authorization: 'token' },
        true
      )
    ).resolves.toBe(buffer);

    expect(fetchMock).toHaveBeenCalledTimes(1);
    expect(fetchMock).toHaveBeenCalledWith('https://example.com/song.mp3', {
      headers: { Authorization: 'token' },
    });
  });
});
