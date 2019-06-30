#include <lib/stdint.h>
#include <lib/stdio.h>
#include "float32.h"
/*
 * A 32bit fixed point real number class, using int32.
 * The lowest 14 bits are fractional bits, the next 17 bits are the integer bits, the highest bit is sign bit.
 * It can donate form -131,071.999 to 131,071.999, the Accuracy is about 1e-4 ~ 1e-5.
 */
const F = 1 << 14;

/* Transfer int32_t to float32 */
struct float32 float32_init(int32_t x)
{
    struct float32 res = {x * F};
    return res;
}

/* Print a float32 with 3 decimal */
void float32_output(struct float32 a)
{
    a.num = a.num * 1000;
    int num = float32_round(a);
    printf("%d.%03d", num / 1000, num % 1000);
}

/* floor and ceil */
int32_t float32_trunc(struct float32 a)
{
    return a.num >> 14;
}

/* floor and ceil */
int32_t float32_round(struct float32 a)
{
    return (a.num >= 0) ? (a.num + (F >> 1)) / F : (a.num - (F >> 1)) / F;
}

struct float32 float32_add(struct float32 a, struct float32 b)
{
    struct float32 res = {a.num + b.num};
    return res;
}

struct float32 float32_add_int(struct float32 a, int32_t b)
{
    struct float32 res = {a.num + b * F};
    return res;
}

struct float32 float32_sub(struct float32 a, struct float32 b)
{
    struct float32 res = {a.num - b.num};
    return res;
}

struct float32 float32_sub_int(struct float32 a, int32_t b)
{
    struct float32 res = {a.num - b * F};
    return res;
}

struct float32 float32_mul(struct float32 a, struct float32 b)
{
    struct float32 res = {(int64_t)a.num * b.num / F};
    return res;
}

struct float32 float32_mul_int(struct float32 a, int32_t b)
{
    struct float32 res = {a.num * b};
    return res;
}

struct float32 float32_div(struct float32 a, struct float32 b)
{
    struct float32 res = {(int64_t)a.num * F / b.num};
    return res;
}

struct float32 float32_div_int(struct float32 a, int32_t b)
{
    struct float32 res = {a.num / b};
    return res;
}

struct float32 float32_div_int2(int32_t a, int32_t b)
{
    struct float32 res = {a * F / b};
    return res;
}

/* Compare two float32, return true if the first argument less then second argument */
bool float32_less(struct float32 a, struct float32 b)
{
    return a.num < b.num;
}