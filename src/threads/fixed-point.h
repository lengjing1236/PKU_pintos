#ifndef FIXED_POINT_H
#define FIXED_POINT_H

#include <stdint.h>

#define FP_SHIFT 14
#define FP_F (1 << FP_SHIFT)
typedef int fixed_point;

static inline fixed_point int_to_fp(int n) {
    return n * FP_F;
}

static inline int fp_to_int_zero(fixed_point x) {
    return x / FP_F;
}

static inline int fp_to_int_nearest(fixed_point x) {
    return x >= 0 ? (x + FP_F / 2) / FP_F : (x - FP_F / 2) / FP_F;
}

static inline fixed_point add_fp(fixed_point x, fixed_point y) {
    return x + y;
}

static inline fixed_point sub_fp(fixed_point x, fixed_point y) {
    return x - y;
}

static inline fixed_point add_fp_int(fixed_point x, int n) {
    return x + n * FP_F;
}

static inline fixed_point sub_fp_int(fixed_point x, int n) {
    return x - n * FP_F;
}

static inline fixed_point mul_fp(fixed_point x, fixed_point y) {
    return ((int64_t) x) * y / FP_F;
}

static inline fixed_point mul_fp_int(fixed_point x, int n) {
    return x * n;
}

static inline fixed_point divide_fp(fixed_point x, fixed_point y) {
    return ((int64_t) x) * FP_F / y;
}

static inline fixed_point divide_fp_int(fixed_point x, int n) {
    return x / n;
}
#endif  /**< fixed-point.h */