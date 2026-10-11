/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define VK_NO_PROTOTYPES
#include <Windows.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_layer.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace JE::VulkanValidationTest
{
    static_assert(VK_HEADER_VERSION == 261, "Use the audited Vulkan 1.3.261 headers");
    constexpr const char* LayerName = "VK_LAYER_KHRONOS_validation";
    constexpr const char* InvalidFlagsVuid = "VUID-VkInstanceCreateInfo-flags-parameter";

    void Require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    class Module
    {
    public:
        explicit Module(const char* path)
            : m_handle(LoadLibraryExA(path, nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32))
        {
            Require(m_handle != nullptr, "LoadLibraryEx failed for the explicit local module");
        }
        ~Module() { FreeLibrary(m_handle); }
        Module(const Module&) = delete;
        Module& operator=(const Module&) = delete;
        template<class T> T Proc(const char* name) const
        {
            const FARPROC raw = GetProcAddress(m_handle, name);
            Require(raw != nullptr, name);
            T result{};
            static_assert(sizeof(result) == sizeof(raw));
            std::memcpy(&result, &raw, sizeof(result));
            return result;
        }
    private:
        HMODULE m_handle;
    };

    void Exports(const char* dll)
    {
        std::fputs("TRACE 1 BEFORE Module layer(dll);\n", stderr); std::fflush(stderr);
        Module layer(dll);
        std::fputs("TRACE 2 BEFORE const auto getInstance = layer.Proc<PFN_vkGetInstanceProcAddr>(vkGetInstanceProcAddr);\n", stderr); std::fflush(stderr);
        const auto getInstance = layer.Proc<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        std::fputs("TRACE 3 BEFORE const auto getDevice = layer.Proc<PFN_vkGetDeviceProcAddr>(vkGetDeviceProcAddr);\n", stderr); std::fflush(stderr);
        const auto getDevice = layer.Proc<PFN_vkGetDeviceProcAddr>("vkGetDeviceProcAddr");
        std::fputs("TRACE 4 BEFORE const auto enumerate = layer.Proc<PFN_vkEnumerateInstanceLayerProperties>(vkEnumerateInstanceLayerProperties);\n", stderr); std::fflush(stderr);
        const auto enumerate = layer.Proc<PFN_vkEnumerateInstanceLayerProperties>("vkEnumerateInstanceLayerProperties");
        std::fputs("TRACE 5 BEFORE const auto extensions = layer.Proc<PFN_vkEnumerateInstanceExtensionProperties>(vkEnumerateInstanceExtensionProperties);\n", stderr); std::fflush(stderr);
        const auto extensions = layer.Proc<PFN_vkEnumerateInstanceExtensionProperties>("vkEnumerateInstanceExtensionProperties");
        std::fputs("TRACE 6 BEFORE const auto negotiate = layer.Proc<PFN_vkNegotiateLoaderLayerInterfaceVersion>(vkNegotiateLoaderLayerInterfaceVersion);\n", stderr); std::fflush(stderr);
        const auto negotiate = layer.Proc<PFN_vkNegotiateLoaderLayerInterfaceVersion>("vkNegotiateLoaderLayerInterfaceVersion");
        uint32_t count = 0;
        std::fputs("TRACE 7 BEFORE Require(enumerate(&count, nullptr) == VK_SUCCESS && count == 1, layer count);\n", stderr); std::fflush(stderr);
        Require(enumerate(&count, nullptr) == VK_SUCCESS && count == 1, "layer count");
        VkLayerProperties properties{};
        std::fputs("TRACE 8 BEFORE Require(enumerate(&count, &properties) == VK_SUCCESS && count == 1, layer enumeration);\n", stderr); std::fflush(stderr);
        Require(enumerate(&count, &properties) == VK_SUCCESS && count == 1, "layer enumeration");
        Require(std::strcmp(properties.layerName, LayerName) == 0
            && properties.specVersion == VK_MAKE_API_VERSION(0, 1, 3, 261), "exact layer/API identity");
        count = 0;
        std::fputs("TRACE 9 BEFORE Require(enumerate(&count, &properties) == VK_INCOMPLETE && count == 0, short enumeration buffer);\n", stderr); std::fflush(stderr);
        Require(enumerate(&count, &properties) == VK_INCOMPLETE && count == 0, "short enumeration buffer");
        std::fputs("TRACE 10 BEFORE Require(extensions(JE.missing.layer, &count, nullptr) == VK_ERROR_LAYER_NOT_PRESENT, unknown layer rejection);\n", stderr); std::fflush(stderr);
        Require(extensions("JE.missing.layer", &count, nullptr) == VK_ERROR_LAYER_NOT_PRESENT, "unknown layer rejection");
        std::fputs("TRACE 11 BEFORE Require(extensions(LayerName, &count, nullptr) == VK_SUCCESS && count > 0, real layer extensions);\n", stderr); std::fflush(stderr);
        Require(extensions(LayerName, &count, nullptr) == VK_SUCCESS && count > 0, "real layer extensions");
        std::fputs("TRACE 12 BEFORE Require(getInstance(VK_NULL_HANDLE, JE_missing_function) == nullptr, unknown entry point rejection);\n", stderr); std::fflush(stderr);
        Require(getInstance(VK_NULL_HANDLE, "JE_missing_function") == nullptr, "unknown entry point rejection");
        std::fputs("TRACE 13 BEFORE VkNegotiateLayerInterface legacy{};\n", stderr); std::fflush(stderr);
        VkNegotiateLayerInterface legacy{};
        legacy.sType = LAYER_NEGOTIATE_INTERFACE_STRUCT;
        legacy.loaderLayerInterfaceVersion = 1;
        Require(negotiate(&legacy) == VK_SUCCESS && legacy.pfnGetInstanceProcAddr == nullptr
            && legacy.pfnGetDeviceProcAddr == nullptr, "original version1 negotiation");
        std::fputs("TRACE 14 BEFORE VkNegotiateLayerInterface modern{};\n", stderr); std::fflush(stderr);
        VkNegotiateLayerInterface modern{};
        modern.sType = LAYER_NEGOTIATE_INTERFACE_STRUCT;
        modern.loaderLayerInterfaceVersion = 2;
        Require(negotiate(&modern) == VK_SUCCESS && modern.pfnGetInstanceProcAddr == getInstance
            && modern.pfnGetDeviceProcAddr == getDevice && modern.pfnGetPhysicalDeviceProcAddr != nullptr,
            "original version2 negotiation");
    }

    struct Diagnostics { bool expectedVuid = false; unsigned errors = 0; };
    VKAPI_ATTR VkBool32 VKAPI_CALL Callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* data, void* user)
    {
        auto& diagnostics = *static_cast<Diagnostics*>(user);
        if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0)
        {
            ++diagnostics.errors;
            if (data && data->pMessageIdName && std::strcmp(data->pMessageIdName, InvalidFlagsVuid) == 0)
            {
                diagnostics.expectedVuid = true;
            }
            std::fprintf(stderr, "VVL %s: %s\n", data && data->pMessageIdName ? data->pMessageIdName : "unknown",
                data && data->pMessage ? data->pMessage : "");
            return VK_TRUE; // Abort invalid input before it reaches the driver.
        }
        return VK_FALSE;
    }

    void Runtime(const char* dll, const char* loaderPath)
    {
        Exports(dll);
        const std::string path(dll);
        const auto separator = path.find_last_of("/\\");
        Require(separator != std::string::npos, "explicit layer directory");
        Require(SetEnvironmentVariableA("VK_LAYER_PATH", path.substr(0, separator).c_str()) != 0, "VK_LAYER_PATH");
        Module loader(loaderPath);
        const auto getInstance = loader.Proc<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        const auto create = reinterpret_cast<PFN_vkCreateInstance>(getInstance(VK_NULL_HANDLE, "vkCreateInstance"));
        const auto enumerate = reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(
            getInstance(VK_NULL_HANDLE, "vkEnumerateInstanceLayerProperties"));
        Require(create && enumerate, "real loader global entry points");
        uint32_t count = 0;
        Require(enumerate(&count, nullptr) == VK_SUCCESS && count > 0, "loader discovers layers");
        std::vector<VkLayerProperties> properties(count);
        Require(enumerate(&count, properties.data()) == VK_SUCCESS, "loader layer enumeration");
        bool exact = false;
        for (const auto& property : properties)
        {
            if (std::strcmp(property.layerName, LayerName) == 0)
            {
                Require(property.specVersion == VK_MAKE_API_VERSION(0, 1, 3, 261), "loader selected another layer version");
                exact = true;
            }
        }
        Require(exact, "loader did not discover the package JSON");
        Diagnostics diagnostics;
        VkDebugUtilsMessengerCreateInfoEXT callback{};
        callback.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        callback.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        callback.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
        callback.pfnUserCallback = Callback;
        callback.pUserData = &diagnostics;
        VkApplicationInfo application{};
        application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        application.pApplicationName = "JE local VVL consumer";
        application.apiVersion = VK_API_VERSION_1_1;
        const char* extension = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        const char* layer = LayerName;
        VkInstanceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        info.pNext = &callback;
        info.pApplicationInfo = &application;
        info.enabledLayerCount = 1;
        info.ppEnabledLayerNames = &layer;
        info.enabledExtensionCount = 1;
        info.ppEnabledExtensionNames = &extension;
        info.flags = static_cast<VkInstanceCreateFlags>(0x80000000u);
        VkInstance rejected = VK_NULL_HANDLE;
        const VkResult invalid = create(&info, nullptr, &rejected);
        Require(invalid == VK_ERROR_VALIDATION_FAILED_EXT && rejected == VK_NULL_HANDLE
            && diagnostics.expectedVuid, "invalid flags did not produce the exact VUID and reject the instance");
        diagnostics = {};
        info.flags = 0;
        VkInstance instance = VK_NULL_HANDLE;
        Require(create(&info, nullptr, &instance) == VK_SUCCESS && instance != VK_NULL_HANDLE
            && diagnostics.errors == 0, "valid instance creation");
        const auto destroy = reinterpret_cast<PFN_vkDestroyInstance>(getInstance(instance, "vkDestroyInstance"));
        Require(destroy != nullptr, "instance destroy entry point");
        destroy(instance, nullptr);
        Require(diagnostics.errors == 0, "valid instance destruction");
    }
}

int main(int argc, char** argv)
{
    try
    {
        JE::VulkanValidationTest::Require(argc == 4, "usage: consumer exports|runtime layer.dll local-vulkan-loader.dll");
        if (std::strcmp(argv[1], "exports") == 0) { JE::VulkanValidationTest::Exports(argv[2]); }
        else if (std::strcmp(argv[1], "runtime") == 0) { JE::VulkanValidationTest::Runtime(argv[2], argv[3]); }
        else { throw std::runtime_error("unknown case"); }
        std::printf("CASE %s PASS: %s\n", argv[1], argv[2]);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "CASE FAIL: %s\n", error.what());
        return 1;
    }
}
