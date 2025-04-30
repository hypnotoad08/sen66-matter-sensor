#pragma once
#include <vector>

class SmaFilter {
public:
    // Constructor: Initialize the filter with a given window size
    explicit SmaFilter(size_t window_size = 5)
        : _window_size(window_size), _current_sum(0), _current_index(0), _buffer(window_size, 0) {}

    // Add a new sample to the filter and return the updated moving average
    float addSample(float new_sample) {
        // Subtract the oldest value from the sum
        _current_sum -= _buffer[_current_index];

        // Replace the oldest value with the new sample
        _buffer[_current_index] = new_sample;

        // Add the new sample to the sum
        _current_sum += new_sample;

        // Move to the next index in the circular buffer
        _current_index = (_current_index + 1) % _window_size;

        // Return the current moving average
        return _current_sum / static_cast<float>(_window_size);
    }

private:
    size_t _window_size;            // Number of samples in the moving average window
    float _current_sum;             // Current sum of the values in the window
    size_t _current_index;          // Index of the oldest value in the circular buffer
    std::vector<float> _buffer;     // Circular buffer to store the samples
};
