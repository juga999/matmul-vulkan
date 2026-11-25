#include "matmul_context.hpp"

#include <cstddef>
#include <stdexcept>

#include <iostream>
#include <vector>
#include <fstream>


const uint32_t TILE_SIZE = 16;

void MatMulContext::destroy() {
    device.destroyFence(fence);

    device.destroyCommandPool(commandPool);

    device.destroyPipeline(computePipeline);
    device.destroyPipelineCache(pipelineCache);

    device.destroyShaderModule(shader);

    device.destroyPipelineLayout(pipelineLayout);

    matrixBufferA.destroy(device);
    matrixBufferB.destroy(device);
    matrixBufferC.destroy(device);

    device.destroyDescriptorPool(descriptorPool);
    device.destroyDescriptorSetLayout(descriptorSetLayout);
    device.destroy();
    instance.destroy();

    std::cout << "Destroyed" << std::endl;
}

void MatMulContext::initialize(const char* path, size_t MM, size_t KK, size_t NN)
{
    shaderPath = path;

    M = MM;
    K = KK;
    N = NN;

    vk::ApplicationInfo appInfo(
        "Vulkan Matmul", 1,
        "Vulkan.hpp", 1,
        VK_API_VERSION_1_3
    );

    vk::InstanceCreateInfo instanceCreateInfo(
        vk::InstanceCreateFlags(),
        &appInfo, {}, {});
    instance = vk::createInstance(instanceCreateInfo);

    physicalDevice = instance.enumeratePhysicalDevices().front();

    vk::PhysicalDeviceProperties physicalDeviceProperties;
    physicalDevice.getProperties(&physicalDeviceProperties);
    std::cout
        << "Max invocations: "
        << physicalDeviceProperties.limits.maxComputeWorkGroupInvocations << std::endl
        << "Max X: "
        << physicalDeviceProperties.limits.maxComputeWorkGroupSize[0] << std::endl
        << "Max Y: "
        << physicalDeviceProperties.limits.maxComputeWorkGroupSize[1] << std::endl;

    
    physicalDevice.getMemoryProperties(&memoryProperties);

    // get the QueueFamilyProperties of the first PhysicalDevice
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    // get the first index into queueFamiliyProperties which supports compute
    auto propertyIterator = std::find_if(
        queueFamilyProperties.begin(), queueFamilyProperties.end(),
        [](vk::QueueFamilyProperties const & props) { return props.queueFlags & vk::QueueFlagBits::eCompute; }
    );
    computeQueueFamilyIndex = std::distance(queueFamilyProperties.begin(),propertyIterator);
    if (computeQueueFamilyIndex == queueFamilyProperties.size()) {
        throw new std::runtime_error("Failed to find compute queue");
    }

    // create a Device
    float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo(
        vk::DeviceQueueCreateFlags(),
        static_cast<uint32_t>(computeQueueFamilyIndex),
        1,
        &queuePriority
    );
    device = physicalDevice.createDevice(
        vk::DeviceCreateInfo(vk::DeviceCreateFlags(), deviceQueueCreateInfo)
    );

    queue = device.getQueue(computeQueueFamilyIndex, 0);

    // layout bindings
    vk::DescriptorSetLayoutBinding bindings[3]{};
    for (size_t i = 0; i < 3; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorType = vk::DescriptorType::eStorageBuffer;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = vk::ShaderStageFlagBits::eCompute;
    }

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
        vk::DescriptorSetLayoutCreateFlags(),
        bindings
    );
    descriptorSetLayout = device.createDescriptorSetLayout(
        descriptorSetLayoutCreateInfo
    );

    vk::DescriptorPoolSize descriptorPoolSize(vk::DescriptorType::eStorageBuffer, 3);
    vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo(vk::DescriptorPoolCreateFlags(), 1, descriptorPoolSize);
    descriptorPool = device.createDescriptorPool(descriptorPoolCreateInfo);
    vk::DescriptorSetAllocateInfo descriptorSetAllocInfo(descriptorPool, 1, &descriptorSetLayout);
    descriptorSets = device.allocateDescriptorSets(descriptorSetAllocInfo);

    // Matrix memory allocation
    descriptorSet = descriptorSets.front();

    matrixBufferA.initialize(device, memoryProperties, M * K);
    matrixBufferB.initialize(device, memoryProperties, K * N);
    matrixBufferC.initialize(device, memoryProperties, M * N);

    vk::DescriptorBufferInfo matrixBufferInfoA(matrixBufferA.buffer, 0, matrixBufferA.size);
    vk::DescriptorBufferInfo matrixBufferInfoB(matrixBufferB.buffer, 0, matrixBufferB.size);
    vk::DescriptorBufferInfo matrixBufferInfoC(matrixBufferC.buffer, 0, matrixBufferC.size);

    const std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {
        {descriptorSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &matrixBufferInfoA},
        {descriptorSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &matrixBufferInfoB},
        {descriptorSet, 2, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &matrixBufferInfoC},
    };
    device.updateDescriptorSets(writeDescriptorSets, {});

    // Pipeline
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo(vk::PipelineLayoutCreateFlags(), descriptorSetLayout);

    vk::PushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eCompute;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);
    pipelineLayoutCreateInfo.setPushConstantRanges(pushConstantRange);

    pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);

    loadShader();

    vk::ShaderModuleCreateInfo shaderModuleCreateinfo(
        vk::ShaderModuleCreateFlags(),
        shaderData.size() * sizeof(uint32_t), shaderData.data()
    );
    shader = device.createShaderModule(shaderModuleCreateinfo);

    vk::PipelineShaderStageCreateInfo pipelineShaderCreateInfo(
        vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eCompute,
        shader, "main"
    );
    vk::ComputePipelineCreateInfo computePipelineCreateInfo(
        vk::PipelineCreateFlags(),
        pipelineShaderCreateInfo,
        pipelineLayout
    );
    vk::PipelineCache pipelineCache = device.createPipelineCache(vk::PipelineCacheCreateInfo());
    auto createResult = device.createComputePipeline(pipelineCache, computePipelineCreateInfo);
    if (createResult.result != vk::Result::eSuccess) {
        throw std::runtime_error("Buffer creation failed: " + std::to_string(static_cast<int>(createResult.result)));
    }
    computePipeline = createResult.value;

    // Commands
    vk::CommandPoolCreateInfo commandPoolCreateInfo(vk::CommandPoolCreateFlags(), computeQueueFamilyIndex);
    commandPool = device.createCommandPool(commandPoolCreateInfo);

    cmdBuffers = device.allocateCommandBuffers({
        commandPool, vk::CommandBufferLevel::ePrimary, 1
    });

    fence = device.createFence(vk::FenceCreateInfo());
}

