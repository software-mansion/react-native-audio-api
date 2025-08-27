#pragma once

/// Benchmarking utilities
#include <chrono>
#include <unordered_map>

#ifdef ANDROID
#include <android/log.h>
#endif

namespace audioapi::benchmarks {

    /// @brief Gets the execution time of a function
    /// @tparam Func The type of the function to benchmark
    /// @param func The function to benchmark
    /// @return The duration of the function execution in nanoseconds
    /// @note This function is safe to use across threads
    template <typename Func>
    double getExecutionTime(Func&& func) {
        auto start = std::chrono::high_resolution_clock::now();
        std::forward<Func>(func)();
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    }

    /// @brief Logs the average execution time of a function
    /// @tparam Func The type of the function to benchmark
    /// @param msg The message to log
    /// @param func The function to benchmark
    /// @return The duration of the function execution in nanoseconds
    /// @note This function should not be used in production
    /// @note This function is not thread-safe and should be used preferably once in a codebase when you need to measure performance
    template <typename Func>
    double logAvgExecutionTime(const std::string& msg, Func&& func) {
        double duration = getExecutionTime(std::forward<Func>(func));

        static std::unordered_map<std::string, double> durationsSum;
        static std::unordered_map<std::string, int> durationsCount;

        // Ensure initialization for first time
        if (durationsSum.find(msg) == durationsSum.end()) {
            durationsSum[msg] = 0.0;
            durationsCount[msg] = 0;
        }

        durationsSum[msg] += duration;
        durationsCount[msg]++;

        long long avgDuration = static_cast<long long>(durationsSum[msg] / durationsCount[msg]);

        #ifdef ANDROID
            __android_log_print(ANDROID_LOG_INFO, "AudioAPI", "%s: %lld ns", msg.c_str(), avgDuration);
        #else
            printf("%s: %lld ns\n", msg.c_str(), avgDuration);
        #endif
        return duration;
    }
}