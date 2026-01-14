class AudioApiError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'AudioApiError';
  }
}

export default AudioApiError;
