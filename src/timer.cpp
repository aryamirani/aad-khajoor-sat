/**
 * @file timer.cpp
 * @brief Implementation of Timer class for performance benchmarking
 * @authors Arya Mirani, Ujjwal
 * @date December 2025
 */

#include "timer.h"

Timer::Timer() : start_time(std::chrono::high_resolution_clock::now()) {}

void Timer::reset() {
    start_time = std::chrono::high_resolution_clock::now();
}

double Timer::elapsed_s() const {
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end_time - start_time;
    return diff.count();
}
