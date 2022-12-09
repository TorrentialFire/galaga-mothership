#ifndef _QND_GLOBAL_H_
#define _QND_GLOBAL_H_

#include <atomic>

typedef struct global_t {
    size_t score;
    size_t stage;
    size_t credits;
    size_t lives;
    size_t shot_count;
    size_t hit_count;
    float accuracy;
} global_t;

extern std::atomic<global_t> globals;

#endif