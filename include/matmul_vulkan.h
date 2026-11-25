#pragma once

#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
    #define MATMUL_VULKAN_EXTERN extern "C"
#else
    #define MATMUL_VULKAN_EXTERN extern
#endif

#if defined(__GNUC__)
    #define MATMUL_VULKAN_API __attribute__((visibility("default")))
#else
    #define MATMUL_VULKAN_API
#endif


typedef struct MatMulContext MatMulContext;

MATMUL_VULKAN_EXTERN MATMUL_VULKAN_API MatMulContext* matmul_context_create(
    const char* path, size_t M, size_t K, size_t N
);

MATMUL_VULKAN_EXTERN MATMUL_VULKAN_API void matmul_compute(
    MatMulContext* ctx, const uint32_t* matrixA, const uint32_t* matrixB, uint32_t* matrixC
);

MATMUL_VULKAN_EXTERN MATMUL_VULKAN_API void matmul_context_destroy(
    MatMulContext* engine
);
