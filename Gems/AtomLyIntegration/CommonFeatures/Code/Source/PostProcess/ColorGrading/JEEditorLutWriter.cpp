/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <PostProcess/ColorGrading/JEEditorLutWriter.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/writer.h>

#include <cmath>
#include <cstring>

namespace JE::ColorGrading
{
    namespace
    {
        bool CubeSize(size_t size, size_t channels, size_t& count)
        {
            const size_t limit = AZStd::numeric_limits<size_t>::max();
            if (size < 2 || size > limit / size)
            {
                return false;
            }
            const size_t square = size * size;
            if (size > limit / square || square * size > limit / channels)
            {
                return false;
            }
            count = square * size * channels;
            return true;
        }

        // The .azasset is published last, after both complete files have been written.
        // All cleanup paths refer only to files created by this call; collisions are never deleted.
        class LutFiles
        {
        public:
            explicit LutFiles(const AZStd::string& base)
                : m_3dl(base + ".3dl")
                , m_asset(base + ".azasset")
            {
                const auto suffix = AZ::Uuid::CreateRandom().ToString<AZStd::string>() + ".tmp";
                m_temp3dl = base + ".3dl." + suffix;
                m_tempAsset = base + "." + suffix;
            }

            ~LutFiles()
            {
                m_3dlFile.Close();
                m_assetFile.Close();
                if (m_created3dl)
                {
                    AZ::IO::SystemFile::Delete(m_temp3dl.c_str());
                }
                if (m_createdAsset)
                {
                    AZ::IO::SystemFile::Delete(m_tempAsset.c_str());
                }
                if (m_published3dl && !m_complete)
                {
                    AZ::IO::SystemFile::Delete(m_3dl.c_str());
                }
            }

            bool WriteAndPublish(const AZStd::string& text3dl, const rapidjson::StringBuffer& json)
            {
                constexpr int mode = AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY | AZ::IO::SystemFile::SF_OPEN_CREATE_NEW;
                if (AZ::IO::SystemFile::Exists(m_3dl.c_str()) || AZ::IO::SystemFile::Exists(m_asset.c_str()))
                {
                    return false;
                }
                m_created3dl = m_3dlFile.Open(m_temp3dl.c_str(), mode);
                if (!m_created3dl)
                {
                    return false;
                }
                m_createdAsset = m_assetFile.Open(m_tempAsset.c_str(), mode);
                if (!m_createdAsset ||
                    m_3dlFile.Write(text3dl.data(), text3dl.size()) != text3dl.size() ||
                    m_assetFile.Write(json.GetString(), json.GetSize()) != json.GetSize())
                {
                    return false;
                }
                m_3dlFile.Close();
                m_assetFile.Close();
                if (!AZ::IO::SystemFile::Rename(m_temp3dl.c_str(), m_3dl.c_str(), false))
                {
                    return false;
                }
                m_created3dl = false;
                m_published3dl = true;
                if (!AZ::IO::SystemFile::Rename(m_tempAsset.c_str(), m_asset.c_str(), false))
                {
                    return false;
                }
                m_createdAsset = false;
                m_complete = true;
                return true;
            }

        private:
            AZStd::string m_3dl;
            AZStd::string m_asset;
            AZStd::string m_temp3dl;
            AZStd::string m_tempAsset;
            AZ::IO::SystemFile m_3dlFile;
            AZ::IO::SystemFile m_assetFile;
            bool m_created3dl = false;
            bool m_createdAsset = false;
            bool m_published3dl = false;
            bool m_complete = false;
        };
    }

    AZ::Outcome<void, AZStd::string> ValidateLookupTable(const AZ::Render::LookupTableAsset& lookupTable)
    {
        size_t valueCount = 0;
        if (!CubeSize(lookupTable.m_intervals.size(), 3, valueCount) || lookupTable.m_values.size() != valueCount)
        {
            return AZ::Failure(AZStd::string("LUT must contain at least two intervals and exactly 3*N*N*N values."));
        }
        if (lookupTable.m_intervals.front() != 0 || lookupTable.m_intervals.back() != 1023)
        {
            return AZ::Failure(AZStd::string("LUT intervals must span 0 through 1023."));
        }
        AZ::u64 previous = 0;
        for (const AZ::u64 interval : lookupTable.m_intervals)
        {
            if (interval > 1023 || interval < previous)
            {
                return AZ::Failure(AZStd::string("LUT intervals must be nondecreasing 10-bit values."));
            }
            previous = interval;
        }
        for (const AZ::u64 value : lookupTable.m_values)
        {
            if (value > 4095)
            {
                return AZ::Failure(AZStd::string("LUT RGB values must be 12-bit values."));
            }
        }
        return AZ::Success();
    }

