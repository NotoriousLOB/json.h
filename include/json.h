#ifndef JSON_H
#define JSON_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <stdlib.h>

#if defined(__AVX2__)
#  include <immintrin.h>
#elif defined(__ARM_NEON)
#  include <arm_neon.h>
#endif

enum json_type { JSON_NULL, JSON_FALSE, JSON_NUMBER, JSON_STRING, JSON_TRUE, JSON_ARRAY, JSON_OBJECT };

struct json { void *priv[4]; };
struct json_valid { bool valid; size_t pos; };

#ifndef JSON_MAXDEPTH
#define JSON_MAXDEPTH 1024
#endif
#ifndef JSON_NOVALIDATEUTF8
#define JSON_NOVALIDATEUTF8 0
#endif

#ifdef JSON_STATIC
#define JSON_EXTERN static
#else
#define JSON_EXTERN extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC API */
JSON_EXTERN bool json_valid(const char *json_str);
JSON_EXTERN bool json_validn(const char *json_str, size_t len);
JSON_EXTERN struct json_valid json_valid_ex(const char *json_str, int opts);
JSON_EXTERN struct json_valid json_validn_ex(const char *json_str, size_t len, int opts);

JSON_EXTERN struct json json_parse(const char *json_str);
JSON_EXTERN struct json json_parsen(const char *json_str, size_t len);

JSON_EXTERN struct json json_first(struct json json);
JSON_EXTERN struct json json_next(struct json json);

JSON_EXTERN bool json_exists(struct json json);
JSON_EXTERN enum json_type json_type(struct json json);

JSON_EXTERN const char *json_raw(struct json json);
JSON_EXTERN size_t json_raw_length(struct json json);
JSON_EXTERN int json_raw_compare(struct json json, const char *str);
JSON_EXTERN int json_raw_comparen(struct json json, const char *str, size_t len);

JSON_EXTERN int json_string_compare(struct json json, const char *str);
JSON_EXTERN int json_string_comparen(struct json json, const char *str, size_t len);
JSON_EXTERN size_t json_string_length(struct json json);
JSON_EXTERN bool json_string_is_escaped(struct json json);
JSON_EXTERN size_t json_string_copy(struct json json, char *str, size_t nbytes);

JSON_EXTERN struct json json_array_get(struct json json, size_t index);
JSON_EXTERN size_t json_array_count(struct json json);

JSON_EXTERN struct json json_object_get(struct json json, const char *key);
JSON_EXTERN struct json json_object_getn(struct json json, const char *key, size_t len);

JSON_EXTERN struct json json_get(const char *json_str, const char *path);
JSON_EXTERN struct json json_getn(const char *json_str, size_t len, const char *path);

JSON_EXTERN double json_double(struct json json);
JSON_EXTERN int json_int(struct json json);
JSON_EXTERN int64_t json_int64(struct json json);
JSON_EXTERN uint64_t json_uint64(struct json json);
JSON_EXTERN bool json_bool(struct json json);

JSON_EXTERN size_t json_escape(const char *str, char *escaped, size_t n);
JSON_EXTERN size_t json_escapen(const char *str, size_t len, char *escaped, size_t n);

JSON_EXTERN struct json json_ensure(struct json json);

#ifdef JSON_IMPLEMENTATION

/* 1. ALIGNED LUTs */
static const uint8_t _Alignas(64) strtoksu[256] = {
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
#if !JSON_NOVALIDATEUTF8
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,
#endif
};

static const uint8_t _Alignas(64) strtoksa[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,
};

