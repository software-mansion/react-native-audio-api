#pragma once

#include <sys/resource.h>
#include <cstddef>
#include <cstdint>

namespace audioapi::test {

/// @brief Thread-local allocation auditor for real-time safety testing.
///
/// When armed on a thread, every call to the globally-overridden
/// operator new / operator delete increments a per-thread violation
/// counter. Use the RAII `Scope` to bracket allocation-free zones.
///
/// Context-switch counters are also sampled via getrusage:
///  - Linux (RUSAGE_THREAD): per-thread, exact.
///  - macOS (RUSAGE_SELF):   process-wide, approximate — keep other
///    threads idle during measurement for best accuracy.
///
/// ## Example
/// ```cpp
/// std::thread audioThread([&] {
///   AudioThreadGuard::Scope guard;
///   graph.processEvents();
///   graph.process();
///   for (auto&& [node, inputs] : graph.iter()) { node.process(); }
///   EXPECT_EQ(guard.allocationViolations(), 0u);
///   EXPECT_EQ(guard.involuntaryContextSwitches(), 0);
/// });
/// ```
class AudioThreadGuard {
 public:
  // ── Thread-local state control ──────────────────────────────────────────

  /// Arms the guard on the calling thread. Resets the violation counter.
  static void arm();

  /// Disarms the guard on the calling thread.
  static void disarm();

  /// Returns true if the guard is currently armed on this thread.
  static bool isArmed();

  /// Number of allocation/deallocation violations since last arm().
  static size_t allocationViolations();

  // ── Context-switch snapshot ─────────────────────────────────────────────

  struct ContextSwitchSnapshot {
    int64_t voluntary = 0;
    int64_t involuntary = 0;
  };

  /// Takes a snapshot of context-switch counters.
  static ContextSwitchSnapshot contextSwitches();

  // ── Internal — called by operator new/delete overrides ──────────────────

  static void recordAllocation();
  static void recordDeallocation();

  // ── RAII Scope ──────────────────────────────────────────────────────────

  /// @brief RAII guard — arms on construction, disarms on destruction.
  ///
  /// Query `allocationViolations()`, `involuntaryContextSwitches()`, etc.
  /// before the scope ends to inspect the window.
  class Scope {
   public:
    Scope();
    ~Scope();
    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;

    /// Number of allocation/deallocation violations within this scope.
    [[nodiscard]] size_t allocationViolations() const;

    /// Delta of involuntary context switches within this scope.
    [[nodiscard]] int64_t involuntaryContextSwitches() const;

    /// Delta of voluntary context switches within this scope.
    [[nodiscard]] int64_t voluntaryContextSwitches() const;

    /// True if no allocation or deallocation violations occurred.
    [[nodiscard]] bool clean() const;

   private:
    ContextSwitchSnapshot startSnapshot_;
  };
};

} // namespace audioapi::test
