class RangeError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'RangeError';
  }
}

export default RangeError;
