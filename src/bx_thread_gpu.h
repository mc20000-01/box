/* Thread pool and GPU acceleration for BoxedLANG */
#ifndef BX_THREAD_GPU_H
#define BX_THREAD_GPU_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================================
 * Thread Pool
 * ============================================================================ */
typedef struct bx_thread_pool bx_thread_pool_t;

typedef void *(*bx_task_fn_t)(void *arg);

bx_thread_pool_t *bx_thread_pool_create(int num_threads);
void bx_thread_pool_destroy(bx_thread_pool_t *pool);
int bx_thread_pool_submit(bx_thread_pool_t *pool, bx_task_fn_t fn, void *arg);
void bx_thread_pool_wait(bx_thread_pool_t *pool);

/* Thread-local storage for per-thread boxes */
typedef struct {
    void *boxes;  /* Thread-local box storage */
    int thread_id;
} bx_thread_local_t;

bx_thread_local_t *bx_thread_local_get(void);
void bx_thread_local_set(bx_thread_local_t *local);

/* ============================================================================
 * GPU Acceleration (OpenCL)
 * ============================================================================ */
typedef struct bx_gpu_context bx_gpu_context_t;

typedef enum {
    BX_GPU_NONE = 0,
    BX_GPU_OPENCL = 1,
    BX_GPU_CUDA = 2,
    BX_GPU_METAL = 3,
    BX_GPU_VULKAN = 4,
} bx_gpu_backend_t;

bx_gpu_context_t *bx_gpu_init(bx_gpu_backend_t preferred);
void bx_gpu_destroy(bx_gpu_context_t *ctx);
int bx_gpu_available(bx_gpu_context_t *ctx);

/* GPU Kernels */
typedef enum {
    BX_GPU_KERNEL_MATH_ADD = 0,
    BX_GPU_KERNEL_MATH_MUL = 1,
    BX_GPU_KERNEL_MATH_MOD = 2,
    BX_GPU_KERNEL_PARALLEL_LOOP = 3,
    BX_GPU_KERNEL_GFX_DRAW = 4,
    BX_GPU_KERNEL_VECTOR_ADD = 5,
    BX_GPU_KERNEL_VECTOR_MUL = 6,
    BX_GPU_KERNEL_MATRIX_MUL = 7,
} bx_gpu_kernel_t;

int bx_gpu_execute_kernel(bx_gpu_context_t *ctx, bx_gpu_kernel_t kernel,
                          void *input, size_t input_size,
                          void *output, size_t output_size,
                          int work_items);

/* SIMD-accelerated math for CPU fallback */
long bx_simd_math_add(long a, long b);
long bx_simd_math_sub(long a, long b);
long bx_simd_math_mul(long a, long b);
long bx_simd_math_div(long a, long b);
long bx_simd_math_mod(long a, long b);

/* Parallel loop execution */
typedef struct {
    long start;
    long end;
    long step;
    void (*body)(long index, void *user_data);
    void *user_data;
} bx_parallel_loop_t;

void bx_parallel_for(bx_thread_pool_t *pool, const bx_parallel_loop_t *loop);

/* ============================================================================
 * Async Execution Model
 * ============================================================================ */
struct bx_future;
typedef struct bx_future bx_future_t;

bx_future_t *bx_async(bx_thread_pool_t *pool, bx_task_fn_t fn, void *arg);
void *bx_await(bx_future_t *future);
void bx_future_destroy(bx_future_t *future);

#endif /* BX_THREAD_GPU_H */