void MatMulContext::loadShader()
{
    std::ifstream file(shaderPath, std::ios::binary | std::ios::ate);

    if (!file) {
        throw std::runtime_error("Cannot open shader");
    }

    size_t size = file.tellg();

    if (size == 0 || size % 4 != 0) {
        throw std::runtime_error("Invalid SPIR-V file");
    }

    shaderData.resize(size / 4);

    file.seekg(0);
    file.read(reinterpret_cast<char*>(shaderData.data()), size);
}

void MatMulContext::compute(const uint32_t* matrixA, const uint32_t* matrixB, uint32_t* matrixC)
{
    matrixBufferA.copy(device, matrixA);
    matrixBufferB.copy(device, matrixB);

    vk::CommandBuffer cmdBuffer = cmdBuffers.front();
    cmdBuffer.reset();

    cmdBuffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    vk::MemoryBarrier inputMemoryBarrier{{}, vk::AccessFlagBits::eShaderRead};
    cmdBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eHost,
        vk::PipelineStageFlagBits::eComputeShader,
        {},
        {inputMemoryBarrier}, {}, {}
    );
    cmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
    cmdBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        pipelineLayout,
        0, {descriptorSet}, {}
    );

    PushConstants constants{
        .M = M,
        .K = K,
        .N = N
    };
    cmdBuffer.pushConstants<PushConstants>(
        pipelineLayout,
        vk::ShaderStageFlagBits::eCompute,
        0, constants
    );

    // Calculate work group counts
    const uint32_t numGroupsX = (N + TILE_SIZE - 1) / TILE_SIZE;
    const uint32_t numGroupsY = (M + TILE_SIZE - 1) / TILE_SIZE;
    cmdBuffer.dispatch(numGroupsX, numGroupsY, 1);

    vk::MemoryBarrier outputMemoryBarrier{vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eHostRead};
    cmdBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eHost,
        {},
        {outputMemoryBarrier}, {}, {}
    );

    cmdBuffer.end();

    vk::SubmitInfo submitInfo(
        0, nullptr, nullptr,
        1,&cmdBuffer
    );
    queue.submit({submitInfo}, fence);

    auto result = device.waitForFences({ fence }, true, UINT64_MAX);
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Wait for fence failed: " + std::to_string(static_cast<int>(result)));
    }

    void* computeResult = device.mapMemory(matrixBufferC.memory, 0, matrixBufferC.size);
    std::memcpy(matrixC, computeResult, matrixBufferC.size);
    device.unmapMemory(matrixBufferC.memory);
}
