#ifndef GPU_H
#define GPU_H

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>

extern int setenv(const char *name, const char *value, int overwrite);

/*
 * GPU tasks default configuration:
 * Global work size: 1,048,576 (2^20) work items
 * Local work group size: 64 work items
 */
#define GPU_DEFAULT_LOCAL_WORK_SIZE  64
#define GPU_DEFAULT_GLOBAL_WORK_SIZE 1048576 /* 2^20 */

#define DEFAULT_LOCAL_WORK_SIZE  GPU_DEFAULT_LOCAL_WORK_SIZE
#define DEFAULT_GLOBAL_WORK_SIZE GPU_DEFAULT_GLOBAL_WORK_SIZE

#define GPU_LOCAL_WORK_SIZE  GPU_DEFAULT_LOCAL_WORK_SIZE
#define GPU_GLOBAL_WORK_SIZE GPU_DEFAULT_GLOBAL_WORK_SIZE

#define LOCAL_WORK_SIZE  GPU_DEFAULT_LOCAL_WORK_SIZE
#define GLOBAL_WORK_SIZE GPU_DEFAULT_GLOBAL_WORK_SIZE

#define GPU_WORK_GROUP_SIZE GPU_DEFAULT_LOCAL_WORK_SIZE

typedef struct GpuContext
{
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem buffer_in;
    cl_mem buffer_out;
    uint8_t *host_input;
    uint32_t *host_output;
    size_t global_work_size;
    size_t local_work_size;
    int is_initialized;
} GpuContext;

