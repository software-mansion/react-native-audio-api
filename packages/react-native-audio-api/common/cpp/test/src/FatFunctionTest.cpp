#include <audioapi/utils/FatFunction.hpp>
#include <gtest/gtest.h>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

using namespace audioapi;

using IntOp = int(int, int); // NOLINT(readability/casting)

class FatFunctionTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }
};

TEST(FatFunctionTest, BasicFunctionality) {
  FatFunction<64, IntOp> add = [](int a, int b) {
    return a + b;
  };
  EXPECT_EQ(add(2, 3), 5);
  // FatFunction contains both raw storage and function pointers.
  // The compiler may add tail padding so each instance still satisfies
  // alignof(std::max_align_t), especially when objects are placed in arrays.
  // We therefore compare against the aligned size, not just the raw field sum.
  constexpr size_t dataSize = 64 + sizeof(void *) * 3;
  constexpr size_t alignment = alignof(std::max_align_t);
  constexpr size_t expectedSize = ((dataSize + alignment - 1) / alignment) * alignment;
  EXPECT_EQ(sizeof(add), expectedSize);
}

TEST(FatFunctionTest, MoveSemantics) {
  FatFunction<64, IntOp> add = [](int a, int b) {
    return a + b;
  };
  FatFunction<64, IntOp> movedAdd = std::move(add);
  EXPECT_EQ(movedAdd(4, 5), 9);
  EXPECT_THROW(add(1, 2), std::bad_function_call); // Original should be empty
}

TEST(FatFunctionTest, Release) {
  int destructorCalls = 0;
  struct Tracked {
    int *counter;
    explicit Tracked(int *c) : counter(c) {}
    int operator()(int a, int b) const {
      return a + b;
    }
    ~Tracked() {
      (*counter)++;
    }
  };

  {
    FatFunction<64, IntOp> add = Tracked(&destructorCalls);
    // we comment this because compiler can optimize it and do not call destructor here
    // EXPECT_EQ(destructorCalls, 1); // Destructor called for the temporary Tracked object, but not for the one inside add
    destructorCalls = 0; // Reset counter after construction
    auto [storage, deleter] = add.release();
    EXPECT_GT(storage.size(), 0);
    EXPECT_NE(deleter, nullptr);

    // We can call the deleter to clean up resources if needed
    if (deleter) {
      deleter(storage.data());
    }
  } // FatFunction goes out of scope here, but it was released so it shouldn't destroy the object again

  EXPECT_EQ(destructorCalls, 1); // Destructor should have been called exactly once from the deleter
}

TEST(FatFunctionTest, EmptyFunctionCall) {
  FatFunction<64, IntOp> emptyFunc;
  EXPECT_THROW(emptyFunc(1, 2), std::bad_function_call);
}

TEST(FatFunctionTest, SwapFunctions) {
  FatFunction<64, IntOp> add = [](int a, int b) {
    return a + b;
  };
  FatFunction<64, IntOp> multiply = [](int a, int b) {
    return a * b;
  };

  std::swap(add, multiply);

  EXPECT_EQ(add(2, 3), 6);      // Now add should multiply
  EXPECT_EQ(multiply(2, 3), 5); // Now multiply should add
}

TEST(FatFunctionTest, LargeCallable) {
  struct LargeCallable {
    int operator()(int a, int b) const {
      return a * b;
    }
    char data[65]; // This makes it larger than the storage size
  };

  // This should fail to compile because LargeCallable exceeds the storage size
  bool isConstructible = std::is_constructible_v<FatFunction<64, IntOp>, LargeCallable>;
  EXPECT_FALSE(isConstructible);
}

TEST(FatFunctionTest, TriviallyMoveableCallable) {
  struct TrivialCallable {
    int operator()(int a, int b) const {
      return a - b;
    }
  };

  FatFunction<64, IntOp> func = TrivialCallable();
  EXPECT_EQ(func(5, 3), 2);
}

TEST(FatFunctionTest, SmallerToLargerMove) {
  FatFunction<32, IntOp> smallFunc = [](int a, int b) {
    return a + b;
  };
  FatFunction<64, IntOp> largeFunc = std::move(smallFunc);
  EXPECT_EQ(largeFunc(2, 3), 5);
  EXPECT_THROW(smallFunc(1, 2), std::bad_function_call); // Original should be empty
}

TEST(FatFunctionTest, LargerToSmallerMove) {
  FatFunction<64, IntOp> largeFunc = [](int a, int b) {
    return a * b;
  };
  // This should fail to compile because largeFunc exceeds the storage size of smallFunc
  bool isConstructible = std::is_constructible_v<FatFunction<32, IntOp>, decltype(largeFunc)>;
  EXPECT_FALSE(isConstructible);
}

