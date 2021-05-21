#include <random>   // std include

namespace __internal_random
{
    float uniform_real_dist(float min, float max);
    float normal_dist(float mean, float sigma);
    uint64_t uniform_uint64_dist(uint64_t min, uint64_t max);
};