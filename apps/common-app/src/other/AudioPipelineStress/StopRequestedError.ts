export default class StopRequestedError extends Error {
  constructor() {
    super('Run stopped by user');
    this.name = 'StopRequestedError';
  }
}