/* Embedded SHA256 OpenCL kernel source fallback */
static const char * const GPU_SHA256_KERNEL_SRC =
"__constant uint H_0[8] = {\n"
"    0x6a09e667U,\n"
"    0xbb67ae85U,\n"
"    0x3c6ef372U,\n"
"    0xa54ff53aU,\n"
"    0x510e527fU,\n"
"    0x9b05688cU,\n"
"    0x1f83d9abU,\n"
"    0x5be0cd19U\n"
"};\n\n"
"__constant uint K[64] = {\n"
"    0x428a2f98U,\n"
"    0x71374491U,\n"
"    0xb5c0fbcfU,\n"
"    0xe9b5dba5U,\n"
"    0x3956c25bU,\n"
"    0x59f111f1U,\n"
"    0x923f82a4U,\n"
"    0xab1c5ed5U,\n"
"    0xd807aa98U,\n"
"    0x12835b01U,\n"
"    0x243185beU,\n"
"    0x550c7dc3U,\n"
"    0x72be5d74U,\n"
"    0x80deb1feU,\n"
"    0x9bdc06a7U,\n"
"    0xc19bf174U,\n"
"    0xe49b69c1U,\n"
"    0xefbe4786U,\n"
"    0x0fc19dc6U,\n"
"    0x240ca1ccU,\n"
"    0x2de92c6fU,\n"
"    0x4a7484aaU,\n"
"    0x5cb0a9dcU,\n"
"    0x76f988daU,\n"
"    0x983e5152U,\n"
"    0xa831c66dU,\n"
"    0xb00327c8U,\n"
"    0xbf597fc7U,\n"
"    0xc6e00bf3U,\n"
"    0xd5a79147U,\n"
"    0x06ca6351U,\n"
"    0x14292967U,\n"
"    0x27b70a85U,\n"
"    0x2e1b2138U,\n"
"    0x4d2c6dfcU,\n"
"    0x53380d13U,\n"
"    0x650a7354U,\n"
"    0x766a0abbU,\n"
"    0x81c2c92eU,\n"
"    0x92722c85U,\n"
"    0xa2bfe8a1U,\n"
"    0xa81a664bU,\n"
"    0xc24b8b70U,\n"
"    0xc76c51a3U,\n"
"    0xd192e819U,\n"
"    0xd6990624U,\n"
"    0xf40e3585U,\n"
"    0x106aa070U,\n"
"    0x19a4c116U,\n"
"    0x1e376c08U,\n"
"    0x2748774cU,\n"
"    0x34b0bcb5U,\n"
"    0x391c0cb3U,\n"
"    0x4ed8aa4aU,\n"
"    0x5b9cca4fU,\n"
"    0x682e6ff3U,\n"
"    0x748f82eeU,\n"
"    0x78a5636fU,\n"
"    0x84c87814U,\n"
"    0x8cc70208U,\n"
"    0x90befffaU,\n"
"    0xa4506cebU,\n"
"    0xbef9a3f7U,\n"
"    0xc67178f2U\n"
"};\n\n"
"inline uint load_be32(__global const uchar *p, int offset)\n"
"{\n"
"    return ((uint)p[offset]     << 24) |\n"
"           ((uint)p[offset + 1] << 16) |\n"
"           ((uint)p[offset + 2] <<  8) |\n"
"            (uint)p[offset + 3];\n"
"}\n\n"
"__kernel void sha256_ocl_kernel(__global const uint *message, __global uint *digest)\n"
"{\n"
"    size_t gid = get_global_id(0);\n"
"    __global const uint *msg = message + gid * 8;\n"
"    __global uint *out       = digest + gid * 8;\n\n"
"    uint W[64];\n"
"    uint i;\n\n"
"    W[0] = load_be32((__global const uchar *)msg, 0);\n"
"    W[1] = load_be32((__global const uchar *)msg, 4);\n"
"    W[2] = load_be32((__global const uchar *)msg, 8);\n"
"    W[3] = load_be32((__global const uchar *)msg, 12);\n"
"    W[4] = load_be32((__global const uchar *)msg, 16);\n"
"    W[5] = load_be32((__global const uchar *)msg, 20);\n"
"    W[6] = load_be32((__global const uchar *)msg, 24);\n"
"    W[7] = load_be32((__global const uchar *)msg, 28);\n\n"
"    W[8]  = 0x80000000U;\n"
"    W[9]  = 0U;\n"
"    W[10] = 0U;\n"
"    W[11] = 0U;\n"
"    W[12] = 0U;\n"
"    W[13] = 0U;\n"
"    W[14] = 0U;\n"
"    W[15] = 256U;\n\n"
"    for (i = 16; i < 64; ++i)\n"
"    {\n"
"        uint s0 = rotate(W[i - 15],  (uint)(32 - 7)) ^ rotate(W[i - 15], (uint)(32 - 18)) ^ (W[i - 15] >>  3);\n"
"        uint s1 = rotate(W[i -  2], (uint)(32 - 17)) ^ rotate(W[i -  2], (uint)(32 - 19)) ^ (W[i -  2] >> 10);\n"
"        W[i] = W[i - 16] + s0 + W[i - 7] + s1;\n"
"    }\n\n"
"    uint a = H_0[0];\n"
"    uint b = H_0[1];\n"
"    uint c = H_0[2];\n"
"    uint d = H_0[3];\n"
"    uint e = H_0[4];\n"
"    uint f = H_0[5];\n"
"    uint g = H_0[6];\n"
"    uint h = H_0[7];\n\n"
"    for (i = 0; i < 64; ++i)\n"
"    {\n"
"        uint S1 = rotate(e, (uint)(32 - 6)) ^ rotate(e, (uint)(32 - 11)) ^ rotate(e, (uint)(32 - 25));\n"
"        uint ch = (e & f) ^ (~e & g);\n"
"        uint temporary1 = h + S1 + ch + K[i] + W[i];\n"
"        uint S0 = rotate(a, (uint)(32 - 2)) ^ rotate(a, (uint)(32 - 13)) ^ rotate(a, (uint)(32 - 22));\n"
"        uint major = (a & b) ^ (a & c) ^ (b & c);\n"
"        uint temporary2 = S0 + major;\n\n"
"        h = g;\n"
"        g = f;\n"
"        f = e;\n"
"        e = d + temporary1;\n"
"        d = c;\n"
"        c = b;\n"
"        b = a;\n"
"        a = temporary1 + temporary2;\n"
"    }\n\n"
"    out[0] = H_0[0] + a;\n"
"    out[1] = H_0[1] + b;\n"
"    out[2] = H_0[2] + c;\n"
"    out[3] = H_0[3] + d;\n"
"    out[4] = H_0[4] + e;\n"
"    out[5] = H_0[5] + f;\n"
"    out[6] = H_0[6] + g;\n"
"    out[7] = H_0[7] + h;\n"
"}\n";

static GpuContext g_gpu_ctx = {
    .global_work_size = GPU_DEFAULT_GLOBAL_WORK_SIZE,
    .local_work_size  = GPU_DEFAULT_LOCAL_WORK_SIZE,
    .is_initialized   = 0
};

static inline GpuContext *gpu_get_context(void)
{
    return &g_gpu_ctx;
}