    AZ::Outcome<AZStd::string, AZStd::string> WriteLutSource(
        const JE::Render::RPI::AttachmentReadback::ReadbackResult& readback, const AZStd::string& absoluteOutputBase)
    {
        if (absoluteOutputBase.find('\0') != AZStd::string::npos ||
            !AZ::IO::PathView(absoluteOutputBase).IsAbsolute() || AZ::IO::PathView(absoluteOutputBase).Filename().empty())
        {
            return AZ::Failure(AZStd::string("LUT output base must be an absolute file path."));
        }
        if (readback.m_state != JE::Render::RPI::AttachmentReadback::ReadbackState::Success ||
            readback.m_attachmentType != AZ::RHI::AttachmentType::Image ||
            readback.m_imageDescriptor.m_format != AZ::RHI::Format::R32G32B32A32_FLOAT)
        {
            return AZ::Failure(AZStd::string("LUT readback must be a successful R32G32B32A32_FLOAT image."));
        }
        const auto& dimensions = readback.m_imageDescriptor.m_size;
        const size_t size = dimensions.m_height;
        size_t byteCount = 0;
        if (!CubeSize(size, sizeof(float) * 4, byteCount) || dimensions.m_depth != 1 || dimensions.m_width != size * size)
        {
            return AZ::Failure(AZStd::string("LUT dimensions must be N*N by N, depth 1, with N >= 2 and no size overflow."));
        }
        if (!readback.m_dataBuffer || readback.m_dataBuffer->size() != byteCount)
        {
            return AZ::Failure(AZStd::string("LUT readback must contain exactly the tightly packed RGBA float bytes."));
        }

        AZ::Render::LookupTableAsset lut;
        lut.m_name = "LookupTable";
        lut.m_intervals.reserve(size);
        lut.m_values.reserve(byteCount / sizeof(float) / 4 * 3);
        for (size_t i = 0; i < size; ++i)
        {
            lut.m_intervals.push_back(static_cast<AZ::u64>(1023) * i / (size - 1));
        }
        // Preserve the scripts' flattened layout u=g*N+r, v=b, and blue-first output order.
        for (size_t r = 0; r < size; ++r)
        {
            for (size_t g = 0; g < size; ++g)
            {
                for (size_t b = 0; b < size; ++b)
                {
                    const size_t pixelIndex = b * size * size + g * size + r;
                    float pixel[4];
                    std::memcpy(pixel, readback.m_dataBuffer->data() + pixelIndex * sizeof(pixel), sizeof(pixel));
                    for (float channel : pixel)
                    {
                        if (!std::isfinite(channel))
                        {
                            return AZ::Failure(AZStd::string("LUT readback contains a non-finite channel."));
                        }
                    }
                    for (size_t channel = 0; channel < 3; ++channel)
                    {
                        lut.m_values.push_back(static_cast<AZ::u64>(std::floor(AZStd::clamp(pixel[channel], 0.0f, 1.0f) * 4095.0f)));
                    }
                }
            }
        }
        const auto validation = ValidateLookupTable(lut);
        if (!validation.IsSuccess())
        {
            return AZ::Failure(validation.GetError());
        }

        AZStd::string text3dl;
        for (AZ::u64 interval : lut.m_intervals)
        {
            text3dl += AZStd::to_string(interval) + " ";
        }
        text3dl += '\n';
        for (size_t i = 0; i < lut.m_values.size(); i += 3)
        {
            text3dl += AZStd::string::format("%llu %llu %llu\n",
                static_cast<unsigned long long>(lut.m_values[i]),
                static_cast<unsigned long long>(lut.m_values[i + 1]),
                static_cast<unsigned long long>(lut.m_values[i + 2]));
        }

        rapidjson::StringBuffer json;
        rapidjson::Writer<rapidjson::StringBuffer> writer(json);
        writer.StartObject();
        writer.Key("Type");
        writer.String("JsonSerialization");
        writer.Key("Version");
        writer.Uint(1);
        writer.Key("ClassName");
        writer.String("LookupTableAsset");
        writer.Key("ClassData");
        writer.StartObject();
        writer.Key("Name");
        writer.String("LookupTable");
        writer.Key("Intervals");
        writer.StartArray();
        for (AZ::u64 interval : lut.m_intervals)
        {
            writer.Uint64(interval);
        }
        writer.EndArray();
        writer.Key("Values");
        writer.StartArray();
        for (AZ::u64 value : lut.m_values)
        {
            writer.Uint64(value);
        }
        writer.EndArray();
        writer.EndObject();
        writer.EndObject();

        LutFiles files(absoluteOutputBase);
        if (!files.WriteAndPublish(text3dl, json))
        {
            return AZ::Failure(AZStd::string("LUT output creation, writing or no-overwrite publication failed."));
        }
        return AZ::Success(absoluteOutputBase + ".azasset");
    }
}
