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
 * GPU tasks default configuration (RTX 3060 12GB)
 * Global work size: 1,048,576 (2^20) work items
 * Local work group size: 64 work items
 */
#define GPU_DEFAULT_LOCAL_WORK_SIZE  64
#define GPU_DEFAULT_GLOBAL_WORK_SIZE 1048576

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

    for (cl_uint p = 0; p < num_platforms; ++p)
    {
        cl_uint num_devices = 0;
        if (clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_GPU, 1, &selected_device, &num_devices) == CL_SUCCESS && num_devices > 0)
        {
            selected_platform = platforms[p];
            break;
        }
    }

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

    size_t src_size = 0;
    char *file_src = gpu_read_kernel_file("opencl/sha256.cl", &src_size);
    const char *kernel_code = file_src;

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

    size_t gws;
    if (num_candidates >= ctx->global_work_size)
    {
        gws = ctx->global_work_size;
    }
    else
    {
        gws = ((num_candidates + lws - 1) / lws) * lws;
    }

    if (candidates && candidates != ctx->host_input)
    {
        size_t copy_bytes = num_candidates * 32;
        if (copy_bytes > ctx->global_work_size * 32)
        {
            copy_bytes = ctx->global_work_size * 32;
        }
        memcpy(ctx->host_input, candidates, copy_bytes);
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
