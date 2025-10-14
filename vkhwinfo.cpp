// vkhwinfo - show information about available devices from Vulkan API perspective.
// Derived from 03_physical_device_selection.cpp example program from https://vulkan-tutorial.com/

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <optional>

#include <vulkan/vulkan.h>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;

    bool isComplete() {
        return graphicsFamily.has_value();
    }
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

bool isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    return indices.isComplete();
}

void printDeviceProperties(const VkPhysicalDeviceProperties &props) {
    const std::vector<std::string> deviceTypes = {
        "Other",
        "Integrated GPU",
        "Discrete GPU",
        "Virtual GPU",
        "CPU",
    };

    std::cout << "Device name: " << props.deviceName << std::endl;
    std::cout << "Device type: " << props.deviceType;
    if (props.deviceType < deviceTypes.size()) {
        std::cout << " (" << deviceTypes[props.deviceType] << ")";
    } else {
        std::cout << " (Unknown)"; 
    }
    std::cout << std::endl;
    std::cout << "API version: " << props.apiVersion << std::endl;
    std::cout << "Driver version: " << props.driverVersion << std::endl;
    std::cout << "Vendor ID: " << props.vendorID << std::endl;
    std::cout << "Device ID: " << props.deviceID << std::endl;
    //
    // FIXME Print all limits? https://docs.vulkan.org/spec/latest/chapters/limits.html#VkPhysicalDeviceLimits
    //
    std::cout << "Max computer shared memory size: " << props.limits.maxComputeSharedMemorySize << std::endl;

    //
    // FIXME Print sparseProperties? https://docs.vulkan.org/spec/latest/chapters/sparsemem.html#VkPhysicalDeviceSparseProperties
    //
}

int main() {
    try {
        VkInstance instance;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "vkhwinfo";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "N/A";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        createInfo.enabledExtensionCount = 0;
        createInfo.ppEnabledExtensionNames = nullptr;
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (auto i = 0; i < devices.size(); i++) {
            std::cout << "Device " << i << std::endl;
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(devices[i], &props);
            printDeviceProperties(props);
            std::cout << std::endl;
        }

        vkDestroyInstance(instance, nullptr);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
