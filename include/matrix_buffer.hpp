#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_handles.hpp>

struct MatrixBuffer {
    vk::Buffer buffer = VK_NULL_HANDLE;
    vk::DeviceMemory memory = VK_NULL_HANDLE;
    vk::DeviceSize size = 0;

    void initialize(
        const vk::Device& device,
        const vk::PhysicalDeviceMemoryProperties& memoryProperties,
        vk::DeviceSize size);

    void destroy(const vk::Device& device);

    void copy(const vk::Device& device, const uint32_t* data);

    uint32_t getMemoryTypeIndex(
        const vk::PhysicalDeviceMemoryProperties& memoryProperties,
        const vk::MemoryRequirements& memoryRequirements);
};
