// === MyKernel Math Library Implementation ===

#include "../include/math.h"

static unsigned int rand_seed = 1;

// === Basic Functions ===

float abs(float x) {
    return x < 0 ? -x : x;
}

float fabs(float x) {
    return x < 0 ? -x : x;
}

float floor(float x) {
    return (float)((int)x - (x < 0 && x != (int)x));
}

float ceil(float x) {
    return (float)((int)x + (x > 0 && x != (int)x));
}

float sqrt(float x) {
    if (x <= 0) return 0;
    
    // Newton-Raphson method
    float guess = x;
    for (int i = 0; i < 10; i++) {
        guess = (guess + x / guess) * 0.5f;
    }
    return guess;
}

float pow(float base, float exp) {
    if (exp == 0) return 1;
    if (base == 0) return 0;
    if (exp == 1) return base;
    
    float result = 1;
    int int_exp = (int)exp;
    
    // Handle integer exponents
    for (int i = 0; i < int_exp; i++) {
        result *= base;
    }
    
    return result;
}

// === Trigonometric Functions ===

float sin(float x) {
    // Normalize to [-PI, PI]
    while (x > PI) x -= 2 * PI;
    while (x < -PI) x += 2 * PI;
    
    // Taylor series approximation
    float result = x;
    float term = x;
    for (int i = 1; i < 8; i++) {
        term *= -x * x / ((2 * i) * (2 * i + 1));
        result += term;
    }
    return result;
}

float cos(float x) {
    // cos(x) = sin(x + PI/2)
    return sin(x + PI_2);
}

float tan(float x) {
    return sin(x) / cos(x);
}

float asin(float x) {
    if (x < -1 || x > 1) return 0; // Invalid input
    
    // Simple approximation
    return x + (x * x * x) / 6.0f + (3 * x * x * x * x * x) / 40.0f;
}

float acos(float x) {
    return PI_2 - asin(x);
}

float atan(float x) {
    // Simple approximation for small values
    if (abs(x) < 1) {
        return x - (x * x * x) / 3.0f + (x * x * x * x * x) / 5.0f;
    } else {
        return PI_2 - atan(1.0f / x);
    }
}

float atan2(float y, float x) {
    if (x > 0) return atan(y / x);
    if (x < 0 && y >= 0) return atan(y / x) + PI;
    if (x < 0 && y < 0) return atan(y / x) - PI;
    if (x == 0 && y > 0) return PI_2;
    if (x == 0 && y < 0) return -PI_2;
    return 0; // x == 0 && y == 0
}

// === Logarithmic Functions ===

float log(float x) {
    if (x <= 0) return 0;
    
    // Simple approximation using series
    float result = 0;
    float term = (x - 1) / (x + 1);
    float power = term;
    
    for (int i = 1; i < 10; i += 2) {
        result += power / i;
        power *= term * term;
    }
    
    return 2 * result;
}

float log10(float x) {
    return log(x) / log(10);
}

float exp(float x) {
    // Taylor series
    float result = 1;
    float term = 1;
    
    for (int i = 1; i < 15; i++) {
        term *= x / i;
        result += term;
    }
    
    return result;
}

// === Utility Functions ===

float fmod(float x, float y) {
    if (y == 0) return 0;
    return x - floor(x / y) * y;
}

float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

float smoothstep(float edge0, float edge1, float x) {
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// === Vector Math ===

vec2_t vec2_add(vec2_t a, vec2_t b) {
    vec2_t result = {a.x + b.x, a.y + b.y};
    return result;
}

vec2_t vec2_sub(vec2_t a, vec2_t b) {
    vec2_t result = {a.x - b.x, a.y - b.y};
    return result;
}

vec2_t vec2_mul(vec2_t v, float scalar) {
    vec2_t result = {v.x * scalar, v.y * scalar};
    return result;
}

float vec2_dot(vec2_t a, vec2_t b) {
    return a.x * b.x + a.y * b.y;
}

float vec2_length(vec2_t v) {
    return sqrt(v.x * v.x + v.y * v.y);
}

vec2_t vec2_normalize(vec2_t v) {
    float len = vec2_length(v);
    if (len > 0) {
        vec2_t result = {v.x / len, v.y / len};
        return result;
    }
    vec2_t zero = {0, 0};
    return zero;
}

// === Random Number Generation ===

void srand(unsigned int seed) {
    rand_seed = seed;
}

int rand(void) {
    rand_seed = rand_seed * 1103515245 + 12345;
    return (rand_seed / 65536) % 32768;
}

float randf(void) {
    return (float)rand() / 32767.0f;
}

float rand_range(float min, float max) {
    return min + randf() * (max - min);
}