static inline char *gpu_read_kernel_file(const char *path, size_t *out_size)
{
    if (!path)
    {
        return NULL;
    }

    FILE *f = fopen(path, "rb");
    if (!f)
    {
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0)
    {
        fclose(f);
        return NULL;
    }

    long sz = ftell(f);
    if (sz < 0)
    {
        fclose(f);
        return NULL;
    }

    if (fseek(f, 0, SEEK_SET) != 0)
    {
        fclose(f);
        return NULL;
    }

    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf)
    {
        fclose(f);
        return NULL;
    }

    size_t read_bytes = fread(buf, 1, (size_t)sz, f);
    buf[read_bytes] = '\0';
    fclose(f);

    if (out_size)
    {
        *out_size = read_bytes;
    }

    return buf;
}

static inline void cleanup_gpu(GpuContext *ctx)
{
    if (!ctx)
    {
        ctx = &g_gpu_ctx;
    }

    if (ctx->host_input)
    {
        free(ctx->host_input);
        ctx->host_input = NULL;
    }

    if (ctx->host_output)
    {
        free(ctx->host_output);
        ctx->host_output = NULL;
    }

    if (ctx->buffer_out)
    {
        clReleaseMemObject(ctx->buffer_out);
        ctx->buffer_out = NULL;
    }

    if (ctx->buffer_in)
    {
        clReleaseMemObject(ctx->buffer_in);
        ctx->buffer_in = NULL;
    }

    if (ctx->kernel)
    {
        clReleaseKernel(ctx->kernel);
        ctx->kernel = NULL;
    }

    if (ctx->program)
    {
        clReleaseProgram(ctx->program);
        ctx->program = NULL;
    }

    if (ctx->queue)
    {
        clReleaseCommandQueue(ctx->queue);
        ctx->queue = NULL;
    }

    if (ctx->context)
    {
        clReleaseContext(ctx->context);
        ctx->context = NULL;
    }

    ctx->is_initialized = 0;
}

