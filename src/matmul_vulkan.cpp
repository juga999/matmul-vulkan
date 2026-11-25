#include "matmul_vulkan.h"
#include "matmul_context.hpp"

MATMUL_VULKAN_EXTERN MATMUL_VULKAN_API
MatMulContext* matmul_context_create(const char* path, size_t M, size_t K, size_t N)
{
    MatMulContext* ctx = new MatMulContext();
    ctx->initialize(path, M, K, N);
    return ctx;
}

MATMUL_VULKAN_EXTERN void MATMUL_VULKAN_API
matmul_compute(MatMulContext* ctx, const uint32_t* matrixA, const uint32_t* matrixB, uint32_t* matrixC)
{
    ctx->compute(matrixA, matrixB, matrixC);
}

MATMUL_VULKAN_EXTERN void MATMUL_VULKAN_API
matmul_context_destroy(MatMulContext* ctx)
{
    ctx->destroy();
    delete ctx;
}
