#include "AudioThreadGuard.h"

#include <cstdint>
#include <cstdlib>
#include <new>
#include <sys/resource.h>

namespace audioapi::test {

// ── Thread-local state ────────────────────────────────────────────────────

static thread_local bool g_armed = false;
static thread_local size_t g_violations = 0;

void AudioThreadGuard::arm() {
  g_armed = true;
  g_violations = 0;
}

void AudioThreadGuard::disarm() {
  g_armed = false;
}

bool AudioThreadGuard::isArmed() {
  return g_armed;
}

size_t AudioThreadGuard::allocationViolations() {
  return g_violations;
}

void AudioThreadGuard::recordAllocation() {
  if (g_armed) ++g_violations;
}

void AudioThreadGuard::recordDeallocation() {
  if (g_armed) ++g_violations;
}

// ── Context switches ──────────────────────────────────────────────────────

AudioThreadGuard::ContextSwitchSnapshot AudioThreadGuard::contextSwitches() {
  struct rusage usage {};
#if defined(__linux__)
  getrusage(RUSAGE_THREAD, &usage); // per-thread (Linux-only)
#else
  getrusage(RUSAGE_SELF, &usage); // process-wide fallback (macOS)
#endif
  return {usage.ru_nvcsw, usage.ru_nivcsw};
}

// ── Scope ─────────────────────────────────────────────────────────────────

AudioThreadGuard::Scope::Scope()
    : startSnapshot_(contextSwitches()) {
  arm();
}

AudioThreadGuard::Scope::~Scope() {
  disarm();
}

size_t AudioThreadGuard::Scope::allocationViolations() const {
  return AudioThreadGuard::allocationViolations();
}

int64_t AudioThreadGuard::Scope::involuntaryContextSwitches() const {
  auto now = contextSwitches();
  return now.involuntary - startSnapshot_.involuntary;
}

int64_t AudioThreadGuard::Scope::voluntaryContextSwitches() const {
  auto now = contextSwitches();
  return now.voluntary - startSnapshot_.voluntary;
}

bool AudioThreadGuard::Scope::clean() const {
  return allocationViolations() == 0;
}

} // namespace audioapi::test

// =========================================================================
// Global operator new / operator delete overrides
// =========================================================================
//
// Replace the default allocator for the entire test binary.
// The hot path (when the guard is not armed) is a single thread_local
// bool check — negligible overhead.
//
// IMPORTANT: These use std::malloc/std::free internally, which do NOT
// recurse back through operator new, so there is no infinite recursion.
//
// Disabled when ASan or TSan is active — they provide their own
// new/delete interceptors and would conflict.

// __has_feature is Clang-only; GCC uses __SANITIZE_ADDRESS__ / __SANITIZE_THREAD__.
#ifndef __has_feature
#define __has_feature(x) 0
#endif

#if !defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) && \
    !__has_feature(address_sanitizer) && !__has_feature(thread_sanitizer)

void *operator new(std::size_t size) {
  audioapi::test::AudioThreadGuard::recordAllocation();
  void *p = std::malloc(size);
  if (!p) throw std::bad_alloc();
  return p;
}

void *operator new[](std::size_t size) {
  audioapi::test::AudioThreadGuard::recordAllocation();
  void *p = std::malloc(size);
  if (!p) throw std::bad_alloc();
  return p;
}

void operator delete(void *p) noexcept {
  audioapi::test::AudioThreadGuard::recordDeallocation();
  std::free(p);
}

void operator delete[](void *p) noexcept {
  audioapi::test::AudioThreadGuard::recordDeallocation();
  std::free(p);
}

void operator delete(void *p, std::size_t) noexcept {
  audioapi::test::AudioThreadGuard::recordDeallocation();
  std::free(p);
}

void operator delete[](void *p, std::size_t) noexcept {
  audioapi::test::AudioThreadGuard::recordDeallocation();
  std::free(p);
}

#endif // sanitizer guard
