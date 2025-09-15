#ifndef MATH_H
#define MATH_H

// === MyKernel Math Library ===

#define PI 3.14159265358979323846
#define PI_2 1.57079632679489661923  // PI/2
#define PI_4 0.78539816339744830962  // PI/4

// Basic math functions
float sqrt(float x);
float pow(float base, float exp);
float abs(float x);
float fabs(float x);
float floor(float x);
float ceil(float x);

// Trigonometric functions  
float sin(float x);
float cos(float x);
float tan(float x);
float asin(float x);
float acos(float x);
float atan(float x);
float atan2(float y, float x);

// Logarithmic functions
float log(float x);
float log10(float x);
float exp(float x);

// Utility functions
float fmod(float x, float y);
float lerp(float a, float b, float t);
float clamp(float value, float min, float max);
float smoothstep(float edge0, float edge1, float x);

// Vector math (2D)
typedef struct {
    float x, y;
} vec2_t;

vec2_t vec2_add(vec2_t a, vec2_t b);
vec2_t vec2_sub(vec2_t a, vec2_t b);
vec2_t vec2_mul(vec2_t v, float scalar);
float vec2_dot(vec2_t a, vec2_t b);
float vec2_length(vec2_t v);
vec2_t vec2_normalize(vec2_t v);

// Random number generation
void srand(unsigned int seed);
int rand(void);
float randf(void); // 0.0 to 1.0
float rand_range(float min, float max);

#endif