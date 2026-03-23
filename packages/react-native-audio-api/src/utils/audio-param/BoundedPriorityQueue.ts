import { Comparator, TimestampedElement } from '../../types';

/**
 * A bounded priority queue (min-heap) with fixed capacity. When full, new
 * elements are rejected. Implements stable priority order: equal-priority
 * elements are popped in insertion order.
 *
 * @param capacity - Maximum number of elements. Must be a power of two greater
 *   than zero.
 * @param compare - Returns true if `a` has higher priority than `b` (default:
 *   min-heap by value).
 */
export class BoundedPriorityQueue<T> {
  private readonly buffer: Array<TimestampedElement<T> | undefined>;
  private size_: number;
  private globalCounter: number;

  constructor(
    readonly capacity: number = 32,
    private readonly compare: Comparator<T>
  ) {
    if (!BoundedPriorityQueue.isPowerOfTwo(capacity)) {
      throw new Error(
        `BoundedPriorityQueue capacity must be a power of two, got: ${capacity}`
      );
    }
    this.buffer = new Array(capacity).fill(undefined);
    this.size_ = 0;
    this.globalCounter = 0;
  }

  /**
   * Push a value into the priority queue.
   *
   * @returns True if pushed successfully, false if the queue is full.
   */
  public push(value: T): boolean {
    if (this.isFull()) {
      return false;
    }

    this.buffer[this.size_] = {
      data: value,
      insertionOrder: this.globalCounter++,
    };
    this.siftUp(this.size_);
    this.size_++;

    return true;
  }

  /**
   * Pop the top (highest priority) element and return it.
   *
   * @returns The popped element, or undefined if the queue is empty.
   */
  public pop(): T | undefined {
    if (this.isEmpty()) {
      return undefined;
    }

    const top = this.buffer[0]!.data;

    this.size_--;

    if (this.size_ > 0) {
      this.buffer[0] = this.buffer[this.size_];
      this.buffer[this.size_] = undefined;
      this.siftDown(0);
    } else {
      this.buffer[0] = undefined;
    }

    return top;
  }

  /**
   * Peek at the top (highest priority / front) element without removing it.
   *
   * @returns The front element, or undefined if the queue is empty.
   */
  public peekFront(): T {
    return this.buffer[0]!.data;
  }

  /**
   * Peek at the last element in the internal buffer without removing it. This
   * is the element at index size-1 in heap storage, not necessarily the lowest
   * priority.
   *
   * @returns The back element, or undefined if the queue is empty.
   */
  public peekBack(): T {
    return this.buffer[this.size_ - 1]!.data;
  }

  /**
   * Peek at the i-th element in the internal buffer (heap order, not sorted).
   * Intended for iterating over all elements without removing them.
   *
   * @param i - Index in the internal buffer.
   * @returns The element at index i, or undefined if out of bounds.
   */
  public peekAt(i: number): T {
    return this.buffer[i]!.data;
  }

  /**
   * Check if the queue is empty.
   *
   * @returns True if the queue is empty, false otherwise.
   */
  public isEmpty(): boolean {
    return this.size_ === 0;
  }

  /**
   * Check if the queue is full.
   *
   * @returns True if the queue is full, false otherwise.
   */
  public isFull(): boolean {
    return this.size_ === this.capacity;
  }

  /**
   * Get the number of elements currently in the queue.
   *
   * @returns The number of elements in the queue.
   */
  public get size(): number {
    return this.size_;
  }

  private internalCompare(
    a: TimestampedElement<T>,
    b: TimestampedElement<T>
  ): boolean {
    if (this.compare(a.data, b.data)) {
      return true;
    }

    if (this.compare(b.data, a.data)) {
      return false;
    }

    return a.insertionOrder < b.insertionOrder;
  }

  private siftUp(index: number): void {
    while (index > 0) {
      const parent = Math.floor((index - 1) / 2);

      if (this.internalCompare(this.buffer[index]!, this.buffer[parent]!)) {
        this.swapAt(index, parent);
        index = parent;
      } else {
        break;
      }
    }
  }

  private siftDown(index: number): void {
    while (true) {
      const left = 2 * index + 1;
      const right = 2 * index + 2;
      let top = index;

      if (
        left < this.size_ &&
        this.internalCompare(this.buffer[left]!, this.buffer[top]!)
      ) {
        top = left;
      }

      if (
        right < this.size_ &&
        this.internalCompare(this.buffer[right]!, this.buffer[top]!)
      ) {
        top = right;
      }

      if (top === index) {
        break;
      }

      this.swapAt(index, top);
      index = top;
    }
  }

  private swapAt(a: number, b: number): void {
    const tmp = this.buffer[a];
    this.buffer[a] = this.buffer[b];
    this.buffer[b] = tmp;
  }

  private static isPowerOfTwo(n: number): boolean {
    return n > 0 && (n & (n - 1)) === 0;
  }
}
