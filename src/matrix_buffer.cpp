#include "matrix_buffer.hpp"

constexpr auto hostFlags =
    vk::MemoryPropertyFlagBits::eHostVisible |
    vk::MemoryPropertyFlagBits::eHostCoherent;


uint32_t MatrixBuffer::getMemoryTypeIndex(
        const vk::PhysicalDeviceMemoryProperties& memoryProperties,
        const vk::MemoryRequirements& memoryRequirements)
{
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        auto memPropertyFlags = memoryProperties.memoryTypes[i].propertyFlags;
        if ((memoryRequirements.memoryTypeBits & (1u << i)) && (memPropertyFlags & hostFlags) == hostFlags) {
            return i;
        }
    }
    throw std::runtime_error("No suitable memory type");
}

void MatrixBuffer::initialize(
        const vk::Device& device, 
        const vk::PhysicalDeviceMemoryProperties& memoryProperties,
        vk::DeviceSize deviceSize)
{
    vk::BufferCreateInfo createInfo{};
    createInfo.size = deviceSize * sizeof(uint32_t);
    createInfo.usage = vk::BufferUsageFlagBits::eStorageBuffer;
    createInfo.sharingMode = vk::SharingMode::eExclusive;
    
    auto result = device.createBuffer(&createInfo, nullptr, &buffer);
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Buffer creation failed: " + std::to_string(static_cast<int>(result)));
    }

    vk::MemoryRequirements memoryRequirements;
    device.getBufferMemoryRequirements(buffer, &memoryRequirements);

    uint32_t memoryTypeIndex = getMemoryTypeIndex(memoryProperties, memoryRequirements);

    vk::MemoryAllocateInfo allocateInfo(memoryRequirements.size, memoryTypeIndex);
    memory = device.allocateMemory(allocateInfo);
    device.bindBufferMemory(buffer, memory, 0);
    size = createInfo.size;
}

void MatrixBuffer::copy(const vk::Device& device, const uint32_t* data)
{
    void* dest = device.mapMemory(memory, 0, size);
    std::memcpy(dest, data, size);
    device.unmapMemory(memory);
}

void MatrixBuffer::destroy(const vk::Device& device)
{
    device.destroyBuffer(buffer);
    device.freeMemory(memory);
}
