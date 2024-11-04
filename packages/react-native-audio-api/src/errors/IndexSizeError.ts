class IndexSizeError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'IndexSizeError';
  }
}

export default IndexSizeError;
