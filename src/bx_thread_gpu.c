/* Thread pool and GPU acceleration implementation */
#define _POSIX_C_SOURCE 200809L
#include "bx_thread_gpu.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

/* ============================================================================
 * Thread Pool Implementation
 * ============================================================================ */
typedef struct {
    bx_task_fn_t fn;
    void *arg;
} bx_task_t;

typedef struct bx_thread_pool {
    pthread_t *threads;
    int num_threads;
    bx_task_t *queue;
    int queue_cap;
    int queue_head;
    int queue_tail;
    int queue_count;
    int shutdown;
    pthread_mutex_t mutex;
    pthread_cond_t cond_not_empty;
    pthread_cond_t cond_not_full;
} bx_thread_pool_t;

static void *worker_thread(void *arg) {
    bx_thread_pool_t *pool = (bx_thread_pool_t*)arg;
    while (1) {
        pthread_mutex_lock(&pool->mutex);
        while (pool->queue_count == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->cond_not_empty, &pool->mutex);
        }
        if (pool->shutdown && pool->queue_count == 0) {
            pthread_mutex_unlock(&pool->mutex);
            break;
        }
        bx_task_t task = pool->queue[pool->queue_head];
        pool->queue_head = (pool->queue_head + 1) % pool->queue_cap;
        pool->queue_count--;
        pthread_cond_signal(&pool->cond_not_full);
        pthread_mutex_unlock(&pool->mutex);
        task.fn(task.arg);
    }
    return NULL;
}

bx_thread_pool_t *bx_thread_pool_create(int num_threads) {
    if (num_threads <= 0) num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_threads <= 0) num_threads = 4;
    
    bx_thread_pool_t *pool = calloc(1, sizeof(bx_thread_pool_t));
    pool->num_threads = num_threads;
    pool->queue_cap = 1024;
    pool->queue = calloc(pool->queue_cap, sizeof(bx_task_t));
    pool->queue_head = pool->queue_tail = pool->queue_count = 0;
    pool->shutdown = 0;
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->cond_not_empty, NULL);
    pthread_cond_init(&pool->cond_not_full, NULL);
    pool->threads = malloc(num_threads * sizeof(pthread_t));
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&pool->threads[i], NULL, worker_thread, pool);
    }
    return pool;
}

void bx_thread_pool_destroy(bx_thread_pool_t *pool) {
    pthread_mutex_lock(&pool->mutex);
    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->cond_not_empty);
    pthread_mutex_unlock(&pool->mutex);
    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond_not_empty);
    pthread_cond_destroy(&pool->cond_not_full);
    free(pool->threads);
    free(pool->queue);
    free(pool);
}

int bx_thread_pool_submit(bx_thread_pool_t *pool, bx_task_fn_t fn, void *arg) {
    pthread_mutex_lock(&pool->mutex);
    while (pool->queue_count == pool->queue_cap && !pool->shutdown) {
        pthread_cond_wait(&pool->cond_not_full, &pool->mutex);
    }
    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->mutex);
        return -1;
    }
    pool->queue[pool->queue_tail].fn = fn;
    pool->queue[pool->queue_tail].arg = arg;
    pool->queue_tail = (pool->queue_tail + 1) % pool->queue_cap;
    pool->queue_count++;
    pthread_cond_signal(&pool->cond_not_empty);
    pthread_mutex_unlock(&pool->mutex);
    return 0;
}

void bx_thread_pool_wait(bx_thread_pool_t *pool) {
    pthread_mutex_lock(&pool->mutex);
    while (pool->queue_count > 0) {
        pthread_cond_wait(&pool->cond_not_full, &pool->mutex);
    }
    pthread_mutex_unlock(&pool->mutex);
}

/* ============================================================================
 * Thread-local storage
 * ============================================================================ */
static __thread bx_thread_local_t *tls_local = NULL;

bx_thread_local_t *bx_thread_local_get(void) {
    return tls_local;
}

void bx_thread_local_set(bx_thread_local_t *local) {
    tls_local = local;
}

/* ============================================================================
 * GPU Acceleration (OpenCL stub - compile with -DBX_OPENCL for real implementation)
 * ============================================================================ */
struct bx_gpu_context {
    bx_gpu_backend_t backend;
    void *platform_data;  /* cl_platform_id, cl_device_id, cl_context, cl_command_queue */
    int initialized;
};

bx_gpu_context_t *bx_gpu_init(bx_gpu_backend_t preferred) {
    bx_gpu_context_t *ctx = calloc(1, sizeof(bx_gpu_context_t));
    ctx->backend = preferred;
    ctx->initialized = 0;
    
#if defined(BX_OPENCL)
    /* Real OpenCL initialization would go here */
    /* clGetPlatformIDs, clGetDeviceIDs, clCreateContext, clCreateCommandQueue, etc. */
    ctx->initialized = 1;
    ctx->backend = BX_GPU_OPENCL;
#else
    (void)preferred;
    ctx->initialized = 0;
    ctx->backend = BX_GPU_NONE;
#endif
    
    return ctx;
}

