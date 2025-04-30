#pragma once
#include <vector>

class SmaFilter {
public:
    explicit SmaFilter(size_t window_size = 5)
      : _size(window_size), _sum(0), _idx(0), _buf(window_size, 0) {}

    float addSample(float x) {
        // subtract old, add new
        _sum -= _buf[_idx];
        _buf[_idx] = x;
        _sum += x;
        if (++_idx >= _size) _idx = 0;
        return _sum / float(_size);
    }

private:
    size_t         _size;
    float          _sum;
    size_t         _idx;
    std::vector<float> _buf;
};
