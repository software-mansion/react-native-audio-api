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

  it('falls through to a Range GET probe when HEAD succeeds without confirming Accept-Ranges', async () => {
    isFfmpegEnabledMock.mockReturnValue(true);
    fetchMock.mockImplementation(
      async (_url: string, options?: { method?: string }) => {
        if (options?.method === 'HEAD') {
          // Some Icecast/Shoutcast servers omit Accept-Ranges from HEAD even
          // though they honor a Range GET — confirmed against a real public
          // stream (SomaFM) during the aiirmobile investigation.
          return { ok: true, headers: { get: () => null } };
        }
        return { ok: true, status: 206 };
      }
    );

    await expect(
      loadRemoteHttpSource('https://example.com/stream.mp3')
    ).resolves.toBe('https://example.com/stream.mp3');

    expect(fetchMock).toHaveBeenCalledTimes(2);
    expect(fetchMock).toHaveBeenNthCalledWith(
      1,
      'https://example.com/stream.mp3',
      { method: 'HEAD', headers: undefined }
    );
    expect(fetchMock).toHaveBeenNthCalledWith(
      2,
      'https://example.com/stream.mp3',
      { headers: { Range: 'bytes=0-0' } }
    );
  });

  it('downloads when neither HEAD nor a Range GET confirm byte-range support', async () => {
    isFfmpegEnabledMock.mockReturnValue(true);
    const buffer = new ArrayBuffer(4);
    fetchMock.mockImplementation(
      async (
        _url: string,
        options?: { method?: string; headers?: { Range?: string } }
      ) => {
        if (options?.method === 'HEAD') {
          return { ok: true, headers: { get: () => null } };
        }
        if (options?.headers?.Range) {
          return { ok: true, status: 200 }; // server ignored the Range request
        }
        return { ok: true, arrayBuffer: async () => buffer };
      }
    );

    await expect(
      loadRemoteHttpSource('https://example.com/stream.mp3')
    ).resolves.toBe(buffer);

    expect(fetchMock).toHaveBeenCalledTimes(3);
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
