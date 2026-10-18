/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Features/Utils/FrameCaptureBus.h>
#include <JE/Render/Features/Utils/FrameCaptureTestBus.h>
#include <Atom/Utils/DdsFile.h>
#include <Atom/Utils/PpmFile.h>
#include <Atom/Utils/PngFile.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/limits.h>

namespace JE::Render::Features
{
    void FrameCaptureError::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<FrameCaptureError>()
                ->Version(1)
                ->Field("ErrorMessage", &FrameCaptureError::m_errorMessage);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<FrameCaptureError>("FrameCaptureError")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "utils")
                ->Property("ErrorMessage", BehaviorValueProperty(&FrameCaptureError::m_errorMessage))
                    ->Attribute(AZ::Script::Attributes::Alias, "error_message");
        }
    }

    void FrameCaptureTestError::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<FrameCaptureTestError>()
                ->Version(1)
                ->Field("ErrorMessage", &FrameCaptureTestError::m_errorMessage);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<FrameCaptureTestError>("FrameCaptureTestError")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "utils")
                ->Property("ErrorMessage", BehaviorValueProperty(&FrameCaptureTestError::m_errorMessage))
                    ->Attribute(AZ::Script::Attributes::Alias, "error_message");
        }
    }

    FrameCaptureOutputResult DdsFrameCaptureOutput(
        const AZStd::string& outputFilePath, const JE::Render::RPI::AttachmentReadback::ReadbackResult& readbackResult)
    {
        // write the read back result of the image attachment to a dds file
        const auto outcome = AZ::DdsFile::WriteFile(
            outputFilePath,
            { readbackResult.m_imageDescriptor.m_size, readbackResult.m_imageDescriptor.m_format, readbackResult.m_dataBuffer.get() });

        return outcome.IsSuccess() ? FrameCaptureOutputResult{ FrameCaptureResult::Success, AZStd::nullopt }
                                   : FrameCaptureOutputResult{ FrameCaptureResult::InternalError, outcome.GetError().m_message };
    }

    FrameCaptureOutputResult R10G10B10A2PngFrameCaptureOutput(
        const AZStd::string& outputFilePath, const JE::Render::RPI::AttachmentReadback::ReadbackResult& readbackResult)
    {
        if (readbackResult.m_imageDescriptor.m_format != AZ::RHI::Format::R10G10B10A2_UNORM)
        {
            return { FrameCaptureResult::UnsupportedFormat, "Ten-bit PNG capture requires R10G10B10A2_UNORM" };
        }

        const auto& size = readbackResult.m_imageDescriptor.m_size;
        // PngFile uses uint32_t dimension arithmetic. Validate that arithmetic and the tight readback size before any read/write.
        if (outputFilePath.empty() || !readbackResult.m_dataBuffer || size.m_width == 0 || size.m_height == 0 || size.m_depth != 1 ||
            size.m_width > AZStd::numeric_limits<uint32_t>::max() / 4 / size.m_height)
        {
            return { FrameCaptureResult::InvalidArgument, "Invalid ten-bit PNG capture dimensions, path or buffer" };
        }
        const size_t byteCount = static_cast<size_t>(size.m_width) * size.m_height * 4;
        if (readbackResult.m_dataBuffer->size() != byteCount)
        {
            return { FrameCaptureResult::InvalidArgument, "Ten-bit PNG capture buffer does not match the image dimensions" };
        }
        const AZStd::string ddsPath = outputFilePath + ".dds";
        if (AZ::IO::SystemFile::Exists(outputFilePath.c_str()) || AZ::IO::SystemFile::Exists(ddsPath.c_str()))
        {
            return { FrameCaptureResult::FileWriteError, "Ten-bit PNG capture requires new PNG and DDS output paths" };
        }

        AZStd::vector<uint8_t> preview(byteCount);
        const auto& original = *readbackResult.m_dataBuffer;
        for (size_t offset = 0; offset < byteCount; offset += 4)
        {
            // R10G10B10A2 is packed little-endian. Byte reads avoid alignment and aliasing assumptions.
            const uint32_t pixel = uint32_t{ original[offset] } | (uint32_t{ original[offset + 1] } << 8) |
                (uint32_t{ original[offset + 2] } << 16) | (uint32_t{ original[offset + 3] } << 24);
            preview[offset] = static_cast<uint8_t>(((pixel & 1023u) * 255u + 511u) / 1023u);
            preview[offset + 1] = static_cast<uint8_t>((((pixel >> 10) & 1023u) * 255u + 511u) / 1023u);
            preview[offset + 2] = static_cast<uint8_t>((((pixel >> 20) & 1023u) * 255u + 511u) / 1023u);
            preview[offset + 3] = static_cast<uint8_t>((pixel >> 30) * 85u);
        }
        auto image = AZ::Utils::PngFile::Create(size, AZ::RHI::Format::R8G8B8A8_UNORM, AZStd::move(preview));
        if (!image)
        {
            return { FrameCaptureResult::InternalError, "Unable to create ten-bit PNG preview" };
        }
        const auto ddsResult = DdsFrameCaptureOutput(ddsPath, readbackResult);
        if (ddsResult.m_result != FrameCaptureResult::Success)
        {
            return ddsResult;
        }

        AZ::Utils::PngFile::SaveSettings saveSettings;
        if (auto console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            console->GetCvarValue("r_pngCompressionLevel", saveSettings.m_compressionLevel);
        }
        saveSettings.m_stripAlpha = false;
        AZStd::string pngError;
        saveSettings.m_errorHandler = [&pngError](const char* message) { pngError = message; };
        if (!image.Save(outputFilePath.c_str(), saveSettings))
        {
            return { FrameCaptureResult::FileWriteError,
                AZStd::string::format("Unable to save ten-bit PNG preview '%s': %s; original DDS retained at '%s'",
                    outputFilePath.c_str(), pngError.c_str(), ddsPath.c_str()) };
        }
        return { FrameCaptureResult::Success, AZStd::nullopt };
    }

    FrameCaptureOutputResult PpmFrameCaptureOutput(
        const AZStd::string& outputFilePath, const JE::Render::RPI::AttachmentReadback::ReadbackResult& readbackResult)
    {
        // write the read back result of the image attachment to a buffer
        const AZStd::vector<uint8_t> outBuffer = AZ::Utils::PpmFile::CreatePpmFromImageBuffer(
            *readbackResult.m_dataBuffer.get(), readbackResult.m_imageDescriptor.m_size, readbackResult.m_imageDescriptor.m_format);

        // write the buffer to a ppm file
        if (AZ::IO::FileIOStream fileStream(outputFilePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath);
            fileStream.IsOpen())
        {
            fileStream.Write(outBuffer.size(), outBuffer.data());
            fileStream.Close();

            return FrameCaptureOutputResult{ FrameCaptureResult::Success, AZStd::nullopt };
        }

        return FrameCaptureOutputResult{ FrameCaptureResult::FileWriteError,
                                         AZStd::string::format("Failed to open file %s for writing", outputFilePath.c_str()) };
    }
} // namespace JE::Render::Features
