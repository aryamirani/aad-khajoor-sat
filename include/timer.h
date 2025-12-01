/**
 * @file timer.h
 * @brief High-resolution timer utility for performance benchmarking
 * @authors Arya Mirani, Ujjwal
 * @date December 2025
 */

#ifndef TIMER_H
#define TIMER_H

#include <chrono>

/**
 * @class Timer
 * @brief Provides high-resolution timing functionality for performance measurement
 * 
 * This class uses std::chrono::high_resolution_clock to measure elapsed time
 * with microsecond precision. Useful for benchmarking algorithm performance.
 * 
 * Example usage:
 * @code
 *   Timer t;
 *   // ... perform computation ...
 *   double seconds = t.elapsed_s();
 *   std::cout << "Elapsed: " << seconds << "s" << std::endl;
 * @endcode
 */
class Timer {
public:
    /**
     * @brief Constructs a Timer and starts timing immediately
     * 
     * The timer begins counting from the moment of construction.
     * Call elapsed_s() to get the time since construction.
     */
    Timer();

    /**
     * @brief Resets the timer to the current time
     * 
     * Use this to restart timing without creating a new Timer object.
     * After calling reset(), elapsed_s() will measure time from this point.
     */
    void reset();

    /**
     * @brief Returns elapsed time in seconds since construction or last reset
     * 
     * @return double Elapsed time in seconds with high precision
     * 
     * Time Complexity: O(1)
     */
    double elapsed_s() const;

private:
    /// Time point when timer was started or last reset
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
};

#endif // TIMER_H
