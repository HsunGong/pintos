#ifndef FLOAT32_H
#define FLOAT32_H

struct float32
{
    int32_t num;
};


struct float32 float32_init(int32_t);

void float32_output(struct float32);

int32_t float32_trunc(struct float32);
int32_t float32_round(struct float32);

/* Implemented
 * +,-,*,/ between two float32,
 * +,-,*,/ between a float32 and a int,
 * / between two int.
 * */
struct float32 float32_add(struct float32, struct float32);
struct float32 float32_add_int(struct float32, int32_t);
struct float32 float32_sub(struct float32, struct float32);
struct float32 float32_sub_int(struct float32, int32_t);
struct float32 float32_mul(struct float32, struct float32);
struct float32 float32_mul_int(struct float32, int32_t);
struct float32 float32_div(struct float32, struct float32);
struct float32 float32_div_int(struct float32, int32_t);
struct float32 float32_div_int2(int32_t, int32_t);

bool float32_less(struct float32, struct float32);

#endif 