static inline cl_int init_gpu(GpuContext *ctx)
{
    if (!ctx)
    {
        ctx = &g_gpu_ctx;
    }

    if (ctx->is_initialized)
    {
        return CL_SUCCESS;
    }

    /* Provide rusticl fallback on systems without proprietary drivers */
    if (!getenv("RUSTICL_ENABLE"))
    {
        setenv("RUSTICL_ENABLE", "llvmpipe", 0);
    }

    cl_uint num_platforms = 0;
    cl_int err = clGetPlatformIDs(0, NULL, &num_platforms);
    if (err != CL_SUCCESS || num_platforms == 0)
    {
        return (err != CL_SUCCESS) ? err : CL_DEVICE_NOT_FOUND;
    }

    cl_platform_id *platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id) * num_platforms);
    if (!platforms)
    {
        return CL_OUT_OF_HOST_MEMORY;
    }
    clGetPlatformIDs(num_platforms, platforms, NULL);

    cl_platform_id selected_platform = NULL;
    cl_device_id selected_device     = NULL;

    /* Prefer GPU device first across all platforms */
    for (cl_uint p = 0; p < num_platforms; ++p)
    {
        cl_uint num_devices = 0;
        if (clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_GPU, 1, &selected_device, &num_devices) == CL_SUCCESS && num_devices > 0)
        {
            selected_platform = platforms[p];
            break;
        }
    }

    /* Fall back to any device type if GPU not found */
    if (!selected_device)
    {
        for (cl_uint p = 0; p < num_platforms; ++p)
        {
            cl_uint num_devices = 0;
            if (clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_ALL, 1, &selected_device, &num_devices) == CL_SUCCESS && num_devices > 0)
            {
                selected_platform = platforms[p];
                break;
            }
        }
    }
    free(platforms);

    if (!selected_device)
    {
        return CL_DEVICE_NOT_FOUND;
    }

    ctx->platform = selected_platform;
    ctx->device   = selected_device;

    ctx->context = clCreateContext(NULL, 1, &ctx->device, NULL, NULL, &err);
    if (err != CL_SUCCESS)
    {
        return err;
    }

    ctx->queue = clCreateCommandQueueWithProperties(ctx->context, ctx->device, NULL, &err);
    if (err != CL_SUCCESS)
    {
        clReleaseContext(ctx->context);
        ctx->context = NULL;
        return err;
    }

    /* Attempt loading kernel source from disk, fallback to embedded string */
    size_t src_size = 0;
    char *file_src = gpu_read_kernel_file("opencl/sha256.cl", &src_size);
    const char *kernel_code = file_src ? file_src : GPU_SHA256_KERNEL_SRC;

    ctx->program = clCreateProgramWithSource(ctx->context, 1, &kernel_code, NULL, &err);
    if (err != CL_SUCCESS)
    {
        if (file_src)
        {
            free(file_src);
        }
        cleanup_gpu(ctx);
        return err;
    }

    err = clBuildProgram(ctx->program, 1, &ctx->device, NULL, NULL, NULL);
    if (file_src)
    {
        free(file_src);
    }
    if (err != CL_SUCCESS)
    {
        cleanup_gpu(ctx);
        return err;
    }

    /* Create SHA-256 kernel */
    ctx->kernel = clCreateKernel(ctx->program, "sha256_ocl_kernel", &err);
    if (err != CL_SUCCESS)
    {
        ctx->kernel = clCreateKernel(ctx->program, "sha256_native_kernel", &err);
        if (err != CL_SUCCESS)
        {
            cleanup_gpu(ctx);
            return err;
        }
    }

    if (ctx->global_work_size == 0)
    {
        ctx->global_work_size = GPU_DEFAULT_GLOBAL_WORK_SIZE;
    }
    if (ctx->local_work_size == 0)
    {
        ctx->local_work_size = GPU_DEFAULT_LOCAL_WORK_SIZE;
    }

    size_t buffer_size = ctx->global_work_size * 32;

    ctx->buffer_in = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY, buffer_size, NULL, &err);
    if (err != CL_SUCCESS)
    {
        cleanup_gpu(ctx);
        return err;
    }

    ctx->buffer_out = clCreateBuffer(ctx->context, CL_MEM_WRITE_ONLY, buffer_size, NULL, &err);
    if (err != CL_SUCCESS)
    {
        cleanup_gpu(ctx);
        return err;
    }

    ctx->host_input  = (uint8_t *)malloc(buffer_size);
    ctx->host_output = (uint32_t *)malloc(buffer_size);
    if (!ctx->host_input || !ctx->host_output)
    {
        cleanup_gpu(ctx);
        return CL_OUT_OF_HOST_MEMORY;
    }
    memset(ctx->host_input, 0, buffer_size);
    memset(ctx->host_output, 0, buffer_size);

    err = clSetKernelArg(ctx->kernel, 0, sizeof(cl_mem), &ctx->buffer_in);
    if (err != CL_SUCCESS)
    {
        cleanup_gpu(ctx);
        return err;
    }

    err = clSetKernelArg(ctx->kernel, 1, sizeof(cl_mem), &ctx->buffer_out);
    if (err != CL_SUCCESS)
    {
        cleanup_gpu(ctx);
        return err;
    }

    ctx->is_initialized = 1;
    return CL_SUCCESS;
}

static inline uint8_t *gpu_get_input_buffer(void)
{
    if (!g_gpu_ctx.is_initialized)
    {
        if (init_gpu(&g_gpu_ctx) != CL_SUCCESS)
        {
            return NULL;
        }
    }
    return g_gpu_ctx.host_input;
}

static inline uint32_t *gpu_get_output_digests(void)
{
    return g_gpu_ctx.host_output;
}

/*
 * Launch a batch of distinct candidates on the GPU.
 * Each GPU work item evaluates a distinct candidate in parallel.
 */
