//
// Created by John Kindem on 2021/4/28.
//

#include <cstring>
#include <stdexcept>

#include <Explosion/Driver/GpuBuffer.h>
#include <Explosion/Driver/Driver.h>
#include <Explosion/Driver/CommandBuffer.h>
#include <Explosion/Driver/VkAdapater.h>

namespace Explosion {
    GpuBuffer::GpuBuffer(Driver& driver, const Config& config)
        : GpuRes(driver), device(*driver.GetDevice()), config(config) {}

    GpuBuffer::~GpuBuffer() = default;

    void GpuBuffer::OnCreate()
    {
        GpuRes::OnCreate();
        CreateBuffer();
        AllocateMemory();
    }

    void GpuBuffer::OnDestroy()
    {
        GpuRes::OnDestroy();
        FreeMemory();
        DestroyBuffer();
    }

    uint32_t GpuBuffer::GetSize() const
    {
        return config.size;
    }

    const VkBuffer& GpuBuffer::GetVkBuffer() const
    {
        return vkBuffer;
    }

    const VkDeviceMemory& GpuBuffer::GetVkDeviceMemory() const
    {
        return vkDeviceMemory;
    }

    void GpuBuffer::SetupBufferCreateInfo(VkBufferCreateInfo& createInfo) {}

    VkMemoryPropertyFlags GpuBuffer::GetMemoryPropertyFlags()
    {
        return 0;
    }

    void GpuBuffer::CreateBuffer()
    {
        VkBufferCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.size = static_cast<VkDeviceSize>(config.size);
        createInfo.usage = VkGetFlags<BufferUsage, VkBufferUsageFlagBits>(config.usages);
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;

        if (vkCreateBuffer(device.GetVkDevice(), &createInfo, nullptr, &vkBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create vulkan buffer");
        }
    }

    void GpuBuffer::DestroyBuffer()
    {
        vkDestroyBuffer(device.GetVkDevice(), vkBuffer, nullptr);
    }


    void GpuBuffer::AllocateMemory()
    {
        VkMemoryRequirements memoryRequirements {};
        vkGetBufferMemoryRequirements(device.GetVkDevice(), vkBuffer, &memoryRequirements);
        std::optional<uint32_t> memType = FindMemoryType(
            device.GetVkPhysicalDeviceMemoryProperties(),
            memoryRequirements.memoryTypeBits,
            GetMemoryPropertyFlags()
        );
        if (!memType.has_value()) {
            throw std::runtime_error("failed to find suitable memory type");
        }

        VkMemoryAllocateInfo allocateInfo {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.pNext = nullptr;
        allocateInfo.allocationSize = static_cast<VkDeviceSize>(config.size);
        allocateInfo.memoryTypeIndex = memType.value();

        if (vkAllocateMemory(device.GetVkDevice(), &allocateInfo, nullptr, &vkDeviceMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate vulkan device memory");
        }
        vkBindBufferMemory(device.GetVkDevice(), vkBuffer, vkDeviceMemory, 0);
    }

    void GpuBuffer::FreeMemory()
    {
        vkFreeMemory(device.GetVkDevice(), vkDeviceMemory, nullptr);
    }

    HostVisibleBuffer::HostVisibleBuffer(Driver& driver, const Config& config) : GpuBuffer(driver, config) {}

    HostVisibleBuffer::~HostVisibleBuffer() = default;

    void HostVisibleBuffer::UpdateData(void* data)
    {
        void* mapped = nullptr;
        vkMapMemory(device.GetVkDevice(), vkDeviceMemory, 0, static_cast<VkDeviceSize>(config.size), 0, &mapped);
        memcpy(mapped, data, static_cast<size_t>(config.size));
        vkUnmapMemory(device.GetVkDevice(), vkDeviceMemory);
    }

    VkMemoryPropertyFlags HostVisibleBuffer::GetMemoryPropertyFlags()
    {
        return GpuBuffer::GetMemoryPropertyFlags()
            | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }

    DeviceLocalBuffer::DeviceLocalBuffer(Driver& driver, const Config& config) : GpuBuffer(driver, config) {}

    DeviceLocalBuffer::~DeviceLocalBuffer() = default;

    void DeviceLocalBuffer::SetupBufferCreateInfo(VkBufferCreateInfo& createInfo)
    {
        GpuBuffer::SetupBufferCreateInfo(createInfo);
        createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    VkMemoryPropertyFlags DeviceLocalBuffer::GetMemoryPropertyFlags()
    {
        return GpuBuffer::GetMemoryPropertyFlags()
            | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    void DeviceLocalBuffer::UpdateData(void* data)
    {
        GpuBuffer::Config stagingConfig = {
            config.size,
            { BufferUsage::TRANSFER_SRC }
        };
        auto* stagingBuffer = driver.CreateGpuRes<HostVisibleBuffer>(stagingConfig);
        stagingBuffer->UpdateData(data);
        {
            auto* commandBuffer = driver.CreateGpuRes<CommandBuffer>();
            commandBuffer->EncodeCommands([&stagingBuffer, this](auto* encoder) -> void {
                encoder->CopyBuffer(stagingBuffer, this);
            });
            commandBuffer->SubmitNow();
            driver.DestroyGpuRes<CommandBuffer>(commandBuffer);
        }
        driver.DestroyGpuRes<HostVisibleBuffer>(stagingBuffer);
    }
}