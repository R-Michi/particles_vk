#include "random.h"

std::random_device rd;
std::seed_seq seed{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
std::mt19937 rand_engine(seed);

float __internal_random::uniform_real_dist(float min, float max)
{
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rand_engine);
}

float __internal_random::normal_dist(float mean, float sigma)
{
    std::normal_distribution<float> dist(mean, sigma);
    return dist(rand_engine);
}

uint64_t __internal_random::uniform_uint64_dist(uint64_t min, uint64_t max)
{
    std::uniform_int_distribution<uint64_t> dist(min, max);
    return dist(rand_engine);
}