static inline cl_int launch_gpu_batch_ex(
    GpuContext *ctx,
    const void *candidates,
    size_t num_candidates,
    size_t candidate_len,
    size_t local_work_size
)
{
    (void)candidate_len;

    if (!ctx)
    {
        ctx = &g_gpu_ctx;
    }

    if (num_candidates == 0)
    {
        return CL_SUCCESS;
    }

    if (!ctx->is_initialized)
    {
        cl_int init_err = init_gpu(ctx);
        if (init_err != CL_SUCCESS)
        {
            return init_err;
        }
    }

    size_t lws = (local_work_size > 0) ? local_work_size : ctx->local_work_size;
    if (lws == 0)
    {
        lws = GPU_DEFAULT_LOCAL_WORK_SIZE;
    }

    /* Calculate global work size rounded to work group size */
    size_t gws;
    if (num_candidates >= ctx->global_work_size)
    {
        gws = ctx->global_work_size;
    }
    else
    {
        gws = ((num_candidates + lws - 1) / lws) * lws;
    }

    /* If candidates buffer is external, copy into host_input */
    if (candidates && candidates != ctx->host_input)
    {
        size_t copy_bytes = num_candidates * 32;
        if (copy_bytes > ctx->global_work_size * 32)
        {
            copy_bytes = ctx->global_work_size * 32;
        }
        memcpy(ctx->host_input, candidates, copy_bytes);
        /* Zero out any remaining padding slots up to gws */
        if (gws > num_candidates)
        {
            memset(ctx->host_input + num_candidates * 32, 0, (gws - num_candidates) * 32);
        }
    }
    else if (gws > num_candidates)
    {
        memset(ctx->host_input + num_candidates * 32, 0, (gws - num_candidates) * 32);
    }

    size_t write_bytes = gws * 32;
    cl_int err = clEnqueueWriteBuffer(
        ctx->queue,
        ctx->buffer_in,
        CL_FALSE,
        0,
        write_bytes,
        ctx->host_input,
        0,
        NULL,
        NULL
    );
    if (err != CL_SUCCESS)
    {
        return err;
    }

    err = clEnqueueNDRangeKernel(
        ctx->queue,
        ctx->kernel,
        1,
        NULL,
        &gws,
        &lws,
        0,
        NULL,
        NULL
    );

    /* Fallback if device does not support requested work group size */
    if (err == CL_INVALID_WORK_GROUP_SIZE)
    {
        err = clEnqueueNDRangeKernel(
            ctx->queue,
            ctx->kernel,
            1,
            NULL,
            &gws,
            NULL,
            0,
            NULL,
            NULL
        );
    }

    if (err != CL_SUCCESS)
    {
        return err;
    }

    err = clFinish(ctx->queue);
    if (err != CL_SUCCESS)
    {
        return err;
    }

    err = clEnqueueReadBuffer(
        ctx->queue,
        ctx->buffer_out,
        CL_TRUE,
        0,
        write_bytes,
        ctx->host_output,
        0,
        NULL,
        NULL
    );

    return err;
}

static inline cl_int launch_gpu_batch(
    const void *candidates,
    size_t num_candidates,
    size_t candidate_len
)
{
    return launch_gpu_batch_ex(
        &g_gpu_ctx,
        candidates,
        num_candidates,
        candidate_len,
        GPU_DEFAULT_LOCAL_WORK_SIZE
    );
}

/*
 * Base inline function launching GPU kernel for a single candidate.
 */
static inline cl_int (launch_gpu_kernel)(const char *candidate, size_t length)
{
    if (!g_gpu_ctx.is_initialized)
    {
        cl_int init_err = init_gpu(&g_gpu_ctx);
        if (init_err != CL_SUCCESS)
        {
            return init_err;
        }
    }

    memset(g_gpu_ctx.host_input, 0, 32);
    if (candidate && length > 0)
    {
        size_t copy_len = (length > 32) ? 32 : length;
        memcpy(g_gpu_ctx.host_input, candidate, copy_len);
    }

    return launch_gpu_batch_ex(
        &g_gpu_ctx,
        g_gpu_ctx.host_input,
        1,
        length,
        GPU_DEFAULT_LOCAL_WORK_SIZE
    );
}

static inline cl_int (gpu_launch_kernel)(const char *candidate, size_t length)
{
    return (launch_gpu_kernel)(candidate, length);
}

static inline cl_int (launch_kernel)(const char *candidate, size_t length)
{
    return (launch_gpu_kernel)(candidate, length);
}

static inline cl_int launch_gpu_kernel_1(const char *candidate)
{
    return (launch_gpu_kernel)(candidate, candidate ? strlen(candidate) : 0);
}

static inline cl_int launch_gpu_kernel_2(const char *candidate, size_t length)
{
    return (launch_gpu_kernel)(candidate, length);
}

#define _GPU_LAUNCH_SELECT(_1, _2, NAME, ...) NAME
#define launch_gpu_kernel(...) _GPU_LAUNCH_SELECT(__VA_ARGS__, launch_gpu_kernel_2, launch_gpu_kernel_1)(__VA_ARGS__)
#define gpu_launch_kernel(...) _GPU_LAUNCH_SELECT(__VA_ARGS__, launch_gpu_kernel_2, launch_gpu_kernel_1)(__VA_ARGS__)
#define launch_kernel(...)     _GPU_LAUNCH_SELECT(__VA_ARGS__, launch_gpu_kernel_2, launch_gpu_kernel_1)(__VA_ARGS__)

#endif /* GPU_H */
