class InvalidAccessError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'InvalidAccessError';
  }
}

export default InvalidAccessError;
