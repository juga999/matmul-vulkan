#pragma once

#include <cstddef>
#include <vulkan/vulkan.hpp>

#include <vector>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_handles.hpp>

#include "matrix_buffer.hpp"

struct MatMulContext {
    std::string shaderPath;
    uint32_t M, K, N;

    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    vk::PhysicalDeviceMemoryProperties memoryProperties;
    size_t computeQueueFamilyIndex = -1;
    vk::Device device;
    vk::Queue queue;

    vk::DescriptorSetLayout descriptorSetLayout;
    vk::DescriptorPool descriptorPool;
    std::vector<vk::DescriptorSet> descriptorSets;
    vk::DescriptorSet descriptorSet;

    MatrixBuffer matrixBufferA;
    MatrixBuffer matrixBufferB;
    MatrixBuffer matrixBufferC;

    vk::PipelineLayout pipelineLayout;

    std::vector<uint32_t> shaderData;
    vk::ShaderModule shader;

    vk::PipelineCache pipelineCache;
    vk::Pipeline computePipeline;

    vk::CommandPool commandPool;

    std::vector<vk::CommandBuffer> cmdBuffers;

    vk::Fence fence;

    ~MatMulContext() {}

    void initialize(const char* path, size_t MM, size_t KK, size_t NN);

    void loadShader();

    void compute(const uint32_t* matrixA, const uint32_t* matrixB, uint32_t* matrixC);

    void destroy();
};

struct PushConstants {
    uint32_t M;
    uint32_t K;
    uint32_t N;
};
