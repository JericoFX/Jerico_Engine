// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#include <samplerate.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace JE::SamplerateTests
{
    void Require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    struct DeleteState
    {
        void operator()(SRC_STATE* state) const { src_delete(state); }
    };
    using State = std::unique_ptr<SRC_STATE, DeleteState>;

    State NewState(int converter, int channels)
    {
        int error = -1;
        State state(src_new(converter, channels, &error));
        Require(state != nullptr && error == 0, "src_new failed");
        Require(src_get_channels(state.get()) == channels, "channel count differs");
        return state;
    }

    // Small output buffers force partial consumption and final draining. The
    // oracle is the mathematical duration/waveform, not another SRC backend.
    std::vector<float> Stream(SRC_STATE* state, double ratio)
    {
        constexpr long frames = 8192;
        constexpr int channels = 2;
        constexpr long capacity = 97;
        constexpr float guard = 12345.0f;
        constexpr double twoPi = 6.2831853071795864769;
        std::vector<float> input(static_cast<size_t>(frames * channels));
        for (long frame = 0; frame < frames; ++frame)
        {
            const float value = static_cast<float>(0.25 * std::sin(twoPi * 440.0 * frame / 48000.0));
            input[static_cast<size_t>(frame * channels)] = value;
            input[static_cast<size_t>(frame * channels + 1)] = -0.5f * value;
        }
        Require(src_set_ratio(state, ratio) == 0, "src_set_ratio failed");
        std::vector<float> output;
        long consumed = 0;
        bool drained = false;
        bool sawFinalInput = false;
        bool sawNonFinalInput = false;
        for (int iteration = 0; iteration < 20000 && !drained; ++iteration)
        {
            const long offered = std::min(193L, frames - consumed);
            std::array<float, static_cast<size_t>(capacity * channels + 2)> buffer;
            buffer.fill(guard);
            SRC_DATA data{};
            data.data_in = input.data() + consumed * channels;
            data.data_out = buffer.data() + 1;
            data.input_frames = offered;
            data.output_frames = capacity;
            data.src_ratio = ratio;
            data.end_of_input = consumed + offered == frames ? 1 : 0;
            sawFinalInput = sawFinalInput || data.end_of_input != 0;
            sawNonFinalInput = sawNonFinalInput || data.end_of_input == 0;
            Require(src_process(state, &data) == 0, "stream src_process failed");
            Require(data.input_frames_used >= 0 && data.input_frames_used <= offered, "consumed outside offered frames");
            Require(data.output_frames_gen >= 0 && data.output_frames_gen <= capacity, "generated outside output capacity");
            Require(buffer.front() == guard && buffer.back() == guard, "output guard overwritten");
            consumed += data.input_frames_used;
            output.insert(output.end(), buffer.begin() + 1, buffer.begin() + 1 + data.output_frames_gen * channels);
            if (consumed == frames && data.output_frames_gen == 0) { drained = true; }
            else { Require(data.input_frames_used != 0 || data.output_frames_gen != 0, "stream stalled before completion"); }
        }
        Require(drained && consumed == frames && sawFinalInput && sawNonFinalInput, "stream did not consume and drain");
        const double actualFrames = static_cast<double>(output.size() / channels);
        Require(std::abs(actualFrames - frames * ratio) <= std::ceil(std::max(1.0, 1.0 / ratio)), "duration does not match rate ratio");
        Require(output.size() > 2048, "insufficient waveform for interior check");
        for (size_t frame = 0; frame < output.size() / channels; ++frame)
        {
            const float left = output[frame * channels];
            const float right = output[frame * channels + 1];
            Require(std::isfinite(left) && std::isfinite(right) && std::abs(left) < 0.3f, "nonfinite or excessive sample");
            Require(std::abs(right + 0.5f * left) < 0.00001f, "stereo channel ordering/gain lost");
            // Exclude start/end filter transients, including the best sinc.
            if (frame >= 512 && frame + 512 < output.size() / channels)
            {
                const double expected = 0.25 * std::sin(twoPi * 440.0 * static_cast<double>(frame) / (48000.0 * ratio));
                Require(std::abs(left - expected) < 0.003, "interior sample timing/amplitude differs");
            }
        }
        return output;
    }

    void Quality(int converter)
    {
        for (const double ratio : {1.0, 2.0 / 3.0, 1.5})
        {
            auto state = NewState(converter, 2);
            Stream(state.get(), ratio);
        }
    }

    void Api()
    {
        Require(std::strcmp(src_get_version(), "libsamplerate-0.2.1 (c) 2002-2008 Erik de Castro Lopo") == 0, "linked version is not 0.2.1");
        for (int converter = SRC_SINC_BEST_QUALITY; converter <= SRC_LINEAR; ++converter)
        {
            Require(src_get_name(converter) != nullptr && src_get_description(converter) != nullptr, "converter missing");
            auto state = NewState(converter, 1);
            Require(src_error(state.get()) == 0, "fresh state has error");
            int error = -1;
            State clone(src_clone(state.get(), &error));
            Require(clone != nullptr && error == 0 && src_get_channels(clone.get()) == 1, "clone ownership/channels failed");
        }
        Require(src_get_name(99) == nullptr && src_get_description(99) == nullptr, "unknown converter unexpectedly present");
        std::array<float, 512> input{};
        std::array<float, 1024> output;
        output.fill(7.0f);
        SRC_DATA data{};
        data.data_in = input.data(); data.data_out = output.data();
        data.input_frames = 512; data.output_frames = 1024; data.src_ratio = 2.0;
        Require(src_simple(&data, SRC_SINC_MEDIUM_QUALITY, 1) == 0 && data.input_frames_used == 512, "simple API failed");
        Require(std::abs(data.output_frames_gen - 1024L) <= 1, "simple output duration differs");
        for (long i = 0; i < data.output_frames_gen; ++i) { Require(output[static_cast<size_t>(i)] == 0.0f, "silence became signal"); }
    }

    void Conversions()
    {
        const std::array<short, 7> input{{-32768, -16384, -1, 0, 1, 16384, 32767}};
        std::array<float, 9> floating;
        floating.fill(42.0f);
        src_short_to_float_array(input.data(), floating.data() + 1, static_cast<int>(input.size()));
        Require(floating.front() == 42.0f && floating.back() == 42.0f, "short conversion guard overwritten");
        for (size_t i = 0; i < input.size(); ++i) { Require(floating[i + 1] == input[i] / 32768.0f, "short scale incorrect"); }
        std::array<short, 7> roundtrip{};
        src_float_to_short_array(floating.data() + 1, roundtrip.data(), static_cast<int>(roundtrip.size()));
        Require(roundtrip == input, "short/float exact roundtrip failed");
        const std::array<float, 5> clipped{{-2.0f, -1.0f, 0.0f, 1.0f, 2.0f}};
        std::array<short, 5> integers{};
        src_float_to_short_array(clipped.data(), integers.data(), static_cast<int>(integers.size()));
        Require(integers == std::array<short, 5>{{-32768, -32768, 0, 32767, 32767}}, "short saturation differs");
        const std::array<int, 3> wide{{-1073741824, 0, 1073741824}};
        std::array<float, 3> wideFloat{};
        std::array<int, 3> wideBack{};
        src_int_to_float_array(wide.data(), wideFloat.data(), 3);
        Require(wideFloat == std::array<float, 3>{{-0.5f, 0.0f, 0.5f}}, "int scaling differs");
        src_float_to_int_array(wideFloat.data(), wideBack.data(), 3);
        Require(wideBack == wide, "int exact roundtrip failed");
    }

    void Reset()
    {
        auto state = NewState(SRC_SINC_MEDIUM_QUALITY, 2);
        const auto before = Stream(state.get(), 2.0 / 3.0);
        Require(src_reset(state.get()) == 0 && src_get_channels(state.get()) == 2, "reset changed channels or failed");
        const auto after = Stream(state.get(), 2.0 / 3.0);
        Require(before == after, "reset did not reproduce fresh stream");
        for (const int converter : {SRC_LINEAR, SRC_ZERO_ORDER_HOLD})
        {
            std::array<float, 512> input{};
            std::array<float, 1024> output;
            output.fill(7.0f);
            SRC_DATA data{};
            data.data_in = input.data(); data.data_out = output.data();
            data.input_frames = 512; data.output_frames = 1024; data.src_ratio = 2.0;
            Require(src_simple(&data, converter, 1) == 0 && data.input_frames_used == 512, "linear/ZOH simple API failed");
            Require(std::abs(data.output_frames_gen - 1024L) <= 1, "linear/ZOH duration differs");
            for (long i = 0; i < data.output_frames_gen; ++i) { Require(output[static_cast<size_t>(i)] == 0.0f, "linear/ZOH silence became signal"); }
        }
    }

    void Errors()
    {
        int error = 0;
        State bad(src_new(99, 1, &error));
        Require(!bad && error != 0 && std::strlen(src_strerror(error)) != 0, "bad converter was accepted");
        error = 0; bad.reset(src_new(SRC_SINC_MEDIUM_QUALITY, 0, &error));
        Require(!bad && error != 0, "zero channels were accepted");
        Require(src_delete(nullptr) == nullptr && src_get_channels(nullptr) < 0 && src_reset(nullptr) != 0, "null state contract differs");
        auto state = NewState(SRC_SINC_MEDIUM_QUALITY, 1);
        Require(!src_is_valid_ratio(0.0) && !src_is_valid_ratio(1e9) && src_is_valid_ratio(44100.0 / 48000.0), "ratio bounds differ");
        Require(src_set_ratio(state.get(), 0.0) != 0 && src_process(state.get(), nullptr) != 0, "invalid ratio/data accepted");
        std::array<float, 8> storage;
        storage.fill(9.0f);
        SRC_DATA data{};
        data.data_in = storage.data(); data.data_out = storage.data();
        data.input_frames = 8; data.output_frames = 8; data.src_ratio = 1.0;
        Require(src_process(state.get(), &data) != 0, "overlapping data accepted");
        data.data_in = nullptr;
        Require(src_process(state.get(), &data) != 0, "missing input accepted");
        data.input_frames = 0; data.src_ratio = 0.0;
        Require(src_process(state.get(), &data) != 0, "invalid process ratio accepted");
        Require(std::all_of(storage.begin(), storage.end(), [](float value) { return value == 9.0f; }), "error mutated output");
    }
}

int main(int argc, char** argv)
{
    try
    {
        JE::SamplerateTests::Require(argc == 2, "one case required");
        const std::string name = argv[1];
        using namespace JE::SamplerateTests;
        if (name == "api") { Api(); }
        else if (name == "conversions") { Conversions(); }
        else if (name == "fast") { Quality(SRC_SINC_FASTEST); }
        else if (name == "medium") { Quality(SRC_SINC_MEDIUM_QUALITY); }
        else if (name == "best") { Quality(SRC_SINC_BEST_QUALITY); }
        else if (name == "reset") { Reset(); }
        else if (name == "errors") { Errors(); }
        else { throw std::runtime_error("unknown case"); }
        std::cout << "PASS " << name << '\n';
        return 0;
    }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