TEST(FatFunctionTest, SmallerToLargerMoveWithNonTrivialMoveAndDestruct) {
  int destructorCalled = 0;
  int moverCalled = 0;

  struct NonTrivialCallable {
    int *dCounter;
    int *mCounter;

    NonTrivialCallable(int *d, int *m) : dCounter(d), mCounter(m) {}

    int operator()(int a, int b) const {
      return a + b;
    }
    ~NonTrivialCallable() {
      if (dCounter)
        (*dCounter)++;
    }
    NonTrivialCallable(NonTrivialCallable &&other)
        : dCounter(other.dCounter), mCounter(other.mCounter) {
      if (mCounter)
        (*mCounter)++;
    }
    NonTrivialCallable(const NonTrivialCallable &) = delete; // Non-copyable
  };

  {
    FatFunction<32, IntOp> smallFunc = NonTrivialCallable(&destructorCalled, &moverCalled);

    // Initial construction involves move from temporary
    EXPECT_EQ(moverCalled, 1);
    EXPECT_EQ(destructorCalled, 1);

    FatFunction<64, IntOp> largeFunc = std::move(smallFunc);

    // Move to largeFunc invokes move ctor + destruction of smallFunc's content
    EXPECT_EQ(moverCalled, 2);
    EXPECT_EQ(destructorCalled, 2);

    EXPECT_EQ(largeFunc(2, 3), 5);
    EXPECT_THROW(smallFunc(1, 2), std::bad_function_call); // Original should be empty
  }
  EXPECT_EQ(moverCalled, 2);
  EXPECT_EQ(destructorCalled, 3);
}

TEST(FatFunctionTest, MutableLambdaBasic) {
  // A mutable lambda that modifies captured state on each call
  int counter = 0;
  FatFunction<64, IntOp> func = [counter](int a, int b) mutable {
    counter++;
    return a + b + counter;
  };
  EXPECT_EQ(func(1, 2), 4); // 1 + 2 + 1
  EXPECT_EQ(func(1, 2), 5); // 1 + 2 + 2
  EXPECT_EQ(func(1, 2), 6); // 1 + 2 + 3
}

TEST(FatFunctionTest, MutableLambdaWithUniquePtr) {
  // Mutable lambda capturing a unique_ptr — the original Graph.hpp use case
  auto ptr = std::make_unique<int>(42);
  FatFunction<64, IntOp> func = [ptr = std::move(ptr)](int a, int b) mutable -> int {
    int val = *ptr;
    ptr.reset(); // mutates the captured unique_ptr
    return a + b + val;
  };
  EXPECT_EQ(func(1, 2), 45); // 1 + 2 + 42
}

TEST(FatFunctionTest, MutableLambdaStateIsolatedAfterMove) {
  // Verify that state is correctly preserved when a mutable-lambda FatFunction is moved
  FatFunction<64, IntOp> func = [n = 0](int a, int b) mutable {
    n++;
    return a + b + n;
  };
  EXPECT_EQ(func(0, 0), 1); // n becomes 1
  EXPECT_EQ(func(0, 0), 2); // n becomes 2

  FatFunction<64, IntOp> moved = std::move(func);
  EXPECT_THROW(func(0, 0), std::bad_function_call); // original is empty

  // Moved function should continue from where the state left off
  EXPECT_EQ(moved(0, 0), 3); // n becomes 3
  EXPECT_EQ(moved(0, 0), 4); // n becomes 4
}

TEST(FatFunctionTest, MutableLambdaSmallerToLargerMove) {
  FatFunction<32, IntOp> small = [n = 0](int a, int b) mutable {
    n += 10;
    return a + b + n;
  };
  EXPECT_EQ(small(1, 1), 12); // n = 10

  FatFunction<64, IntOp> large = std::move(small);
  EXPECT_EQ(large(1, 1), 22); // n = 20
}

TEST(FatFunctionTest, MutableLambdaVoidReturn) {
  using VoidOp = void(int, int); // NOLINT(readability/casting)
  int accumulator = 0;
  FatFunction<64, VoidOp> func = [&accumulator, calls = 0](int a, int b) mutable {
    calls++;
    accumulator += (a + b) * calls;
  };
  func(1, 2); // calls=1, accumulator += 3*1 = 3
  func(1, 2); // calls=2, accumulator += 3*2 = 9
  EXPECT_EQ(accumulator, 9);
}
