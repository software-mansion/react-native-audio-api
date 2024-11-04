class ValueRangeError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'ValueRangeError';
  }
}

export default ValueRangeError;