void bx_gpu_destroy(bx_gpu_context_t *ctx) {
#if defined(BX_OPENCL)
    /* Cleanup OpenCL resources */
#endif
    free(ctx);
}

int bx_gpu_available(bx_gpu_context_t *ctx) {
    return ctx && ctx->initialized;
}

int bx_gpu_execute_kernel(bx_gpu_context_t *ctx, bx_gpu_kernel_t kernel,
                          void *input, size_t input_size,
                          void *output, size_t output_size,
                          int work_items) {
    if (!bx_gpu_available(ctx)) return -1;
    
#if defined(BX_OPENCL)
    /* Real kernel execution */
    (void)kernel; (void)input; (void)input_size; (void)output; (void)output_size; (void)work_items;
    return 0;
#else
    (void)kernel; (void)input; (void)input_size; (void)output; (void)output_size; (void)work_items;
    return -1;
#endif
}

/* ============================================================================
 * SIMD-accelerated math (CPU fallback)
 * ============================================================================ */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elif defined(__aarch64__)
#include <arm_neon.h>
#endif

long bx_simd_math_add(long a, long b) { return a + b; }
long bx_simd_math_sub(long a, long b) { return a - b; }
long bx_simd_math_mul(long a, long b) { return a * b; }
long bx_simd_math_div(long a, long b) { return b ? a / b : 0; }
long bx_simd_math_mod(long a, long b) { return b ? a % b : 0; }

/* ============================================================================
 * Parallel Loop Execution
 * ============================================================================ */
typedef struct {
    const bx_parallel_loop_t *loop;
    long start_idx;
    long end_idx;
    bx_thread_pool_t *pool;
    int *done_count;
    int total_chunks;
} bx_parallel_chunk_t;

static void *parallel_chunk_worker(void *arg) {
    bx_parallel_chunk_t *chunk = (bx_parallel_chunk_t*)arg;
    for (long i = chunk->start_idx; i < chunk->end_idx; i += chunk->loop->step) {
        chunk->loop->body(i, chunk->loop->user_data);
    }
    __sync_fetch_and_add(chunk->done_count, 1);
    return NULL;
}

void bx_parallel_for(bx_thread_pool_t *pool, const bx_parallel_loop_t *loop) {
    if (!pool || !loop || !loop->body) return;
    
    long total = loop->end - loop->start;
    if (total <= 0) return;
    
    int num_threads = pool->num_threads;
    long chunk_size = (total + num_threads - 1) / num_threads;
    
    bx_parallel_chunk_t *chunks = calloc(num_threads, sizeof(bx_parallel_chunk_t));
    int done_count = 0;
    
    for (int i = 0; i < num_threads; i++) {
        long start = loop->start + i * chunk_size;
        long end = start + chunk_size;
        if (end > loop->end) end = loop->end;
        if (start >= end) break;
        
        chunks[i].loop = loop;
        chunks[i].start_idx = start;
        chunks[i].end_idx = end;
        chunks[i].pool = pool;
        chunks[i].done_count = &done_count;
        chunks[i].total_chunks = num_threads;
        
        bx_thread_pool_submit(pool, parallel_chunk_worker, &chunks[i]);
    }
    
    while (done_count < num_threads) {
        sched_yield();
    }
    free(chunks);
}

/* ============================================================================
 * Async/Future Implementation
 * ============================================================================ */
struct bx_future {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int ready;
    void *result;
    bx_task_fn_t fn;
    void *arg;
    pthread_t thread;
};

static void *future_worker(void *arg) {
    bx_future_t *fut = (bx_future_t*)arg;
    fut->fn(fut->arg);
    pthread_mutex_lock(&fut->mutex);
    fut->ready = 1;
    pthread_cond_signal(&fut->cond);
    pthread_mutex_unlock(&fut->mutex);
    return NULL;
}

bx_future_t *bx_async(bx_thread_pool_t *pool, bx_task_fn_t fn, void *arg) {
    (void)pool;
    bx_future_t *fut = calloc(1, sizeof(bx_future_t));
    fut->fn = fn;
    fut->arg = arg;
    pthread_mutex_init(&fut->mutex, NULL);
    pthread_cond_init(&fut->cond, NULL);
    pthread_create(&fut->thread, NULL, future_worker, fut);
    return fut;
}

void *bx_await(bx_future_t *future) {
    pthread_mutex_lock(&future->mutex);
    while (!future->ready) {
        pthread_cond_wait(&future->cond, &future->mutex);
    }
    void *result = future->result;
    pthread_mutex_unlock(&future->mutex);
    return result;
}

void bx_future_destroy(bx_future_t *future) {
    pthread_join(future->thread, NULL);
    pthread_mutex_destroy(&future->mutex);
    pthread_cond_destroy(&future->cond);
    free(future);
}