static const uint8_t _Alignas(64) numtoks[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,1,0,1,3,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,
    0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

static const uint8_t _Alignas(64) nesttoks[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,2,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,2,0,0,
};

static const uint8_t _Alignas(64) typetoks[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,3,0,0,0,0,0,0,0,0,0,0,2,0,0,2,2,2,2,2,2,2,2,2,2,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,
    0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,6,0,0,0,0,
};

static const uint8_t _Alignas(64) hextoks[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,
    0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

static const uint8_t _Alignas(64) hexchars[16] = "0123456789abcdef";

/* 2. WHITESPACE LUT for fast scalar skipping */
static const uint8_t _Alignas(64) is_ws[256] = {
    0,0,0,0,0,0,0,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

/* 3. FAST HYBRID WHITESPACE SKIP — scalar LUT + SIMD */
static inline int64_t __attribute__((always_inline, hot)) fast_skip_ws(const uint8_t *restrict data, int64_t dlen, int64_t i)
{
    if (i >= dlen) return i;
    
    /* Fast scalar path using LUT - handles compact JSON efficiently */
    int64_t limit = i + 64;
    if (limit > dlen) limit = dlen;
    for (; i < limit; ++i) {
        if (!is_ws[data[i]]) return i;
    }
    
#if defined(__AVX2__)
    while (i + 32 <= dlen) {
        __m256i chunk = _mm256_loadu_si256((const __m256i*)(data + i));
        __m256i m = _mm256_or_si256(
            _mm256_or_si256(_mm256_cmpeq_epi8(chunk, _mm256_set1_epi8(' ')),
                           _mm256_cmpeq_epi8(chunk, _mm256_set1_epi8('\t'))),
            _mm256_or_si256(_mm256_cmpeq_epi8(chunk, _mm256_set1_epi8('\n')),
                           _mm256_cmpeq_epi8(chunk, _mm256_set1_epi8('\r'))));
        uint32_t bits = (uint32_t)_mm256_movemask_epi8(m);
        if (bits != 0xFFFFFFFFu) return i + __builtin_ctz(~bits);
        i += 32;
    }
#elif defined(__ARM_NEON)
    while (i + 16 <= dlen) {
        uint8x16_t chunk = vld1q_u8(data + i);
        uint8x16_t m = vorrq_u8(
            vorrq_u8(vceqq_u8(chunk, vdupq_n_u8(' ')), vceqq_u8(chunk, vdupq_n_u8('\t'))),
            vorrq_u8(vceqq_u8(chunk, vdupq_n_u8('\n')), vceqq_u8(chunk, vdupq_n_u8('\r'))));
        uint8x16_t non_ws = vmvnq_u8(m);
        uint64x2_t nz = vreinterpretq_u64_u8(non_ws);
        if ((vgetq_lane_u64(nz,0) | vgetq_lane_u64(nz,1)) != 0ULL) {
            uint8x8_t low = vget_low_u8(non_ws), high = vget_high_u8(non_ws);
            uint64_t low64 = vget_lane_u64(vreinterpret_u64_u8(low), 0);
            if (low64) return i + (__builtin_ctzll(low64) >> 3);
            uint64_t high64 = vget_lane_u64(vreinterpret_u64_u8(high), 0);
            return i + 8 + (__builtin_ctzll(high64) >> 3);
        }
        i += 16;
    }
#endif
    while (i < dlen && is_ws[data[i]]) ++i;
    return i;
}

/* 3. EXACT POW10 TABLE (numpy-generated) */
static const double _Alignas(64) POW10[620] = {
    9.99999999999999909e-309, 9.99999999999999909e-308, 1.00000000000000003e-306, 9.99999999999999996e-306,
    9.99999999999999971e-305, 9.99999999999999931e-304, 9.99999999999999963e-303, 1.00000000000000007e-301, 1.00000000000000003e-300,
    9.99999999999999992e-300, 9.99999999999999912e-299, 1.00000000000000004e-297, 1.00000000000000001e-296, 1.00000000000000006e-295,
    1.00000000000000002e-294, 1.00000000000000005e-293, 1.00000000000000005e-292, 9.99999999999999962e-292, 1.00000000000000007e-290,
    1.00000000000000001e-289, 1.00000000000000006e-288, 1.00000000000000002e-287, 1.00000000000000005e-286, 1.00000000000000007e-285,
    1.00000000000000004e-284, 9.99999999999999947e-284, 1.00000000000000002e-282, 1.00000000000000002e-281, 9.99999999999999957e-281,
    1.00000000000000006e-279, 9.99999999999999938e-279, 9.99999999999999969e-278, 1.00000000000000009e-276, 9.99999999999999934e-276,
    9.99999999999999966e-275, 1.00000000000000009e-273, 9.99999999999999930e-273, 9.99999999999999963e-272, 1.00000000000000004e-270,
    9.99999999999999958e-270, 9.99999999999999958e-269, 9.99999999999999985e-268, 9.99999999999999985e-267, 9.99999999999999985e-266,
    1.00000000000000001e-264, 1.00000000000000001e-263, 1.00000000000000001e-262, 9.99999999999999984e-262, 9.99999999999999961e-261,
    1.00000000000000007e-259, 9.99999999999999954e-259, 9.99999999999999977e-258, 9.99999999999999977e-257, 1.00000000000000001e-255,
    9.99999999999999912e-255, 1.00000000000000006e-253, 9.99999999999999943e-253, 1.00000000000000002e-251, 1.00000000000000005e-250,
    1.00000000000000005e-249, 9.99999999999999980e-249, 1.00000000000000002e-247, 9.99999999999999956e-247, 9.99999999999999930e-246,
    9.99999999999999930e-245, 9.99999999999999995e-244, 9.99999999999999969e-243, 9.99999999999999969e-242, 9.99999999999999969e-241,
    1.00000000000000008e-239, 9.99999999999999991e-239, 9.99999999999999991e-238, 1.00000000000000005e-236, 9.99999999999999958e-236,
    9.99999999999999958e-235, 9.99999999999999958e-234, 1.00000000000000002e-232, 9.99999999999999989e-232, 1.00000000000000005e-230,
    1.00000000000000007e-229, 1.00000000000000003e-228, 9.99999999999999945e-228, 9.99999999999999921e-227, 9.99999999999999959e-226,
    1.00000000000000002e-224, 9.99999999999999971e-224, 1.00000000000000005e-222, 1.00000000000000002e-221, 9.99999999999999992e-221,
    1.00000000000000003e-219, 1.00000000000000003e-218, 1.00000000000000008e-217, 1.00000000000000004e-216, 1.00000000000000004e-215,
    9.99999999999999913e-215, 9.99999999999999954e-214, 9.99999999999999954e-213, 1.00000000000000009e-211, 1.00000000000000004e-210,
    1.00000000000000004e-209, 1.00000000000000010e-208, 9.99999999999999925e-208, 1.00000000000000003e-206, 1.00000000000000000e-205,
    1.00000000000000000e-204, 1.00000000000000004e-203, 1.00000000000000004e-202, 9.99999999999999946e-202, 9.99999999999999982e-201,
    9.99999999999999982e-200, 9.99999999999999912e-199, 9.99999999999999987e-198, 1.00000000000000005e-196, 1.00000000000000009e-195,
    1.00000000000000002e-194, 1.00000000000000005e-193, 1.00000000000000010e-192, 1.00000000000000002e-191, 1.00000000000000002e-190,
    1.00000000000000007e-189, 9.99999999999999949e-189, 1.00000000000000001e-187, 9.99999999999999911e-187, 9.99999999999999992e-186,
    1.00000000000000006e-184, 1.00000000000000001e-183, 1.00000000000000005e-182, 1.00000000000000005e-181, 1.00000000000000002e-180,
    1.00000000000000002e-179, 9.99999999999999952e-179, 9.99999999999999952e-178, 9.99999999999999996e-177, 9.99999999999999996e-176,
    9.99999999999999996e-175, 1.00000000000000004e-173, 1.00000000000000004e-172, 9.99999999999999983e-172, 9.99999999999999983e-171,
    1.00000000000000002e-169, 1.00000000000000005e-168, 1.00000000000000000e-167, 1.00000000000000004e-166, 1.00000000000000001e-165,
    9.99999999999999962e-165, 9.99999999999999923e-164, 9.99999999999999954e-163, 1.00000000000000003e-161, 9.99999999999999989e-161,
    9.99999999999999989e-160, 1.00000000000000006e-158, 9.99999999999999943e-158, 1.00000000000000004e-156, 1.00000000000000001e-155,
    9.99999999999999973e-155, 1.00000000000000004e-153, 1.00000000000000007e-152, 9.99999999999999938e-152, 1.00000000000000001e-150,
    9.99999999999999979e-150, 9.99999999999999936e-149, 9.99999999999999970e-148, 1.00000000000000003e-146, 9.99999999999999915e-146,
    9.99999999999999950e-145, 9.99999999999999950e-144, 1.00000000000000004e-142, 1.00000000000000004e-141, 9.99999999999999983e-141,
    1.00000000000000003e-139, 1.00000000000000007e-138, 9.99999999999999978e-138, 1.00000000000000000e-136, 1.00000000000000004e-135,
    1.00000000000000004e-134, 1.00000000000000006e-133, 9.99999999999999986e-133, 9.99999999999999986e-132, 1.00000000000000009e-130,
    9.99999999999999926e-130, 1.00000000000000005e-128, 1.00000000000000003e-127, 9.99999999999999946e-127, 1.00000000000000001e-125,
    9.99999999999999933e-125, 1.00000000000000006e-123, 1.00000000000000006e-122, 9.99999999999999979e-122, 9.99999999999999979e-121,
    1.00000000000000001e-119, 9.99999999999999985e-119, 1.00000000000000003e-117, 9.99999999999999994e-117, 1.00000000000000005e-115,
    1.00000000000000005e-114, 9.99999999999999979e-114, 9.99999999999999950e-113, 1.00000000000000009e-111, 1.00000000000000005e-110,
    9.99999999999999992e-110, 1.00000000000000004e-108, 1.00000000000000000e-107, 9.99999999999999941e-107, 9.99999999999999965e-106,
    9.99999999999999927e-105, 9.99999999999999958e-104, 9.99999999999999933e-103, 1.00000000000000005e-101, 1.00000000000000002e-100,
    1.00000000000000002e-99, 9.99999999999999939e-99, 1.00000000000000004e-97, 9.99999999999999906e-97, 9.99999999999999989e-96,
    9.99999999999999956e-95, 9.99999999999999903e-94, 9.99999999999999988e-93, 1.00000000000000002e-91, 9.99999999999999995e-91,
    1.00000000000000004e-89, 9.99999999999999934e-89, 1.00000000000000002e-87, 1.00000000000000008e-86, 9.99999999999999977e-86,
    1.00000000000000003e-84, 1.00000000000000003e-83, 9.99999999999999961e-83, 9.99999999999999961e-82, 9.99999999999999961e-81,
    9.99999999999999999e-80, 9.99999999999999999e-79, 9.99999999999999927e-78, 9.99999999999999927e-77, 9.99999999999999958e-76,
    9.99999999999999958e-75, 9.99999999999999997e-74, 9.99999999999999966e-73, 9.99999999999999915e-72, 9.99999999999999996e-71,
    9.99999999999999963e-70, 1.00000000000000007e-68, 9.99999999999999943e-68, 9.99999999999999976e-67, 9.99999999999999923e-66,
    9.99999999999999965e-65, 1.00000000000000007e-63, 1.00000000000000004e-62, 1.00000000000000004e-61, 9.99999999999999970e-61,
    1.00000000000000003e-59, 1.00000000000000003e-58, 9.99999999999999955e-58, 1.00000000000000004e-56, 9.99999999999999995e-56,
    1.00000000000000003e-54, 1.00000000000000003e-53, 1.00000000000000001e-52, 1.00000000000000001e-51, 1.00000000000000001e-50,
    9.99999999999999936e-50, 9.99999999999999974e-49, 9.99999999999999974e-48, 1.00000000000000002e-46, 9.99999999999999984e-46,
    9.99999999999999953e-45, 1.00000000000000008e-43, 1.00000000000000004e-42, 1.00000000000000001e-41, 9.99999999999999929e-41,
    9.99999999999999929e-40, 9.99999999999999962e-39, 1.00000000000000007e-37, 9.99999999999999941e-37, 1.00000000000000001e-35,
    9.99999999999999928e-35, 1.00000000000000006e-33, 1.00000000000000006e-32, 1.00000000000000008e-31, 1.00000000000000008e-30,
    9.99999999999999943e-30, 9.99999999999999971e-29, 1.00000000000000004e-27, 1.00000000000000004e-26, 1.00000000000000004e-25,
    9.99999999999999924e-25, 9.99999999999999960e-24, 1.00000000000000005e-22, 9.99999999999999908e-22, 9.99999999999999945e-21,
    9.99999999999999975e-20, 1.00000000000000007e-18, 1.00000000000000007e-17, 9.99999999999999979e-17, 1.00000000000000008e-15,
    9.99999999999999999e-15, 1.00000000000000003e-13, 9.99999999999999980e-13, 9.99999999999999939e-12, 1.00000000000000004e-10,
    1.00000000000000006e-09, 1.00000000000000002e-08, 9.99999999999999955e-08, 9.99999999999999955e-07, 1.00000000000000008e-05,
    1.00000000000000005e-04, 1.00000000000000002e-03, 1.00000000000000002e-02, 1.00000000000000006e-01, 1.00000000000000000e+00,
    1.00000000000000000e+01, 1.00000000000000000e+02, 1.00000000000000000e+03, 1.00000000000000000e+04, 1.00000000000000000e+05,
    1.00000000000000000e+06, 1.00000000000000000e+07, 1.00000000000000000e+08, 1.00000000000000000e+09, 1.00000000000000000e+10,
    1.00000000000000000e+11, 1.00000000000000000e+12, 1.00000000000000000e+13, 1.00000000000000000e+14, 1.00000000000000000e+15,
    1.00000000000000000e+16, 1.00000000000000000e+17, 1.00000000000000000e+18, 1.00000000000000000e+19, 1.00000000000000000e+20,
    1.00000000000000000e+21, 1.00000000000000000e+22, 1.00000000000000008e+23, 9.99999999999999983e+23, 1.00000000000000009e+25,
    1.00000000000000005e+26, 1.00000000000000001e+27, 9.99999999999999958e+27, 9.99999999999999914e+28, 1.00000000000000002e+30,
    9.99999999999999964e+30, 1.00000000000000005e+32, 9.99999999999999946e+32, 9.99999999999999946e+33, 9.99999999999999969e+34,
    1.00000000000000004e+36, 9.99999999999999954e+36, 9.99999999999999977e+37, 9.99999999999999940e+38, 1.00000000000000003e+40,
    1.00000000000000001e+41, 1.00000000000000004e+42, 1.00000000000000001e+43, 1.00000000000000009e+44, 9.99999999999999930e+44,
    9.99999999999999993e+45, 1.00000000000000004e+47, 1.00000000000000004e+48, 9.99999999999999946e+48, 1.00000000000000008e+50,
    9.99999999999999993e+50, 9.99999999999999993e+51, 9.99999999999999993e+52, 1.00000000000000008e+54, 1.00000000000000001e+55,
    1.00000000000000009e+56, 1.00000000000000005e+57, 9.99999999999999944e+57, 9.99999999999999972e+58, 9.99999999999999949e+59,
    9.99999999999999949e+60, 1.00000000000000004e+62, 1.00000000000000006e+63, 1.00000000000000002e+64, 9.99999999999999992e+64,
    9.99999999999999945e+65, 9.99999999999999983e+66, 9.99999999999999953e+67, 1.00000000000000007e+69, 1.00000000000000007e+70,
    1.00000000000000004e+71, 9.99999999999999944e+71, 9.99999999999999983e+72, 9.99999999999999952e+73, 9.99999999999999927e+74,
    1.00000000000000005e+76, 9.99999999999999983e+76, 1.00000000000000001e+78, 9.99999999999999967e+78, 1.00000000000000000e+80,
    9.99999999999999921e+80, 9.99999999999999963e+81, 1.00000000000000003e+83, 1.00000000000000006e+84, 1.00000000000000001e+85,
    1.00000000000000001e+86, 9.99999999999999959e+86, 9.99999999999999959e+87, 9.99999999999999995e+88, 9.99999999999999966e+89,
    1.00000000000000008e+91, 1.00000000000000004e+92, 1.00000000000000004e+93, 1.00000000000000002e+94, 1.00000000000000002e+95,
    1.00000000000000005e+96, 1.00000000000000007e+97, 9.99999999999999998e+97, 9.99999999999999967e+98, 1.00000000000000002e+100,
    9.99999999999999977e+100, 9.99999999999999977e+101, 1.00000000000000000e+103, 1.00000000000000000e+104, 9.99999999999999938e+104,
    1.00000000000000009e+106, 9.99999999999999969e+106, 1.00000000000000003e+108, 9.99999999999999982e+108, 1.00000000000000002e+110,
    9.99999999999999957e+110, 9.99999999999999930e+111, 1.00000000000000002e+113, 1.00000000000000002e+114, 1.00000000000000002e+115,
    1.00000000000000002e+116, 1.00000000000000005e+117, 9.99999999999999967e+117, 9.99999999999999944e+118, 9.99999999999999980e+119,
    1.00000000000000004e+121, 1.00000000000000001e+122, 9.99999999999999978e+122, 9.99999999999999948e+123, 9.99999999999999925e+124,
    9.99999999999999925e+125, 9.99999999999999955e+126, 1.00000000000000008e+128, 9.99999999999999998e+128, 1.00000000000000006e+130,
    9.99999999999999912e+130, 9.99999999999999991e+131, 1.00000000000000002e+133, 9.99999999999999921e+133, 9.99999999999999962e+134,
    1.00000000000000006e+136, 1.00000000000000003e+137, 1.00000000000000003e+138, 1.00000000000000003e+139, 1.00000000000000006e+140,
    1.00000000000000002e+141, 1.00000000000000005e+142, 1.00000000000000002e+143, 1.00000000000000002e+144, 9.99999999999999989e+144,
    9.99999999999999934e+145, 9.99999999999999978e+146, 1.00000000000000005e+148, 1.00000000000000005e+149, 9.99999999999999981e+149,
    1.00000000000000002e+151, 1.00000000000000005e+152, 1.00000000000000000e+153, 1.00000000000000004e+154, 1.00000000000000001e+155,
    9.99999999999999983e+155, 9.99999999999999983e+156, 9.99999999999999953e+157, 9.99999999999999928e+158, 1.00000000000000001e+160,
    1.00000000000000004e+161, 9.99999999999999938e+161, 9.99999999999999938e+162, 1.00000000000000000e+164, 9.99999999999999899e+164,
    9.99999999999999940e+165, 1.00000000000000004e+167, 9.99999999999999934e+167, 9.99999999999999934e+168, 1.00000000000000003e+170,
    9.99999999999999954e+170, 1.00000000000000008e+172, 1.00000000000000001e+173, 1.00000000000000007e+174, 9.99999999999999937e+174,
    1.00000000000000001e+176, 1.00000000000000001e+177, 1.00000000000000005e+178, 9.99999999999999980e+178, 1.00000000000000001e+180,
    9.99999999999999917e+180, 1.00000000000000006e+182, 9.99999999999999947e+182, 1.00000000000000002e+184, 9.99999999999999980e+184,
    9.99999999999999980e+185, 9.99999999999999907e+186, 1.00000000000000002e+188, 1.00000000000000002e+189, 1.00000000000000007e+190,
    1.00000000000000007e+191, 1.00000000000000004e+192, 1.00000000000000007e+193, 9.99999999999999945e+193, 9.99999999999999977e+194,
    9.99999999999999951e+195, 9.99999999999999951e+196, 1.00000000000000002e+198, 1.00000000000000010e+199, 9.99999999999999970e+199,
    1.00000000000000004e+201, 9.99999999999999902e+201, 9.99999999999999989e+202, 9.99999999999999989e+203, 1.00000000000000002e+205,
    1.00000000000000004e+206, 1.00000000000000004e+207, 9.99999999999999982e+207, 1.00000000000000007e+209, 1.00000000000000007e+210,
    9.99999999999999956e+210, 9.99999999999999910e+211, 9.99999999999999984e+212, 9.99999999999999954e+213, 9.99999999999999907e+214,
    1.00000000000000002e+216, 9.99999999999999960e+216, 1.00000000000000008e+218, 9.99999999999999965e+218, 9.99999999999999996e+219,
    1.00000000000000005e+221, 1.00000000000000005e+222, 1.00000000000000005e+223, 9.99999999999999970e+223, 9.99999999999999928e+224,
    9.99999999999999961e+225, 1.00000000000000009e+227, 9.99999999999999925e+227, 9.99999999999999992e+228, 1.00000000000000010e+230,
    1.00000000000000006e+231, 1.00000000000000006e+232, 9.99999999999999974e+232, 1.00000000000000002e+234, 1.00000000000000005e+235,
    1.00000000000000005e+236, 9.99999999999999940e+236, 1.00000000000000005e+238, 9.99999999999999991e+238, 1.00000000000000001e+240,
    1.00000000000000005e+241, 1.00000000000000005e+242, 1.00000000000000007e+243, 1.00000000000000007e+244, 1.00000000000000004e+245,
    1.00000000000000007e+246, 9.99999999999999952e+246, 1.00000000000000005e+248, 9.99999999999999921e+248, 9.99999999999999921e+249,
    1.00000000000000005e+251, 1.00000000000000010e+252, 9.99999999999999936e+252, 9.99999999999999936e+253, 9.99999999999999988e+254,
    1.00000000000000003e+256, 1.00000000000000003e+257, 1.00000000000000006e+258, 9.99999999999999929e+258, 1.00000000000000007e+260,
    9.99999999999999929e+260, 1.00000000000000002e+262, 1.00000000000000002e+263, 1.00000000000000004e+264, 1.00000000000000007e+265,
    1.00000000000000003e+266, 9.99999999999999973e+266, 9.99999999999999973e+267, 1.00000000000000005e+269, 1.00000000000000005e+270,
    9.99999999999999953e+270, 1.00000000000000007e+272, 9.99999999999999945e+272, 9.99999999999999921e+273, 9.99999999999999960e+274,
    1.00000000000000005e+276, 1.00000000000000000e+277, 9.99999999999999964e+277, 1.00000000000000006e+279, 1.00000000000000003e+280,
    1.00000000000000003e+281, 1.00000000000000003e+282, 9.99999999999999955e+282, 1.00000000000000008e+284, 9.99999999999999980e+284,
    1.00000000000000003e+286, 1.00000000000000008e+287, 1.00000000000000001e+288, 1.00000000000000006e+289, 1.00000000000000006e+290,
    9.99999999999999958e+290, 1.00000000000000001e+292, 9.99999999999999925e+292, 1.00000000000000007e+294, 9.99999999999999981e+294,
    9.99999999999999981e+295, 1.00000000000000002e+297, 9.99999999999999960e+297, 1.00000000000000005e+299, 1.00000000000000005e+300,
    1.00000000000000005e+301, 1.00000000000000008e+302, 1.00000000000000000e+303, 9.99999999999999939e+303, 9.99999999999999939e+304,
    1.00000000000000002e+306, 9.99999999999999986e+306, 1.00000000000000001e+308,
    0.0, 0.0, 0.0
};

/* 4. FAST DOUBLE PARSER */
static double __attribute__((hot)) fast_parse_double(const uint8_t *restrict s, size_t len, int validate_full)
{
    if (len == 0) return 0.0;
    int64_t mant = 0; int exp = 0; int sign = 1; size_t i = 0;
    if (s[0] == '-') { sign = -1; ++i; } else if (s[0] == '+') ++i;
    while (i < len && s[i] >= '0' && s[i] <= '9') { if (mant > (INT64_MAX-9)/10) goto ovf; mant = mant*10 + (s[i++]-'0'); }
    if (i < len && s[i] == '.') { ++i; while (i < len && s[i] >= '0' && s[i] <= '9') { if (mant > (INT64_MAX-9)/10) goto ovf; mant = mant*10 + (s[i++]-'0'); --exp; } }
    if (i < len && (s[i]=='e'||s[i]=='E')) {
        ++i; int esign = 1; if (i < len && s[i]=='+') ++i; else if (i < len && s[i]=='-') { esign=-1; ++i; }
        int e = 0; while (i < len && s[i]>='0'&&s[i]<='9') { e = e*10 + (s[i++]-'0'); if (e>1000) goto ovf; }
        exp += esign * e;
    }
    if (validate_full && i != len) return 0.0;  // Invalid trailing chars
    double d = (double)mant * sign;
    int idx = exp + 308;
    if (idx >= 0 && idx < 617) d *= POW10[idx];
    else if (exp > 308) d = sign > 0 ? INFINITY : -INFINITY; else d = 0.0;
    return d;
ovf: return sign > 0 ? INFINITY : -INFINITY;
}

/* 5. THE REST (repo code with SIMD everywhere) */
struct vutf8res { int n; uint32_t cp; };
static inline struct vutf8res vutf8(const uint8_t data[], int64_t len) {
    uint32_t cp; int n = 0;
    if (data[0]>>4 == 14) {
        if (len < 3) goto fail;
        if (((data[1]>>6)|(data[2]>>6<<2)) != 10) goto fail;
        cp = ((uint32_t)(data[0]&15)<<12)|((uint32_t)(data[1]&63)<<6)|((uint32_t)(data[2]&63));
        n = 3;
    } else if (data[0]>>3 == 30) {
        if (len < 4) goto fail;
        if (((data[1]>>6)|(data[2]>>6<<2)|(data[3]>>6<<4)) != 42) goto fail;
        cp = ((uint32_t)(data[0]&7)<<18)|((uint32_t)(data[1]&63)<<12)|((uint32_t)(data[2]&63)<<6)|((uint32_t)(data[3]&63));
        n = 4;
    } else if (data[0]>>5 == 6) {
        if (len < 2) goto fail;
        if (data[1]>>6 != 2) goto fail;
        cp = ((uint32_t)(data[0]&31)<<6)|((uint32_t)(data[1]&63));
        n = 2;
    } else goto fail;
    if (cp < 128) goto fail;
    if (cp >= 0x10FFFF) goto fail;
    if (cp >= 0xD800 && cp <= 0xDFFF) goto fail;
    return (struct vutf8res){.n=n,.cp=cp};
fail: return (struct vutf8res){0};
}

static inline int64_t vesc(const uint8_t *json, int64_t jlen, int64_t i) {
    i += 1; if (i == jlen) return -(i+1);
    switch (json[i]) {
    case '"': case '\\': case '/': case 'b': case 'f': case 'n': case 'r': case 't': return i+1;
    case 'u':
        for (int j = 0; j < 4; j++) { i++; if (i == jlen) return -(i+1); if (!((json[i]>='0'&&json[i]<='9')||(json[i]>='a'&&json[i]<='f')||(json[i]>='A'&&json[i]<='F'))) return -(i+1); }
        return i+1;
    }
    return -(i+1);
}

#ifndef ludo
#define ludo
#define ludo1(i,f) f; i++;
#define ludo2(i,f) ludo1(i,f); ludo1(i,f);
#define ludo4(i,f) ludo2(i,f); ludo2(i,f);
#define ludo8(i,f) ludo4(i,f); ludo4(i,f);
#define ludo16(i,f) ludo8(i,f); ludo8(i,f);
#define ludo32(i,f) ludo16(i,f); ludo16(i,f);
#define ludo64(i,f) ludo32(i,f); ludo32(i,f);
#define for1(i,n,f) while(i+1<=(n)) { ludo1(i,f); }
#define for2(i,n,f) while(i+2<=(n)) { ludo2(i,f); } for1(i,n,f);
#define for4(i,n,f) while(i+4<=(n)) { ludo4(i,f); } for1(i,n,f);
#define for8(i,n,f) while(i+8<=(n)) { ludo8(i,f); } for1(i,n,f);
#define for16(i,n,f) while(i+16<=(n)) { ludo16(i,f); } for1(i,n,f);
#define for32(i,n,f) while(i+32<=(n)) { ludo32(i,f); } for1(i,n,f);
#define for64(i,n,f) while(i+64<=(n)) { ludo64(i,f); } for1(i,n,f);
#endif

static int64_t vstring(const uint8_t *json, int64_t jlen, int64_t i) {
    while (1) {
        for8(i, jlen, { if (strtoksu[json[i]]) goto tok; })
        break;
    tok:
        if (json[i] == '"') return i+1;
#if !JSON_NOVALIDATEUTF8
        else if (json[i] > 127) { struct vutf8res res = vutf8(json+i, jlen-i); if (res.n == 0) break; i += res.n; }
#endif
        else if (json[i] == '\\') { if ((i = vesc(json, jlen, i)) < 0) break; }
        else break;
    }
    return -(i+1);
}

static int64_t vnumber(const uint8_t *data, int64_t dlen, int64_t i) {
    i--; if (data[i] == '-') { i++; if (i == dlen) return -(i+1); if (data[i] < '0' || data[i] > '9') return -(i+1); }
    if (data[i] == '0') { i++; } else { for (; i < dlen; i++) { if (data[i] >= '0' && data[i] <= '9') continue; break; } }
    if (i == dlen) return i;
    if (data[i] == '.') {
        i++; if (i == dlen) return -(i+1); if (data[i] < '0' || data[i] > '9') return -(i+1); i++;
        for (; i < dlen; i++) { if (data[i] >= '0' && data[i] <= '9') continue; break; }
    }
    if (i == dlen) return i;
    if (data[i] == 'e' || data[i] == 'E') {
        i++; if (i == dlen) return -(i+1); if (data[i] == '+' || data[i] == '-') i++;
        if (i == dlen) return -(i+1); if (data[i] < '0' || data[i] > '9') return -(i+1); i++;
        for (; i < dlen; i++) { if (data[i] >= '0' && data[i] <= '9') continue; break; }
    }
    return i;
}

static int64_t vnull(const uint8_t *data, int64_t dlen, int64_t i) { return i+3 <= dlen && data[i]=='u' && data[i+1]=='l' && data[i+2]=='l' ? i+3 : -(i+1); }
static int64_t vtrue(const uint8_t *data, int64_t dlen, int64_t i) { return i+3 <= dlen && data[i]=='r' && data[i+1]=='u' && data[i+2]=='e' ? i+3 : -(i+1); }
static int64_t vfalse(const uint8_t *data, int64_t dlen, int64_t i) { return i+4 <= dlen && data[i]=='a' && data[i+1]=='l' && data[i+2]=='s' && data[i+3]=='e' ? i+4 : -(i+1); }

static int64_t vcolon(const uint8_t *json, int64_t len, int64_t i) {
    i = fast_skip_ws(json, len, i);
    return (i >= len || json[i] != ':') ? -(i+1) : i+1;
}

static int64_t vcomma(const uint8_t *json, int64_t len, int64_t i, uint8_t end) {
    i = fast_skip_ws(json, len, i);
    if (i >= len) return -(i+1);
    if (json[i] == ',') return i;
    return (json[i] == end) ? i : -(i+1);
}

static int64_t vany(const uint8_t *data, int64_t dlen, int64_t i, int depth);

static int64_t varray(const uint8_t *data, int64_t dlen, int64_t i, int depth) {
    i = fast_skip_ws(data, dlen, i);
    if (i < dlen && data[i] == ']') return i+1;
    while (i < dlen) {
        if ((i = vany(data, dlen, i, depth+1)) < 0) return i;
        if ((i = vcomma(data, dlen, i, ']')) < 0) return i;
        if (i < dlen && data[i] == ']') return i+1;
        i++; // skip past comma
    }
    return -(i+1);
}

static int64_t vkey(const uint8_t *json, int64_t len, int64_t i) {
    i = fast_skip_ws(json, len, i);
    for16(i, len, { if (strtoksu[json[i]]) goto tok; })
    return -(i+1);
tok: if (json[i] == '"') return i+1; return vstring(json, len, i);
}

static int64_t vobject(const uint8_t *data, int64_t dlen, int64_t i, int depth) {
    i = fast_skip_ws(data, dlen, i);
    if (i < dlen && data[i] == '}') return i+1;
    for (; i < dlen; i++) {
        switch (data[i]) {
        case '"':
        key:
            if ((i = vkey(data, dlen, i+1)) < 0) return i;
            if ((i = vcolon(data, dlen, i)) < 0) return i;
            if ((i = vany(data, dlen, i, depth+1)) < 0) return i;
            if ((i = vcomma(data, dlen, i, '}')) < 0) return i;
            if (i < dlen && data[i] == '}') return i+1;
            // Skip past comma and whitespace, then expect next key
            i = fast_skip_ws(data, dlen, i+1);
            if (i < dlen && data[i] == '"') goto key;
            return -(i+1);
        case ' ': case '\t': case '\n': case '\r':
            i = fast_skip_ws(data, dlen, i+1) - 1;
            continue;
        case '}': return i+1;
        default: return -(i+1);
        }
    }
    return -(i+1);
}

static int64_t vany(const uint8_t *data, int64_t dlen, int64_t i, int depth) {
    if (depth > JSON_MAXDEPTH) return -(i+1);
    i = fast_skip_ws(data, dlen, i);
    if (i >= dlen) return -(i+1);
    switch (data[i]) {
    case '{': return vobject(data, dlen, i+1, depth);
    case '[': return varray(data, dlen, i+1, depth);
    case '"': return vstring(data, dlen, i+1);
    case 't': return vtrue(data, dlen, i+1);
    case 'f': return vfalse(data, dlen, i+1);
    case 'n': return vnull(data, dlen, i+1);
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': return vnumber(data, dlen, i+1);
    }
    return -(i+1);
}

static int64_t vpayload(const uint8_t *data, int64_t dlen, int64_t i) {
    i = fast_skip_ws(data, dlen, i);
    if (i >= dlen) return -(i+1);
    if ((i = vany(data, dlen, i, 1)) < 0) return i;
    i = fast_skip_ws(data, dlen, i);
    return (i == dlen) ? i : -(i+1);
}

JSON_EXTERN struct json_valid json_validn_ex(const char *json_str, size_t len, int opts) {
    (void)opts;
    int64_t ilen = len;
    if (ilen < 0) return (struct json_valid){0};
    int64_t pos = vpayload((uint8_t*)json_str, len, 0);
    if (pos > 0) return (struct json_valid){.valid=true};
    return (struct json_valid){.pos=(-pos)-1};
}
JSON_EXTERN struct json_valid json_valid_ex(const char *json_str, int opts) { return json_validn_ex(json_str, json_str?strlen(json_str):0, opts); }
JSON_EXTERN bool json_validn(const char *json_str, size_t len) { return json_validn_ex(json_str, len, 0).valid; }
JSON_EXTERN bool json_valid(const char *json_str) { return json_validn(json_str, json_str?strlen(json_str):0); }

enum iflags { IESC = 1, IDOT = 2, ISCI = 4, ISIGN = 8 };
#define jmake(info, raw, end, len) ((struct json){.priv={(void*)(uintptr_t)(info),(void*)(uintptr_t)(raw),(void*)(uintptr_t)(end),(void*)(uintptr_t)(len)}})
#define jinfo(json) ((int)(uintptr_t)((json).priv[0]))
#define jraw(json) ((uint8_t*)(uintptr_t)((json).priv[1]))
#define jend(json) ((uint8_t*)(uintptr_t)((json).priv[2]))
#define jlen(json) ((size_t)(uintptr_t)((json).priv[3]))

static inline size_t count_string(uint8_t *raw, uint8_t *end, int *infoout) {
    size_t len = end-raw, i = 1; int info = 0; bool e = false;
    while (1) {
        for8(i, len, { if (strtoksa[raw[i]]) goto tok; e = false; })
        break;
    tok:
        if (raw[i] == '"') { i++; if (!e) break; e = false; continue; }
        if (raw[i] == '\\') { info |= IESC; e = !e; }
        i++;
    }
    *infoout = info; return i;
}

static struct json take_string(uint8_t *raw, uint8_t *end) { int info = 0; size_t i = count_string(raw, end, &info); return jmake(info, raw, end, i); }

static struct json take_number(uint8_t *raw, uint8_t *end) {
    int64_t len = end-raw; int info = raw[0]=='-' ? ISIGN : 0; int64_t i = 1;
    for16(i, len, { if (!numtoks[raw[i]]) goto done; info |= (numtoks[raw[i]]-1); });
done: return jmake(info, raw, end, i);
}

static size_t count_nested(uint8_t *raw, uint8_t *end) {
    size_t len = end-raw, i = 1; int depth = 1, kind = 0;
    if (i >= len) return i;
    while (depth) {
        for16(i, len, { if (nesttoks[raw[i]]) goto tok0; });
        break;
    tok0:
        kind = nesttoks[raw[i]]; i++;
        if (kind-1) { depth += kind-3; }
        else {
            while (1) {
                for16(i, len, { if (raw[i]=='"') goto tok1; });
                break;
            tok1:
                i++;
                if (raw[i-2] == '\\') {
                    size_t j = i-3, e = 1;
                    while (j > 0 && raw[j] == '\\') { e = (e+1)&1; j--; }
                    if (e) continue;
                }
                break;
            }
        }
    }
    return i;
}

static struct json take_literal(uint8_t *raw, uint8_t *end, size_t litlen) { size_t rlen = end-raw; return jmake(0, raw, end, rlen < litlen ? rlen : litlen); }

static struct json peek_any(uint8_t *raw, uint8_t *end) {
    while (raw < end) {
        switch (*raw) {
        case '}': case ']': return (struct json){0};
        case '{': case '[': return jmake(0, raw, end, 0);
        case '"': return take_string(raw, end);
        case 'n': return take_literal(raw, end, 4);
        case 't': return take_literal(raw, end, 4);
        case 'f': return take_literal(raw, end, 5);
        case '-': case '0': case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9': return take_number(raw, end);
        }
        raw++;
    }
    return (struct json){0};
}

JSON_EXTERN struct json json_first(struct json json) {
    uint8_t *raw = jraw(json), *end = jend(json);
    if (end <= raw || (*raw != '{' && *raw != '[')) return (struct json){0};
    return peek_any(raw+1, end);
}

JSON_EXTERN struct json json_next(struct json json) {
    uint8_t *raw = jraw(json), *end = jend(json);
    if (end <= raw) return (struct json){0};
    raw += jlen(json) == 0 ? count_nested(raw, end) : jlen(json);
    return peek_any(raw, end);
}

JSON_EXTERN struct json json_parsen(const char *json_str, size_t len) {
    if (len > 0 && (json_str[0] == '[' || json_str[0] == '{')) return jmake(0, json_str, json_str+len, 0);
    if (len == 0) return (struct json){0};
    return peek_any((uint8_t*)json_str, (uint8_t*)json_str+len);
}
JSON_EXTERN struct json json_parse(const char *json_str) { return json_parsen(json_str, json_str?strlen(json_str):0); }
JSON_EXTERN bool json_exists(struct json json) { return jraw(json) && jend(json); }
JSON_EXTERN const char *json_raw(struct json json) { return (char*)jraw(json); }
JSON_EXTERN size_t json_raw_length(struct json json) { if (jlen(json)) return jlen(json); if (jraw(json) < jend(json)) return count_nested(jraw(json), jend(json)); return 0; }
JSON_EXTERN enum json_type json_type(struct json json) { return jraw(json) < jend(json) ? typetoks[*jraw(json)] : JSON_NULL; }
JSON_EXTERN struct json json_ensure(struct json json) { return jmake(jinfo(json), jraw(json), jend(json), json_raw_length(json)); }

static int strcmpn(const char *a, size_t alen, const char *b, size_t blen) {
    size_t n = alen < blen ? alen : blen;
    int cmp = strncmp(a, b, n);
    if (cmp == 0) cmp = alen < blen ? -1 : alen > blen ? 1 : 0;
    return cmp;
}

static uint32_t decode_hex(const uint8_t *str) { return (((int)hextoks[str[0]])<<12)|(((int)hextoks[str[1]])<<8)|(((int)hextoks[str[2]])<<4)|(((int)hextoks[str[3]])<<0); }
static bool is_surrogate(uint32_t cp) { return cp > 55296 && cp < 57344; }
static uint32_t decode_codepoint(uint32_t cp1, uint32_t cp2) { return cp1>55296 && cp1<56320 && cp2>56320 && cp2<57344 ? ((cp1-55296)<<10)|((cp2-56320)+65536) : 65533; }

static inline int encode_codepoint(uint8_t dst[], uint32_t cp) {
    if (cp < 128) { dst[0] = cp; return 1; }
    else if (cp < 2048) { dst[0] = 192|(cp>>6); dst[1] = 128|(cp&63); return 2; }
    else if (cp > 1114111 || is_surrogate(cp)) cp = 65533;
    if (cp < 65536) { dst[0] = 224|(cp>>12); dst[1] = 128|((cp>>6)&63); dst[2] = 128|(cp&63); return 3; }
    dst[0] = 240|(cp>>18); dst[1] = 128|((cp>>12)&63); dst[2] = 128|((cp>>6)&63); dst[3] = 128|(cp&63); return 4;
}

#define for_each_utf8(jstr, len, f) { \
    size_t nn = (len); int ch = 0; (void)ch; \
    for (size_t ii = 0; ii < nn; ii++) { \
        if ((jstr)[ii] != '\\') { ch = (jstr)[ii]; if (1) f continue; } \
        ii++; if (ii == nn) break; \
        switch ((jstr)[ii]) { \
        case '\\': ch = '\\'; break; case '/': ch = '/'; break; case 'b': ch = '\b'; break; \
        case 'f': ch = '\f'; break; case 'n': ch = '\n'; break; case 'r': ch = '\r'; break; \
        case 't': ch = '\t'; break; case '"': ch = '"'; break; \
        case 'u': \
            if (ii+5 > nn) { nn = 0; continue; } \
            uint32_t cp = decode_hex((jstr)+ii+1); ii += 5; \
            if (is_surrogate(cp)) { if (nn-ii >= 6 && (jstr)[ii]=='\\' && (jstr)[ii+1]=='u') { cp = decode_codepoint(cp, decode_hex((jstr)+ii+2)); ii += 6; } } \
            uint8_t _bytes[4]; int _n = encode_codepoint(_bytes, cp); \
            for (int _j = 0; _j < _n; _j++) { ch = _bytes[_j]; if (1) f } \
            ii--; continue; \
        default: continue; \
        }; \
        if (1) f \
    } \
}

JSON_EXTERN int json_raw_comparen(struct json json, const char *str, size_t len) {
    char *raw = (char*)jraw(json); if (!raw) raw = "";
    size_t rlen = json_raw_length(json);
    return strcmpn(raw, rlen, str, len);
}
JSON_EXTERN int json_raw_compare(struct json json, const char *str) { return json_raw_comparen(json, str, strlen(str)); }

JSON_EXTERN size_t json_string_length(struct json json) {
    size_t len = json_raw_length(json);
    if (json_type(json) != JSON_STRING) return len;
    len = len < 2 ? 0 : len - 2;
    if ((jinfo(json)&IESC) != IESC) return len;
    uint8_t *raw = jraw(json)+1; size_t count = 0;
    for_each_utf8(raw, len, { count++; });
    return count;
}

JSON_EXTERN int json_string_comparen(struct json json, const char *str, size_t slen) {
    if (json_type(json) != JSON_STRING) return json_raw_comparen(json, str, slen);
    uint8_t *raw = jraw(json); size_t rlen = json_raw_length(json);
    raw++; rlen = rlen < 2 ? 0 : rlen - 2;
    if ((jinfo(json)&IESC) != IESC) return strcmpn((char*)raw, rlen, str, slen);
    int cmp = 0; uint8_t *sp = (uint8_t*)(str ? str : "");
    for_each_utf8(raw, rlen, {
        if (!*sp || ch > *sp) { cmp = 1; goto done; }
        else if (ch < *sp) { cmp = -1; goto done; }
        sp++;
    });
done: if (cmp == 0 && *sp) cmp = -1; return cmp;
}
JSON_EXTERN int json_string_compare(struct json json, const char *str) { return json_string_comparen(json, str, str?strlen(str):0); }

JSON_EXTERN size_t json_string_copy(struct json json, char *str, size_t n) {
    size_t len = json_raw_length(json); uint8_t *raw = jraw(json);
    bool isjsonstr = json_type(json) == JSON_STRING, isesc = false;
    if (isjsonstr) { raw++; len = len < 2 ? 0 : len - 2; isesc = (jinfo(json)&IESC) == IESC; }
    if (!isesc) { if (n == 0) return len; n = n-1 < len ? n-1 : len; memcpy(str, raw, n); str[n] = '\0'; return len; }
    size_t count = 0;
    for_each_utf8(raw, len, { if (count < n) str[count] = ch; count++; });
    if (n > count) str[count] = '\0'; else if (n > 0) str[n-1] = '\0';
    return count;
}

JSON_EXTERN size_t json_array_count(struct json json) {
    size_t count = 0;
    if (json_type(json) == JSON_ARRAY) {
        json = json_first(json);
        while (json_exists(json)) { count++; json = json_next(json); }
    }
    return count;
}

JSON_EXTERN struct json json_array_get(struct json json, size_t index) {
    if (json_type(json) == JSON_ARRAY) {
        json = json_first(json);
        while (json_exists(json)) { if (index == 0) return json; json = json_next(json); index--; }
    }
    return (struct json){0};
}

JSON_EXTERN struct json json_object_getn(struct json json, const char *key, size_t len) {
    if (json_type(json) == JSON_OBJECT) {
        json = json_first(json);
        while (json_exists(json)) {
            if (json_string_comparen(json, key, len) == 0) return json_next(json);
            json = json_next(json_next(json));
        }
    }
    return (struct json){0};
}
JSON_EXTERN struct json json_object_get(struct json json, const char *key) { return json_object_getn(json, key, key?strlen(key):0); }

static double stod(const uint8_t *str, size_t len, char *buf) { memcpy(buf, str, len); buf[len] = '\0'; char *ptr; double x = strtod(buf, &ptr); return (size_t)(ptr-buf) == len ? x : 0; }
static double parse_double_big(const uint8_t *str, size_t len) { char buf[512]; if (len >= sizeof(buf)) return 0; return stod(str, len, buf); }
static double parse_double_fallback(const uint8_t *str, size_t len) { char buf[32]; if (len >= sizeof(buf)) return parse_double_big(str, len); return stod(str, len, buf); }

static int64_t parse_int64(const uint8_t *s, size_t len) {
    char buf[21]; double y;
    if (len == 0) return 0;
    if (len < sizeof(buf) && sizeof(long long) == sizeof(int64_t)) {
        memcpy(buf, s, len); buf[len] = '\0'; char *ptr = NULL;
        int64_t x = strtoll(buf, &ptr, 10); if ((size_t)(ptr-buf) == len) return x;
        y = strtod(buf, &ptr); if ((size_t)(ptr-buf) == len) goto clamp;
    }
    y = parse_double_fallback(s, len);
clamp: if (y < (double)INT64_MIN) return INT64_MIN; if (y > (double)INT64_MAX) return INT64_MAX; return y;
}

static uint64_t parse_uint64(const uint8_t *s, size_t len) {
    char buf[21]; double y;
    if (len == 0) return 0;
    if (len < sizeof(buf) && sizeof(long long) == sizeof(uint64_t) && s[0] != '-') {
        memcpy(buf, s, len); buf[len] = '\0'; char *ptr = NULL;
        uint64_t x = strtoull(buf, &ptr, 10); if ((size_t)(ptr-buf) == len) return x;
        y = strtod(buf, &ptr); if ((size_t)(ptr-buf) == len) goto clamp;
    }
    y = parse_double_fallback(s, len);
clamp: if (y < 0) return 0; if (y > (double)UINT64_MAX) return UINT64_MAX; return y;
}

JSON_EXTERN double json_double(struct json json) {
    switch (json_type(json)) {
    case JSON_TRUE: return 1;
    case JSON_STRING: { size_t len = jlen(json); if (len < 3) return 0.0; const uint8_t *s = jraw(json)+1; size_t n = len-2; double d = fast_parse_double(s, n, 1); if (isinf(d)) d = parse_double_fallback(s, n); return d; }
    case JSON_NUMBER: { const uint8_t *s = jraw(json); size_t n = jlen(json); double d = fast_parse_double(s, n, 0); if (isinf(d)) d = parse_double_fallback(s, n); return d; }
    default: return 0.0;
    }
}

JSON_EXTERN int64_t json_int64(struct json json) {
    switch (json_type(json)) {
    case JSON_TRUE: return 1;
    case JSON_STRING: { size_t len = jlen(json); if (len < 2) return 0; return parse_int64(jraw(json)+1, len-2); }
    case JSON_NUMBER: return parse_int64(jraw(json), jlen(json));
    default: return 0;
    }
}
JSON_EXTERN int json_int(struct json json) { int64_t x = json_int64(json); if (x < (int64_t)INT_MIN) return INT_MIN; if (x > (int64_t)INT_MAX) return INT_MAX; return x; }

JSON_EXTERN uint64_t json_uint64(struct json json) {
    switch (json_type(json)) {
    case JSON_TRUE: return 1;
    case JSON_STRING: { size_t len = jlen(json); if (len < 2) return 0; return parse_uint64(jraw(json)+1, len-2); }
    case JSON_NUMBER: return parse_uint64(jraw(json), jlen(json));
    default: return 0;
    }
}

JSON_EXTERN bool json_bool(struct json json) {
    switch (json_type(json)) {
    case JSON_TRUE: return true;
    case JSON_NUMBER: return json_double(json) != 0.0;
    case JSON_STRING: { char *trues[] = {"1","t","T","true","TRUE","True"}; for (size_t i = 0; i < sizeof(trues)/sizeof(char*); i++) if (json_string_compare(json, trues[i]) == 0) return true; return false; }
    default: return false;
    }
}

struct jesc_buf { uint8_t *esc; size_t esclen; size_t count; };
static void jesc_append(struct jesc_buf *buf, uint8_t ch) { if (buf->esclen > 1) { *(buf->esc++) = ch; buf->esclen--; } buf->count++; }
static void jesc_append2(struct jesc_buf *buf, uint8_t c1, uint8_t c2) { jesc_append(buf, c1); jesc_append(buf, c2); }
static void jesc_append_ux(struct jesc_buf *buf, uint8_t c1, uint8_t c2, uint16_t x) {
    jesc_append2(buf, c1, c2); jesc_append2(buf, hexchars[x>>12&0xF], hexchars[x>>8&0xF]);
    jesc_append2(buf, hexchars[x>>4&0xF], hexchars[x>>0&0xF]);
}

JSON_EXTERN size_t json_escapen(const char *str, size_t len, char *esc, size_t n) {
    uint8_t cpbuf[4]; struct jesc_buf buf = {.esc=(uint8_t*)esc,.esclen=n}; jesc_append(&buf, '"');
    for (size_t i = 0; i < len; i++) {
        uint32_t c = (uint8_t)str[i];
        if (c < ' ') {
            switch (c) {
            case '\n': jesc_append2(&buf, '\\', 'n'); break; case '\b': jesc_append2(&buf, '\\', 'b'); break;
            case '\f': jesc_append2(&buf, '\\', 'f'); break; case '\r': jesc_append2(&buf, '\\', 'r'); break;
            case '\t': jesc_append2(&buf, '\\', 't'); break; default: jesc_append_ux(&buf, '\\', 'u', c);
            }
        } else if (c == '>' || c == '<' || c == '&') { jesc_append_ux(&buf, '\\', 'u', c); }
        else if (c == '\\') { jesc_append2(&buf, '\\', '\\'); }
        else if (c == '"') { jesc_append2(&buf, '\\', '"'); }
        else if (c > 127) {
            struct vutf8res res = vutf8((uint8_t*)(str+i), len-i);
            if (res.n == 0) { res.n = 1; res.cp = 0xfffd; }
            int cpn = encode_codepoint(cpbuf, res.cp); for (int j = 0; j < cpn; j++) jesc_append(&buf, cpbuf[j]);
            i = i + res.n - 1;
        } else jesc_append(&buf, str[i]);
    }
    jesc_append(&buf, '"'); if (buf.esclen > 0) { *(buf.esc++) = '\0'; buf.esclen--; }
    return buf.count;
}
JSON_EXTERN size_t json_escape(const char *str, char *esc, size_t n) { return json_escapen(str, str?strlen(str):0, esc, n); }

JSON_EXTERN struct json json_getn(const char *json_str, size_t len, const char *path) {
    if (!path) return (struct json){0};
    struct json json = json_parsen(json_str, len);
    int i = 0; bool end = false; char *p = (char*)path;
    for (; !end && json_exists(json); i++) {
        const char *key = p; while (*p && *p != '.') p++;
        size_t klen = p-key; if (*p == '.') p++; else if (!*p) end = true;
        enum json_type type = json_type(json);
        if (type == JSON_OBJECT) json = json_object_getn(json, key, klen);
        else if (type == JSON_ARRAY) {
            if (klen == 0) { i = 0; break; }
            char *endptr; size_t index = strtol(key, &endptr, 10);
            if (*endptr && *endptr != '.') { i = 0; break; }
            json = json_array_get(json, index);
        } else { i = 0; break; }
    }
    return i == 0 ? (struct json){0} : json;
}
JSON_EXTERN struct json json_get(const char *json_str, const char *path) { return json_getn(json_str, json_str?strlen(json_str):0, path); }
JSON_EXTERN bool json_string_is_escaped(struct json json) { return (jinfo(json)&IESC) == IESC; }

#endif // JSON_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#endif // JSON_H
