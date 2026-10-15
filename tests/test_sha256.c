#define _POSIX_C_SOURCE 200809L
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    const char *name;
    uint8_t input[32];
    uint32_t expected[8];
} TestCase;

static void format_hash(const uint32_t h[8], char out[65]) {
    for (int i = 0; i < 8; i++) {
        snprintf(out + i * 8, 9, "%08x", h[i]);
    }
    out[64] = '\0';
}

static char *read_file(const char *path, size_t *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror(path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) {
        fclose(f);
        return NULL;
    }

    char *buf = (char *)malloc(sz + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t read_bytes = fread(buf, 1, sz, f);
    buf[read_bytes] = '\0';
    fclose(f);
    if (out_size) *out_size = read_bytes;
    return buf;
}

int main(int argc, char *argv[]) {
    // Provide fallback to llvmpipe if Mesa rusticl has no devices configured by default
    setenv("RUSTICL_ENABLE", "llvmpipe", 0);

    const char *kernel_path = (argc > 1) ? argv[1] : "opencl/sha256.cl";

    TestCase tests[] = {
        {
            .name = "32 zero bytes (0x00 * 32)",
            .input = {0},
            .expected = {0x66687aad, 0xf862bd77, 0x6c8fc18b, 0x8e9f8e20,
                         0x08971485, 0x6ee233b3, 0x902a591d, 0x0d5f2925}
        },
        {
            .name = "ASCII string 'abcdefghijklmnopqrstuvwxyz123456'",
            .input = {0},
            .expected = {0xf6d527e6, 0xd0186548, 0x1134f297, 0x88be2afe,
                         0x7fc3c702, 0xe1a55d7c, 0xeafac5f3, 0x5199e8dc}
        },
        {
            .name = "ASCII string '12345678901234567890123456789012'",
            .input = {0},
            .expected = {0xe1b85b27, 0xd6bcb058, 0x46c18e6a, 0x48f118e8,
                         0x9f0c0587, 0x140de9fb, 0x3359f837, 0x0d0dba08}
        },
        {
            .name = "32 bytes of 0xff",
            .input = {0},
            .expected = {0xaf961376, 0x0f72635f, 0xbdb44a5a, 0x0a63c39f,
                         0x12af30f9, 0x50a6ee5c, 0x971be188, 0xe89c4051}
        },
        {
            .name = "Sequential bytes 0x00..0x1f",
            .input = {0},
            .expected = {0x630dcd29, 0x66c43366, 0x91125448, 0xbbb25b4f,
                         0xf412a49c, 0x732db2c8, 0xabc1b858, 0x1bd710dd}
        }
    };

    // Populate input data
    memcpy(tests[1].input, "abcdefghijklmnopqrstuvwxyz123456", 32);
    memcpy(tests[2].input, "12345678901234567890123456789012", 32);
    memset(tests[3].input, 0xff, 32);
    for (int b = 0; b < 32; b++) {
        tests[4].input[b] = (uint8_t)b;
    }

    const size_t num_tests = sizeof(tests) / sizeof(tests[0]);

    // Discover OpenCL platforms
    cl_uint num_platforms = 0;
    cl_int err = clGetPlatformIDs(0, NULL, &num_platforms);
    if (err != CL_SUCCESS || num_platforms == 0) {
        fprintf(stderr, "Error: No OpenCL platforms found (err = %d).\n", err);
        return 1;
    }

    cl_platform_id *platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id) * num_platforms);
    clGetPlatformIDs(num_platforms, platforms, NULL);

    cl_platform_id selected_platform = NULL;
    cl_device_id selected_device = NULL;

    // Prefer GPU device first
    for (cl_uint p = 0; p < num_platforms; p++) {
        cl_uint num_devices = 0;
        if (clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_GPU, 1, &selected_device, &num_devices) == CL_SUCCESS && num_devices > 0) {
            selected_platform = platforms[p];
            break;
        }
    }

    // Fall back to any device type if GPU not found
    if (!selected_device) {
        for (cl_uint p = 0; p < num_platforms; p++) {
            cl_uint num_devices = 0;
            if (clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_ALL, 1, &selected_device, &num_devices) == CL_SUCCESS && num_devices > 0) {
                selected_platform = platforms[p];
                break;
            }
        }
    }
    free(platforms);

    if (!selected_device) {
        fprintf(stderr, "Error: No OpenCL device available.\n");
        return 1;
    }

    char plat_name[128] = {0};
    char dev_name[128] = {0};
    clGetPlatformInfo(selected_platform, CL_PLATFORM_NAME, sizeof(plat_name), plat_name, NULL);
    clGetDeviceInfo(selected_device, CL_DEVICE_NAME, sizeof(dev_name), dev_name, NULL);
    printf("OpenCL Platform: %s\n", plat_name);
    printf("OpenCL Device:   %s\n", dev_name);
    printf("Kernel source:   %s\n\n", kernel_path);

    // Read kernel source
    size_t src_size = 0;
    char *kernel_src = read_file(kernel_path, &src_size);
    if (!kernel_src) {
        return 1;
    }

    // Create Context & Command Queue
    cl_context ctx = clCreateContext(NULL, 1, &selected_device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: clCreateContext failed (%d)\n", err);
        free(kernel_src);
        return 1;
    }

    cl_command_queue queue = clCreateCommandQueueWithProperties(ctx, selected_device, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: clCreateCommandQueueWithProperties failed (%d)\n", err);
        clReleaseContext(ctx);
        free(kernel_src);
        return 1;
    }

    // Build Program
    cl_program prog = clCreateProgramWithSource(ctx, 1, (const char **)&kernel_src, NULL, &err);
    free(kernel_src);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: clCreateProgramWithSource failed (%d)\n", err);
        clReleaseCommandQueue(queue);
        clReleaseContext(ctx);
        return 1;
    }

    err = clBuildProgram(prog, 1, &selected_device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: clBuildProgram failed (%d)\n", err);
        size_t log_size = 0;
        clGetProgramBuildInfo(prog, selected_device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = (char *)malloc(log_size + 1);
        if (log) {
            clGetProgramBuildInfo(prog, selected_device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
            log[log_size] = '\0';
            fprintf(stderr, "Build Log:\n%s\n", log);
            free(log);
        }
        clReleaseProgram(prog);
        clReleaseCommandQueue(queue);
        clReleaseContext(ctx);
        return 1;
    }

    // Create Kernel
    cl_kernel kernel = clCreateKernel(prog, "sha256_ocl_kernel", &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: clCreateKernel('sha256_ocl_kernel') failed (%d)\n", err);
        clReleaseProgram(prog);
        clReleaseCommandQueue(queue);
        clReleaseContext(ctx);
        return 1;
    }

    // Prepare input and output buffers (each message is 32 bytes; each digest is 32 bytes = 8 uints)
    uint8_t *host_input = (uint8_t *)malloc(num_tests * 32);
    uint32_t *host_output = (uint32_t *)calloc(num_tests * 8, sizeof(uint32_t));

    for (size_t i = 0; i < num_tests; i++) {
        memcpy(host_input + i * 32, tests[i].input, 32);
    }

    cl_mem buf_in = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                   num_tests * 32, host_input, &err);
    cl_mem buf_out = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY,
                                    num_tests * 8 * sizeof(uint32_t), NULL, &err);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_in);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_out);

    // Enqueue kernel with 1 work-item per test vector
    size_t global_work_size = num_tests;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: clEnqueueNDRangeKernel failed (%d)\n", err);
    }
    clFinish(queue);

    // Read back results
    clEnqueueReadBuffer(queue, buf_out, CL_TRUE, 0,
                        num_tests * 8 * sizeof(uint32_t), host_output, 0, NULL, NULL);

    // Check results
    size_t passed = 0;
    for (size_t i = 0; i < num_tests; i++) {
        const uint32_t *actual = host_output + i * 8;
        const uint32_t *expected = tests[i].expected;

        int match = (memcmp(actual, expected, 8 * sizeof(uint32_t)) == 0);
        if (match) {
            passed++;
            printf("[\033[32mPASS\033[0m] Test %zu: %s\n", i + 1, tests[i].name);
        } else {
            printf("[\033[31mFAIL\033[0m] Test %zu: %s\n", i + 1, tests[i].name);
            char exp_hex[65], act_hex[65];
            format_hash(expected, exp_hex);
            format_hash(actual, act_hex);
            printf("       Expected: %s\n", exp_hex);
            printf("       Actual:   %s\n", act_hex);
        }
    }

    printf("\nResult: %zu / %zu tests passed.\n", passed, num_tests);

    // Cleanup
    free(host_input);
    free(host_output);
    clReleaseMemObject(buf_in);
    clReleaseMemObject(buf_out);
    clReleaseKernel(kernel);
    clReleaseProgram(prog);
    clReleaseCommandQueue(queue);
    clReleaseContext(ctx);

    return (passed == num_tests) ? 0 : 1;
}
