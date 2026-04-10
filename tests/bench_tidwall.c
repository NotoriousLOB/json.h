// Benchmark for tidwall/json.c original implementation
// This is compiled separately to avoid symbol conflicts

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// Include tidwall/json.c original implementation
#include "../third_party/json.h"

double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

char *readfile(const char *path, long *len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, 2);
    size_t size = ftell(f);
    rewind(f);
    char *mem = (char *)malloc(size+1);
    if (!mem) {
        fclose(f);
        return NULL;
    }
    if (fread(mem, size, 1, f) != 1) {
        free(mem);
        fclose(f);
        return NULL;
    }
    mem[size] = '\0';
    fclose(f);
    if (len) *len = size;
    return mem;
}

char *readtestfile(const char *name, long *len) {
    char path[256];
    snprintf(path, sizeof(path), "testfiles/%s", name);
    return readfile(path, len);
}

char *commaize(unsigned int n) {
    static char s1[64], s2[64];
    snprintf(s1, sizeof(s1), "%d", n);
    int i = strlen(s1)-1; 
    int j = 0, k = 0;
    s2[0] = '\0';
    while (i >= 0) {
        if (k%3 == 0 && k != 0) {
            memmove(s2+1, s2, strlen(s2)+1);
            s2[0] = ',';
        }
        memmove(s2+1, s2, strlen(s2)+1);
        s2[0] = s1[i];
        i--;
        k++;
    }
    return s2;
}

typedef struct {
    double ops_per_sec;
    double ns_per_op;
    double mb_per_sec;
} bench_result_t;

bench_result_t bench_tidwall_validation(const char *json, size_t len, double duration) {
    for (int w = 0; w < 100; w++) {
        json_validn(json, len);
    }
    
    int count = 0;
    double start = now();
    while (now() - start < duration) {
        for (int i = 0; i < 10; i++) {
            json_validn(json, len);
            count++;
        }
    }
    double elapsed = now() - start;
    
    bench_result_t r;
    r.ops_per_sec = count / elapsed;
    r.ns_per_op = (elapsed / count) * 1e9;
    r.mb_per_sec = ((double)len * count) / elapsed / 1024 / 1024;
    return r;
}

bench_result_t bench_tidwall_parsing(const char *json, size_t len, double duration) {
    for (int w = 0; w < 100; w++) {
        struct json j = json_parse(json);
        (void)j;
    }
    
    int count = 0;
    double start = now();
    while (now() - start < duration) {
        for (int i = 0; i < 10; i++) {
            struct json j = json_parse(json);
            (void)j;
            count++;
        }
    }
    double elapsed = now() - start;
    
    bench_result_t r;
    r.ops_per_sec = count / elapsed;
    r.ns_per_op = (elapsed / count) * 1e9;
    r.mb_per_sec = ((double)len * count) / elapsed / 1024 / 1024;
    return r;
}

bench_result_t bench_tidwall_path(const char *json, size_t len, const char *path, double duration) {
    for (int w = 0; w < 100; w++) {
        struct json j = json_getn(json, len, path);
        (void)j;
    }
    
    int count = 0;
    double start = now();
    while (now() - start < duration) {
        for (int i = 0; i < 10; i++) {
            struct json j = json_getn(json, len, path);
            (void)j;
            count++;
        }
    }
    double elapsed = now() - start;
    
    bench_result_t r;
    r.ops_per_sec = count / elapsed;
    r.ns_per_op = (elapsed / count) * 1e9;
    r.mb_per_sec = ((double)len * count) / elapsed / 1024 / 1024;
    return r;
}

void print_results(const char *name, size_t len, bench_result_t *tidwall) {
    printf("\n=== %s (%zu bytes) ===\n", name, len);
    printf("%-20s %12s %12s %12s\n", "Implementation", "ops/sec", "ns/op", "MB/s");
    printf("%-20s %12s %12.0f %12.1f\n", 
           "tidwall/json.c", 
           commaize((int)tidwall->ops_per_sec), 
           tidwall->ns_per_op, 
           tidwall->mb_per_sec);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    
    printf("=================================================================\n");
    printf("JSON Parser Benchmark: tidwall/json.c (original)\n");
    printf("=================================================================\n");
    printf("\nThis benchmark measures the original tidwall/json.c performance.\n");
    printf("For comparison with json.h, compare outputs from both binaries.\n");
    
    double duration = 0.5;
    
    printf("\n\n");
    printf("#############################################################\n");
    printf("# VALIDATION BENCHMARKS                                     #\n");
    printf("#############################################################\n");
    
    const char *files[] = {
        "twitter.json",
        "canada.json", 
        "citm_catalog.json",
        "github_events.json",
        NULL
    };
    
    for (int i = 0; files[i]; i++) {
        long len = 0;
        char *json = readtestfile(files[i], &len);
        if (!json) {
            printf("Failed to load %s\n", files[i]);
            continue;
        }
        
        bench_result_t r = bench_tidwall_validation(json, len, duration);
        print_results(files[i], len, &r);
        free(json);
    }
    
    printf("\n\n");
    printf("#############################################################\n");
    printf("# PARSING BENCHMARKS                                        #\n");
    printf("#############################################################\n");
    
    for (int i = 0; files[i]; i++) {
        long len = 0;
        char *json = readtestfile(files[i], &len);
        if (!json) {
            printf("Failed to load %s\n", files[i]);
            continue;
        }
        
        bench_result_t r = bench_tidwall_parsing(json, len, duration);
        print_results(files[i], len, &r);
        free(json);
    }
    
    printf("\n\n");
    printf("#############################################################\n");
    printf("# PATH ACCESS BENCHMARKS                                    #\n");
    printf("#############################################################\n");
    
    struct {
        const char *file;
        const char *path;
    } path_tests[] = {
        {"twitter.json", "statuses.50.id"},
        {"canada.json", "features.0.geometry.coordinates"},
        {NULL, NULL}
    };
    
    for (int i = 0; path_tests[i].file; i++) {
        long len = 0;
        char *json = readtestfile(path_tests[i].file, &len);
        if (!json) {
            printf("Failed to load %s\n", path_tests[i].file);
            continue;
        }
        
        char title[256];
        snprintf(title, sizeof(title), "%s (path: %s)", 
                 path_tests[i].file, path_tests[i].path);
        
        bench_result_t r = bench_tidwall_path(json, len, path_tests[i].path, duration);
        print_results(title, len, &r);
        free(json);
    }
    
    printf("\n\n");
    printf("=================================================================\n");
    printf("Benchmark complete!\n");
    printf("=================================================================\n");
    
    return 0;
}
