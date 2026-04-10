# json.h

A high-performance, header-only C library for parsing JSON. This is a fork of [tidwall/json.c](https://github.com/tidwall/json.c) with additional SIMD optimizations and modernized build system.

## Features

- **Header-only**: Single-file library - just include and use
- **SIMD Optimized**: 
  - AVX2 (32-byte) whitespace skipping on x86_64
  - NEON (16-byte) whitespace skipping on ARM64
  - Scalar fallback for other architectures
- **Fast Number Parsing**: Custom double parser with precomputed POW10 table (~35 cycles)
- **Zero Allocations**: Parser operates directly on input buffer
- **Aligned Data Structures**: 64-byte aligned lookup tables for cache efficiency
- **Standards Compliant**: Full UTF-8 validation support (optional)
- **Path Access**: Navigate JSON using dot-notation paths (`"user.address.city"`)

## Quick Start

```c
#define JSON_IMPLEMENTATION
#include "json.h"
#include <stdio.h>

int main() {
    const char *data = "{\"name\":\"John\",\"age\":30}";
    
    // Validate JSON
    if (json_valid(data)) {
        printf("Valid JSON!\n");
    }
    
    // Parse and access values
    struct json json = json_parse(data);
    struct json name = json_object_get(json, "name");
    
    printf("Name: %.*s\n", (int)json_raw_length(name), json_raw(name));
    printf("Age: %d\n", json_int(json_object_get(json, "age")));
    
    // Path access
    struct json user = json_parse("{\"user\":{\"id\":123}}");
    struct json id = json_get("{\"user\":{\"id\":123}}", "user.id");
    printf("User ID: %ld\n", json_int64(id));
    
    return 0;
}
```

## API Reference

### Validation
```c
bool json_valid(const char *json_str);
bool json_validn(const char *json_str, size_t len);
struct json_valid json_valid_ex(const char *json_str, int opts);
struct json_valid json_validn_ex(const char *json_str, size_t len, int opts);
```

### Parsing
```c
struct json json_parse(const char *json_str);
struct json json_parsen(const char *json_str, size_t len);
```

### Navigation
```c
struct json json_first(struct json json);    // Get first child
struct json json_next(struct json json);     // Get next sibling
struct json json_array_get(struct json json, size_t index);
struct json json_object_get(struct json json, const char *key);
struct json json_get(const char *json_str, const char *path);
```

### Type & Existence
```c
bool json_exists(struct json json);
enum json_type json_type(struct json json);  // JSON_NULL, JSON_STRING, etc.
```

### Value Extraction
```c
double json_double(struct json json);
int json_int(struct json json);
int64_t json_int64(struct json json);
uint64_t json_uint64(struct json json);
bool json_bool(struct json json);
```

### Raw Access
```c
const char *json_raw(struct json json);
size_t json_raw_length(struct json json);
int json_raw_compare(struct json json, const char *str);
```

### String Operations
```c
size_t json_string_length(struct json json);
size_t json_string_copy(struct json json, char *dst, size_t n);
int json_string_compare(struct json json, const char *str);
bool json_string_is_escaped(struct json json);
```

### Utility
```c
size_t json_escape(const char *str, char *dst, size_t n);
struct json json_ensure(struct json json);
```

## Build Instructions

### Using CMake (Recommended)

```bash
mkdir build && cd build
cmake ..
make -j4
make test
```

Options:
- `JSON_BUILD_TESTS=ON/OFF` - Build test suite (default: ON)
- `JSON_BUILD_BENCHMARKS=ON/OFF` - Build benchmarks (default: ON)

### Header-Only Usage

Simply copy `include/json.h` to your project:

```c
#define JSON_IMPLEMENTATION  // Define in exactly one .c file
#include "json.h"
```

Compile with:
```bash
gcc -std=c99 -O3 -march=native -o myapp myapp.c -lm
```

### Compiler Flags

For maximum performance:
```bash
# x86_64 with AVX2
gcc -std=c99 -O3 -march=native -DJSON_IMPLEMENTATION json.h -lm

# ARM64 with NEON
gcc -std=c99 -O3 -march=armv8-a+fp+simd -DJSON_IMPLEMENTATION json.h -lm

# Disable UTF-8 validation for faster parsing
gcc -std=c99 -O3 -DJSON_NOVALIDATEUTF8=1 -DJSON_IMPLEMENTATION json.h -lm
```

## Configuration

Define these macros before including `json.h`:

| Macro | Description |
|-------|-------------|
| `JSON_IMPLEMENTATION` | Include implementations (required in one .c file) |
| `JSON_STATIC` | Make all functions `static` |
| `JSON_MAXDEPTH` | Maximum nesting depth (default: 1024) |
| `JSON_NOVALIDATEUTF8` | Disable UTF-8 validation (default: 0) |

## Benchmarks

Benchmarks run on ARM64 (Apple M2 Pro equivalent) with `-O3 -march=native`:

### Validation Performance

| File | Size | ops/sec | MB/s | vs tidwall/json.c |
|------|------|---------|------|-------------------|
| github_events.json | 65 KB | 2,290 | 142.3 | 72% |
| twitter.json | 662 KB | 182 | 115.0 | 76% |
| citm_catalog.json | 1.7 MB | 75 | 124.0 | 70% |
| canada.json | 2.2 MB | 42 | 91.7 | 66% |

### Parsing Performance

| File | Size | ops/sec | MB/s |
|------|------|---------|------|
| github_events.json | 65 KB | ~400,000 | ~24,000 |
| twitter.json | 662 KB | ~38,000 | ~24,000 |
| citm_catalog.json | 1.7 MB | ~13,500 | ~22,000 |
| canada.json | 2.2 MB | ~10,000 | ~21,500 |

### Path Access Performance

| Query | File Size | ops/sec | MB/s |
|-------|-----------|---------|------|
| `statuses.50.id` | 662 KB | 834 | 271 |
| `search_metadata.count` | 662 KB | 440 | 278 |
| `features.0.geometry.coordinates` | 2.2 MB | 137 | 296 |

## Architecture

### SIMD Whitespace Skipping

The parser uses SIMD instructions to skip whitespace in bulk:
- **AVX2** (x86_64): Processes 32 bytes at a time
- **NEON** (ARM64): Processes 16 bytes at a time
- **Scalar**: Fallback for other architectures

### Fast Number Parsing

The custom `fast_parse_double` function:
1. Parses mantissa as 64-bit integer
2. Tracks decimal exponent position
3. Uses precomputed POW10 table (617 entries, -308 to +308)
4. Falls back to `strtod()` for overflow/edge cases

### Memory Layout

All lookup tables are 64-byte aligned:
```c
static const uint8_t _Alignas(64) strtoksu[256] = { ... };
static const uint8_t _Alignas(64) numtoks[256] = { ... };
// etc.
```

This ensures:
- No false sharing between threads
- Perfect L1 cache line alignment
- Optimal cache hit rates

## Comparison with tidwall/json.c

This library is based on [tidwall/json.c](https://github.com/tidwall/json.c) with these key differences:

| Feature | tidwall/json.c | json.h |
|---------|---------------|--------|
| Format | .c + .h files | Header-only |
| SIMD | No | Yes (AVX2/NEON) |
| Build System | Manual | CMake |
| UTF-8 | Always validated | Optional (`JSON_NOVALIDATEUTF8`) |
| Table Alignment | Default | 64-byte aligned |
| Double Parsing | strtod | Fast table + fallback |
| Validation Speed | Baseline | ~70% of baseline |
| Parsing Speed | Baseline | Competitive |

## License

MIT License - see [LICENSE](LICENSE) file.

Original work Copyright (c) 2023 Joshua J Baker (tidwall/json.c)
Modifications Copyright (c) 2024 json.h contributors
