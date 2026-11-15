// SPDX-License-Identifier: AGPL-3.0-or-later

#include "dsp/NeutralStft.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
using spectral_decay::NeutralStft;
using TailMode = spectral_decay::NeutralStft::TailMode;

constexpr std::array<double, 2> sampleRates { 44100.0, 48000.0 };
constexpr std::array<std::size_t, 4> blockSizes { 1, 64, 512, 257 };
constexpr std::array<double, 12> toneFrequencies {
    31.25, 62.5, 125.0, 250.0, 500.0, 1000.0,
    2000.0, 4000.0, 8000.0, 12000.0, 16000.0, 20000.0
};

struct ToneWindow
{
    double frequencyHz {};
    std::size_t begin {};
    std::size_t length {};
};

struct TestSignal
{
    std::string name;
    std::vector<std::vector<float>> input;
    std::vector<ToneWindow> toneWindows;
    std::optional<std::size_t> latencyImpulsePosition;
    std::optional<std::size_t> activeChannel;
    std::optional<std::size_t> inactiveChannel;
    bool compareAcrossBlocks { true };
};

struct RunResult
{
    std::vector<std::vector<float>> output;
    std::size_t inputFrames {};
    std::size_t outputFrames {};
    int latency {};
};

struct Summary
{
    std::size_t exactPasses {};
    std::size_t exactFailures {};
    std::size_t measurements {};
    double maxNullPeak {};
    double maxNullRms {};
    double maxBlockDiffPeak {};
    double maxBlockDiffRms {};
};

class CsvReport
{
public:
    explicit CsvReport(std::ostream& stream) : out(stream)
    {
        out.imbue(std::locale::classic());
        out << "fs,block,layout,signal,channel,channelIndex,window,windowStart,"
               "windowEnd,windowBasis,inputframes,outputframes,"
               "reportedlatency,metric,residualPeak,residualPeakDbfs,"
               "residualRms,residualRmsDbfs,outputPeak,outputPeakDbfs,gain,gainDb,"
               "frequencyHz,measuredFrequencyHz,measuredLatency,ambiguity,"
               "inputCorrelation,outputCorrelation,check,status,"
               "inputEstimatedFrequencyHz,outputEstimatedFrequencyHz,"
               "frequencyDifferenceHz\n";
    }

    void row(const std::string& fs,
             const std::string& block,
             const std::string& layout,
             const std::string& signal,
             const std::string& channel,
             const std::string& window,
             const std::string& inputFrames,
             const std::string& outputFrames,
             const std::string& latency,
             const std::string& metric,
             const std::optional<double>& residualPeak = {},
             const std::optional<double>& residualRms = {},
             const std::optional<double>& outputPeak = {},
             const std::optional<double>& gain = {},
             const std::optional<double>& frequencyHz = {},
             const std::optional<double>& measuredFrequencyHz = {},
             const std::string& measuredLatency = {},
             const std::string& ambiguity = {},
             const std::optional<double>& inputCorrelation = {},
             const std::optional<double>& outputCorrelation = {},
             const std::string& check = {},
             const std::string& status = {},
             const std::string& windowStart = {},
             const std::string& windowEnd = {},
             const std::string& windowBasis = {},
             const std::optional<double>& inputEstimatedFrequencyHz = {},
             const std::optional<double>& outputEstimatedFrequencyHz = {},
             const std::optional<double>& frequencyDifferenceHz = {})
    {
        const std::string index = channel == "M" || channel == "L" ? "0"
                                : channel == "R" ? "1"
                                : channel == "LR" ? "0|1" : "";
        out << csv(fs) << ',' << csv(block) << ',' << csv(layout) << ','
            << csv(signal) << ',' << csv(channel) << ',' << csv(index) << ','
            << csv(window) << ',' << csv(windowStart) << ','
            << csv(windowEnd) << ',' << csv(windowBasis) << ','
            << csv(inputFrames) << ',' << csv(outputFrames) << ','
            << csv(latency) << ',' << csv(metric) << ','
            << number(residualPeak) << ',' << decibels(residualPeak) << ','
            << number(residualRms) << ',' << decibels(residualRms) << ','
            << number(outputPeak) << ',' << decibels(outputPeak) << ','
            << number(gain) << ',' << decibels(gain) << ','
            << number(frequencyHz) << ',' << number(measuredFrequencyHz) << ','
            << csv(measuredLatency) << ',' << csv(ambiguity) << ','
            << number(inputCorrelation) << ',' << number(outputCorrelation)
            << ',' << csv(check) << ',' << csv(status) << ','
            << number(inputEstimatedFrequencyHz) << ','
            << number(outputEstimatedFrequencyHz) << ','
            << number(frequencyDifferenceHz) << '\n';
    }

    bool good() const { return static_cast<bool>(out); }

private:
    static std::string csv(const std::string& value)
    {
        if (value.find_first_of(",\"\r\n") == std::string::npos)
            return value;
        std::string escaped = "\"";
        for (const char ch : value)
        {
            if (ch == '"')
                escaped += '"';
            escaped += ch;
        }
        escaped += '"';
        return escaped;
    }

    static std::string number(const std::optional<double>& value)
    {
        if (!value)
            return {};
        if (!std::isfinite(*value))
            return "nonfinite";
        if (*value == 0.0)
            return "0";
        std::ostringstream text;
        text.imbue(std::locale::classic());
        text << std::setprecision(10) << *value;
        return text.str();
    }

    static std::string decibels(const std::optional<double>& value)
    {
        if (!value)
            return {};
        if (!std::isfinite(*value) || *value < 0.0)
            return "nonfinite";
        if (*value == 0.0)
            return "-inf";
        std::ostringstream text;
        text.imbue(std::locale::classic());
        text << std::setprecision(10) << 20.0 * std::log10(*value);
        return text.str();
    }

    std::ostream& out;
};

std::string toString(std::size_t value)
{
    return std::to_string(value);
}

std::string toString(int value)
{
    return std::to_string(value);
}

std::string toString(long long value)
{
    return std::to_string(value);
}

std::string toString(double value)
{
    std::ostringstream text;
    text.imbue(std::locale::classic());
    text << std::setprecision(10) << value;
    return text.str();
}

std::string channelName(std::size_t channel, std::size_t channelCount)
{
    if (channelCount == 1)
        return "M";
    return channel == 0 ? "L" : "R";
}

std::string layoutName(std::size_t channelCount)
{
    return channelCount == 1 ? "mono" : "stereo";
}

std::size_t framesFor(double seconds, double sampleRate)
{
    return static_cast<std::size_t>(std::llround(seconds * sampleRate));
}

TestSignal makeBlankSignal(const std::string& name,
                           std::size_t channelCount,
                           std::size_t frames)
{
    TestSignal signal;
    signal.name = name;
    signal.input.assign(channelCount, std::vector<float>(frames, 0.0f));
    return signal;
}

std::vector<float> makeNoiseBurst(double sampleRate,
                                  std::uint32_t seed,
                                  float amplitude)
{
    const auto burstFrames = framesFor(0.5, sampleRate);
    std::vector<float> result(framesFor(1.0, sampleRate), 0.0f);
    std::uint32_t state = seed;
    constexpr double uint32Max = 4294967295.0;
    for (std::size_t i = 0; i < burstFrames; ++i)
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        const double unit = 2.0 * (static_cast<double>(state) / uint32Max) - 1.0;
        result[i] = static_cast<float>(amplitude * unit);
    }
    return result;
}

std::vector<float> makeLogSweep(double sampleRate)
{
    constexpr double startHz = 20.0;
    constexpr double endHz = 20000.0;
    constexpr double durationSeconds = 10.0;
    constexpr double edgeSeconds = 0.05;
    constexpr double amplitude = 0.25;
    const auto frames = framesFor(durationSeconds, sampleRate);
    std::vector<float> result(frames, 0.0f);
    const double logarithmicRatio = std::log(endHz / startHz);

    for (std::size_t i = 0; i < frames; ++i)
    {
        const double time = static_cast<double>(i) / sampleRate;
        const double phase = 2.0 * std::acos(-1.0) * startHz * durationSeconds
                           * (std::exp(logarithmicRatio * time / durationSeconds) - 1.0)
                           / logarithmicRatio;
        double fade = 1.0;
        if (time < edgeSeconds)
            fade *= 0.5 * (1.0 - std::cos(std::acos(-1.0) * time / edgeSeconds));
        const double remaining = durationSeconds - time;
        if (remaining < edgeSeconds)
            fade *= 0.5 * (1.0 - std::cos(std::acos(-1.0) * remaining / edgeSeconds));
        result[i] = static_cast<float>(amplitude * fade * std::sin(phase));
    }
    return result;
}

std::vector<float> makeSteppedTones(double sampleRate,
                                    std::vector<ToneWindow>& windows)
{
    constexpr double secondsPerTone = 1.0;
    constexpr double measurementSeconds = 0.5;
    constexpr double amplitude = 0.25;
    const auto segmentFrames = framesFor(secondsPerTone, sampleRate);
    const auto measurementFrames = framesFor(measurementSeconds, sampleRate);
    std::vector<float> result(segmentFrames * toneFrequencies.size(), 0.0f);
    windows.clear();
    windows.reserve(toneFrequencies.size());

    for (std::size_t tone = 0; tone < toneFrequencies.size(); ++tone)
    {
        const double frequency = toneFrequencies[tone];
        const std::size_t segmentBegin = tone * segmentFrames;
        for (std::size_t i = 0; i < segmentFrames; ++i)
        {
            const double time = static_cast<double>(i) / sampleRate;
            result[segmentBegin + i] = static_cast<float>(
                amplitude * std::sin(2.0 * std::acos(-1.0) * frequency * time));
        }
        windows.push_back({ frequency,
                            segmentBegin + segmentFrames - measurementFrames,
                            measurementFrames });
    }
    return result;
}

std::vector<TestSignal> makeSignals(double sampleRate,
                                    std::size_t channelCount,
                                    std::size_t blockSize)
{
    std::vector<TestSignal> signals;
    const auto oneSecond = framesFor(1.0, sampleRate);

    signals.push_back(makeBlankSignal("silence_1s", channelCount, oneSecond));

    TestSignal impulseTrain = makeBlankSignal(
        "impulse_train_8_alternating_025", channelCount, framesFor(2.0, sampleRate));
    for (std::size_t pulse = 0; pulse < 8; ++pulse)
    {
        const auto position = pulse * framesFor(0.25, sampleRate);
        const float amplitude = pulse % 2 == 0 ? 0.25f : -0.25f;
        for (auto& channel : impulseTrain.input)
            channel[position] = amplitude;
    }
    signals.push_back(std::move(impulseTrain));

    const auto sweep = makeLogSweep(sampleRate);
    TestSignal sweepSignal = makeBlankSignal("log_sweep_20Hz_20kHz_10s",
                                             channelCount,
                                             sweep.size());
    for (auto& channel : sweepSignal.input)
        channel = sweep;
    signals.push_back(std::move(sweepSignal));

    const auto noise = makeNoiseBurst(sampleRate, 0x6D2B79F5u, 0.1f);
    TestSignal noiseSignal = makeBlankSignal("xorshift32_noise_burst_seed_6D2B79F5",
                                             channelCount,
                                             noise.size());
    for (auto& channel : noiseSignal.input)
        channel = noise;
    signals.push_back(std::move(noiseSignal));

    constexpr double nearFullAmplitude = 0.9885530946569389; // 10^(-0.1/20)
    TestSignal nearFull = makeBlankSignal("near_full_1kHz_minus_0p1dBFS",
                                          channelCount,
                                          oneSecond);
    for (std::size_t i = 0; i < oneSecond; ++i)
    {
        const double time = static_cast<double>(i) / sampleRate;
        const float value = static_cast<float>(
            nearFullAmplitude * std::sin(2.0 * std::acos(-1.0) * 1000.0 * time));
        for (auto& channel : nearFull.input)
            channel[i] = value;
    }
    signals.push_back(std::move(nearFull));

    TestSignal tones = makeBlankSignal("stepped_tones_1s_each_amplitude_025",
                                      channelCount,
                                      framesFor(12.0, sampleRate));
    const auto toneMono = makeSteppedTones(sampleRate, tones.toneWindows);
    for (auto& channel : tones.input)
        channel = toneMono;
    signals.push_back(std::move(tones));

    if (channelCount == 2)
    {
        const auto independentLeft = makeNoiseBurst(sampleRate, 0x6D2B79F5u, 0.1f);
        const auto independentRight = makeNoiseBurst(sampleRate, 0x1B873593u, 0.1f);

        TestSignal correlated = makeBlankSignal("correlated_stereo_noise",
                                                channelCount,
                                                noise.size());
        correlated.input[0] = noise;
        correlated.input[1] = noise;
        signals.push_back(std::move(correlated));

        TestSignal independent = makeBlankSignal("independent_stereo_noise_two_seeds",
                                                 channelCount,
                                                 noise.size());
        independent.input[0] = independentLeft;
        independent.input[1] = independentRight;
        signals.push_back(std::move(independent));

        TestSignal leftOnly = makeBlankSignal("left_only_noise",
                                              channelCount,
                                              independentLeft.size());
        leftOnly.input[0] = independentLeft;
        leftOnly.inactiveChannel = 1;
        signals.push_back(std::move(leftOnly));

        TestSignal rightOnly = makeBlankSignal("right_only_noise",
                                               channelCount,
                                               independentRight.size());
        rightOnly.input[1] = independentRight;
        rightOnly.inactiveChannel = 0;
        signals.push_back(std::move(rightOnly));
    }

    const std::size_t impulseFrames = 3 * NeutralStft::fftSize;
    std::set<std::size_t> impulsePositions {
        0,
        1,
        NeutralStft::hopSize - 1,
        NeutralStft::hopSize,
        NeutralStft::hopSize + 1,
        NeutralStft::fftSize - 1,
        NeutralStft::fftSize,
        NeutralStft::fftSize + 1,
        impulseFrames - 1
    };
    if (blockSize > 0)
    {
        impulsePositions.insert(blockSize - 1);
        impulsePositions.insert(blockSize);
        impulsePositions.insert(blockSize + 1);
    }

    for (const auto position : impulsePositions)
    {
        for (std::size_t active = 0; active < channelCount; ++active)
        {
            const auto channelTag = channelCount == 1 ? "M" : (active == 0 ? "L" : "R");
            TestSignal impulse = makeBlankSignal(
                "unit_impulse_at_" + toString(position) + "_" + channelTag,
                channelCount,
                impulseFrames);
            impulse.input[active][position] = 1.0f;
            impulse.latencyImpulsePosition = position;
            impulse.activeChannel = active;
            if (channelCount == 2)
                impulse.inactiveChannel = 1 - active;
            // Block-dependent boundary positions are not the same stimulus for every block.
            const bool fixedBoundary = position == 0
                                    || position == 1
                                    || position == NeutralStft::hopSize - 1
                                    || position == NeutralStft::hopSize
                                    || position == NeutralStft::hopSize + 1
                                    || position == NeutralStft::fftSize - 1
                                    || position == NeutralStft::fftSize
                                    || position == NeutralStft::fftSize + 1
                                    || position == impulseFrames - 1;
            impulse.compareAcrossBlocks = fixedBoundary;
            signals.push_back(std::move(impulse));
        }
    }

    return signals;
}

RunResult processSignal(NeutralStft& processor,
                        const TestSignal& signal,
                        std::size_t blockSize,
                        bool insertZeroBlocks = false,
                        bool appendFlush = true,
                        bool splitAtMidpoint = false)
{
    RunResult result;
    result.inputFrames = signal.input.empty() ? 0 : signal.input.front().size();
    result.latency = processor.latencySamples();

    const long long requestedFlush = static_cast<long long>(result.latency)
                                   + 4LL * static_cast<long long>(NeutralStft::fftSize);
    const std::size_t flushFrames = !appendFlush ? 0
        : requestedFlush > 0 ? static_cast<std::size_t>(requestedFlush)
                             : 4 * NeutralStft::fftSize;
    result.outputFrames = result.inputFrames + flushFrames;
    result.output.assign(signal.input.size(),
                         std::vector<float>(result.outputFrames, 0.0f));
    for (std::size_t channel = 0; channel < signal.input.size(); ++channel)
        std::copy(signal.input[channel].begin(),
                  signal.input[channel].end(),
                  result.output[channel].begin());

    if (insertZeroBlocks)
        processor.process(nullptr, 0);

    const std::size_t middlePosition = result.inputFrames / 2;
    const bool splitAtMiddle = insertZeroBlocks || splitAtMidpoint;
    bool middleBoundaryHandled = !splitAtMiddle;
    std::array<float*, 2> channelPointers {};
    std::size_t offset = 0;
    while (offset < result.outputFrames)
    {
        if (splitAtMiddle && !middleBoundaryHandled && offset >= middlePosition)
        {
            if (insertZeroBlocks)
                processor.process(nullptr, 0);
            middleBoundaryHandled = true;
        }

        auto count = std::min(blockSize, result.outputFrames - offset);
        if (splitAtMiddle && !middleBoundaryHandled)
            count = std::min(count, middlePosition - offset);
        for (std::size_t channel = 0; channel < result.output.size(); ++channel)
            channelPointers[channel] = result.output[channel].data() + offset;
        processor.process(channelPointers.data(), count);
        offset += count;
    }

    if (insertZeroBlocks)
    {
        if (!middleBoundaryHandled)
            processor.process(nullptr, 0);
        processor.process(nullptr, 0);
    }
    return result;
}

struct WindowMetrics
{
    double peak {};
    double rms {};
    double outputPeak {};
    bool finite { true };
};

WindowMetrics compareWindow(const TestSignal& signal,
                            const RunResult& result,
                            std::size_t channel,
                            std::size_t begin,
                            std::size_t end)
{
    WindowMetrics metrics;
    long double residualEnergy = 0.0L;
    std::size_t count = 0;
    for (std::size_t frame = begin; frame < end; ++frame)
    {
        const double output = result.output[channel][frame];
        if (!std::isfinite(output))
            metrics.finite = false;
        metrics.outputPeak = std::max(metrics.outputPeak, std::abs(output));

        const long long inputFrame = static_cast<long long>(frame)
                                   - static_cast<long long>(result.latency);
        double reference = 0.0;
        if (inputFrame >= 0
            && static_cast<std::size_t>(inputFrame) < signal.input[channel].size())
            reference = signal.input[channel][static_cast<std::size_t>(inputFrame)];
        const double residual = output - reference;
        if (!std::isfinite(residual))
            metrics.finite = false;
        metrics.peak = std::max(metrics.peak, std::abs(residual));
        residualEnergy += static_cast<long double>(residual) * residual;
        ++count;
    }
    if (count > 0)
        metrics.rms = std::sqrt(static_cast<double>(residualEnergy / count));
    return metrics;
}

bool allFinite(const RunResult& result, std::size_t channel)
{
    return std::all_of(result.output[channel].begin(),
                       result.output[channel].end(),
                       [](float sample) { return std::isfinite(sample); });
}

bool exactlyEqualChannel(const std::vector<float>& left,
                        const std::vector<float>& right)
{
    return left.size() == right.size()
        && (left.empty()
            || std::memcmp(left.data(), right.data(), left.size() * sizeof(float)) == 0);
}

std::pair<double, double> differenceMetrics(
    const std::vector<std::vector<float>>& left,
    const std::vector<std::vector<float>>& right,
    std::size_t channel)
{
    const std::size_t count = std::max(left[channel].size(), right[channel].size());
    double peak = 0.0;
    long double energy = 0.0L;
    for (std::size_t i = 0; i < count; ++i)
    {
        const double a = i < left[channel].size() ? left[channel][i] : 0.0;
        const double b = i < right[channel].size() ? right[channel][i] : 0.0;
        const double delta = a - b;
        peak = std::max(peak, std::abs(delta));
        energy += static_cast<long double>(delta) * delta;
    }
    return { peak, count == 0 ? 0.0 : std::sqrt(static_cast<double>(energy / count)) };
}

std::pair<double, double> latencyAlignedDifference(
    const RunResult& reference,
    const RunResult& candidate,
    std::size_t channel)
{
    const long long referenceStart = -static_cast<long long>(reference.latency);
    const long long candidateStart = -static_cast<long long>(candidate.latency);
    const long long begin = std::min(referenceStart, candidateStart);
    const long long end = static_cast<long long>(reference.inputFrames)
                        + 4LL * static_cast<long long>(NeutralStft::fftSize);
    const auto count = static_cast<std::size_t>(std::max(0LL, end - begin));
    double peak = 0.0;
    long double energy = 0.0L;
    for (long long time = begin; time < end; ++time)
    {
        const long long refIndex = time + reference.latency;
        const long long candidateIndex = time + candidate.latency;
        const double ref = refIndex >= 0
                        && static_cast<std::size_t>(refIndex) < reference.outputFrames
                         ? reference.output[channel][static_cast<std::size_t>(refIndex)]
                         : 0.0;
        const double current = candidateIndex >= 0
                           && static_cast<std::size_t>(candidateIndex) < candidate.outputFrames
                            ? candidate.output[channel][static_cast<std::size_t>(candidateIndex)]
                            : 0.0;
        const double delta = current - ref;
        peak = std::max(peak, std::abs(delta));
        energy += static_cast<long double>(delta) * delta;
    }
    return { peak, count == 0 ? 0.0 : std::sqrt(static_cast<double>(energy / count)) };
}

std::optional<double> correlation(const std::vector<float>& left,
                                  const std::vector<float>& right)
{
    const auto count = std::min(left.size(), right.size());
    if (count < 2)
        return {};
    long double leftMean = 0.0L;
    long double rightMean = 0.0L;
    for (std::size_t i = 0; i < count; ++i)
    {
        leftMean += left[i];
        rightMean += right[i];
    }
    leftMean /= count;
    rightMean /= count;

    long double covariance = 0.0L;
    long double leftEnergy = 0.0L;
    long double rightEnergy = 0.0L;
    for (std::size_t i = 0; i < count; ++i)
    {
        const long double l = static_cast<long double>(left[i]) - leftMean;
        const long double r = static_cast<long double>(right[i]) - rightMean;
        covariance += l * r;
        leftEnergy += l * l;
        rightEnergy += r * r;
    }
    if (leftEnergy == 0.0L || rightEnergy == 0.0L)
        return {};
    const auto value = static_cast<double>(
        covariance / std::sqrt(leftEnergy * rightEnergy));
    return std::isfinite(value) ? std::optional<double>(value) : std::nullopt;
}

struct FrequencyEstimate
{
    std::optional<double> frequencyHz;
    std::string ambiguity;
};

FrequencyEstimate estimateByPositiveZeroCrossings(const std::vector<float>& samples,
                                                   std::size_t begin,
                                                   std::size_t end,
                                                   double sampleRate)
{
    std::vector<double> crossings;
    if (end > samples.size())
        end = samples.size();
    if (begin >= end || end - begin < 2)
        return { {}, "window_has_fewer_than_two_samples" };

    for (std::size_t i = begin; i + 1 < end; ++i)
    {
        const double first = samples[i];
        const double second = samples[i + 1];
        if (first <= 0.0 && second > 0.0)
        {
            const double fraction = -first / (second - first);
            crossings.push_back(static_cast<double>(i) + fraction);
        }
    }
    if (crossings.size() < 2)
        return { {}, "positive_zero_crossings=" + toString(crossings.size()) };

    // Fit crossing time = free intercept + slope * crossing index using all
    // linearly interpolated positive-going crossings in this window.
    const long double count = static_cast<long double>(crossings.size());
    const long double meanIndex = (count - 1.0L) * 0.5L;
    long double meanTime = 0.0L;
    for (const double crossing : crossings)
        meanTime += crossing;
    meanTime /= count;

    long double covariance = 0.0L;
    long double indexEnergy = 0.0L;
    for (std::size_t i = 0; i < crossings.size(); ++i)
    {
        const long double centeredIndex = static_cast<long double>(i) - meanIndex;
        covariance += centeredIndex
                    * (static_cast<long double>(crossings[i]) - meanTime);
        indexEnergy += centeredIndex * centeredIndex;
    }
    if (!std::isfinite(covariance) || !std::isfinite(indexEnergy)
        || indexEnergy <= 0.0L)
        return { {}, "invalid_crossing_regression" };
    const long double slopeSamplesPerCrossing = covariance / indexEnergy;
    if (!std::isfinite(slopeSamplesPerCrossing) || slopeSamplesPerCrossing <= 0.0L)
        return { {}, "invalid_crossing_regression_slope" };

    const double frequency = sampleRate
                           / static_cast<double>(slopeSamplesPerCrossing);
    if (!std::isfinite(frequency))
        return { {}, "nonfinite_frequency_estimate" };
    return { frequency,
             "all_positive_zero_crossings_linear_regression="
                 + toString(crossings.size()) };
}

void recordMeasurement(Summary& summary,
                       CsvReport& report,
                       const std::string& fs,
                       const std::string& block,
                       const std::string& layout,
                       const std::string& signal,
                       const std::string& channel,
                       const std::string& window,
                       const RunResult& result,
                       const std::string& metric,
                       const std::optional<double>& residualPeak = {},
                       const std::optional<double>& residualRms = {},
                       const std::optional<double>& outputPeak = {},
                       const std::optional<double>& gain = {},
                       const std::optional<double>& frequencyHz = {},
                       const std::optional<double>& measuredFrequencyHz = {},
                       const std::string& measuredLatency = {},
                       const std::string& ambiguity = {},
                       const std::optional<double>& inputCorrelation = {},
                       const std::optional<double>& outputCorrelation = {},
                       const std::string& check = {},
                       const std::string& windowStart = {},
                       const std::string& windowEnd = {},
                       const std::string& windowBasis = {},
                       const std::optional<double>& inputEstimatedFrequencyHz = {},
                       const std::optional<double>& outputEstimatedFrequencyHz = {},
                       const std::optional<double>& frequencyDifferenceHz = {})
{
    ++summary.measurements;
    report.row(fs, block, layout, signal, channel, window,
               toString(result.inputFrames), toString(result.outputFrames),
               toString(result.latency), metric, residualPeak, residualRms,
               outputPeak, gain, frequencyHz, measuredFrequencyHz,
               measuredLatency, ambiguity, inputCorrelation, outputCorrelation,
               check, "MEASUREMENT_ONLY", windowStart, windowEnd, windowBasis,
               inputEstimatedFrequencyHz, outputEstimatedFrequencyHz,
               frequencyDifferenceHz);
}

void recordExactCheck(Summary& summary,
                     CsvReport& report,
                     const std::string& fs,
                     const std::string& block,
                     const std::string& layout,
                     const std::string& signal,
                     const std::string& channel,
                     const std::string& window,
                     const RunResult& result,
                     const std::string& check,
                     bool passed,
                     const std::optional<double>& residualPeak = {},
                     const std::optional<double>& residualRms = {},
                     const std::string& ambiguity = {},
                     const std::string& windowStart = {},
                     const std::string& windowEnd = {},
                     const std::string& windowBasis = {})
{
    if (passed)
        ++summary.exactPasses;
    else
        ++summary.exactFailures;
    report.row(fs, block, layout, signal, channel, window,
               toString(result.inputFrames), toString(result.outputFrames),
               toString(result.latency), "exact_check", residualPeak, residualRms,
               {}, {}, {}, {}, {}, ambiguity, {}, {}, check,
               passed ? "PASS" : "FAIL", windowStart, windowEnd, windowBasis);
}

std::pair<std::size_t, bool> maximumPosition(const std::vector<float>& samples)
{
    std::size_t position = 0;
    double maximum = -1.0;
    std::size_t exactTies = 0;
    for (std::size_t i = 0; i < samples.size(); ++i)
    {
        const double magnitude = std::abs(static_cast<double>(samples[i]));
        if (magnitude > maximum)
        {
            maximum = magnitude;
            position = i;
            exactTies = 1;
        }
        else if (magnitude == maximum)
        {
            ++exactTies;
        }
    }
    return { position, exactTies == 1 };
}

void analyzeSignal(const TestSignal& signal,
                   const RunResult& result,
                   double sampleRate,
                   std::size_t blockSize,
                   CsvReport& report,
                   Summary& summary)
{
    const std::string fs = toString(sampleRate);
    const std::string block = toString(blockSize);
    const std::string layout = layoutName(signal.input.size());
    const auto startupEnd = std::min(result.outputFrames, 4 * NeutralStft::fftSize);
    const auto tailBegin = result.outputFrames > 4 * NeutralStft::fftSize
                         ? result.outputFrames - 4 * NeutralStft::fftSize
                         : 0;

    for (std::size_t channel = 0; channel < result.output.size(); ++channel)
    {
        const std::string name = channelName(channel, result.output.size());
        const bool finite = allFinite(result, channel);
        recordExactCheck(summary, report, fs, block, layout, signal.name, name,
                         "full", result, "all_output_samples_finite", finite,
                         {}, {}, {}, "0", toString(result.outputFrames),
                         "output_sample_index");

        const std::array<std::pair<std::string, std::pair<std::size_t, std::size_t>>, 3>
            windows {{
                { "startup_first_4N", { 0, startupEnd } },
                { "tail_last_4N", { tailBegin, result.outputFrames } },
                { "full", { 0, result.outputFrames } }
            }};

        for (const auto& window : windows)
        {
            const auto metrics = compareWindow(signal, result, channel,
                                               window.second.first, window.second.second);
            summary.maxNullPeak = std::max(summary.maxNullPeak, metrics.peak);
            summary.maxNullRms = std::max(summary.maxNullRms, metrics.rms);
            recordMeasurement(summary, report, fs, block, layout, signal.name, name,
                              window.first, result, "null",
                              metrics.peak, metrics.rms, metrics.outputPeak,
                              {}, {}, {}, {}, {}, {}, {}, {},
                              toString(window.second.first),
                              toString(window.second.second),
                              "output_sample_index");
        }

        bool exactDelayedIdentity = finite;
        for (std::size_t frame = 0; frame < result.outputFrames && exactDelayedIdentity; ++frame)
        {
            const long long inputFrame = static_cast<long long>(frame)
                                       - static_cast<long long>(result.latency);
            const float reference = inputFrame >= 0
                && static_cast<std::size_t>(inputFrame) < signal.input[channel].size()
                ? signal.input[channel][static_cast<std::size_t>(inputFrame)]
                : 0.0f;
            exactDelayedIdentity = result.output[channel][frame] == reference;
        }
        recordMeasurement(summary, report, fs, block, layout, signal.name, name,
                          "full", result, "exact_delayed_identity",
                          {}, {}, {}, {}, {}, {}, {},
                          exactDelayedIdentity
                              ? "bitwise-equivalent-samples-do-not-prove-internal-STFT-path"
                              : "not-exactly-delayed-input",
                          {}, {}, {}, "0", toString(result.outputFrames),
                          "output_sample_index");

        if (signal.inactiveChannel && channel == *signal.inactiveChannel)
        {
            const bool silent = std::all_of(
                result.output[channel].begin(), result.output[channel].end(),
                [](float sample) { return sample == 0.0f; });
            recordExactCheck(summary, report, fs, block, layout, signal.name, name,
                             "full", result, "inactive_stereo_channel_exactly_zero", silent,
                             {}, {}, {}, "0", toString(result.outputFrames),
                             "output_sample_index");
        }
    }

    if (signal.input.size() == 2)
    {
        std::vector<float> delayedLeft(result.outputFrames, 0.0f);
        std::vector<float> delayedRight(result.outputFrames, 0.0f);
        for (std::size_t frame = 0; frame < result.outputFrames; ++frame)
        {
            const long long inputFrame = static_cast<long long>(frame)
                                       - static_cast<long long>(result.latency);
            if (inputFrame >= 0
                && static_cast<std::size_t>(inputFrame) < signal.input[0].size())
            {
                delayedLeft[frame] = signal.input[0][static_cast<std::size_t>(inputFrame)];
                delayedRight[frame] = signal.input[1][static_cast<std::size_t>(inputFrame)];
            }
        }
        recordMeasurement(summary, report, fs, block, layout, signal.name, "LR",
                          "full", result, "stereo_correlation",
                          {}, {}, {}, {}, {}, {}, {}, {},
                          correlation(delayedLeft, delayedRight),
                          correlation(result.output[0], result.output[1]),
                          {}, "0", toString(result.outputFrames),
                          "output_sample_index");
    }

    if (signal.latencyImpulsePosition && signal.activeChannel)
    {
        const auto active = *signal.activeChannel;
        const auto peak = maximumPosition(result.output[active]);
        const auto measured = static_cast<long long>(peak.first)
                            - static_cast<long long>(*signal.latencyImpulsePosition);
        const std::string ambiguity = peak.second
            ? "unique_exact_absolute_peak"
            : "exact_absolute_peak_tie";
        recordExactCheck(summary, report, fs, block, layout, signal.name,
                         channelName(active, result.output.size()), "full", result,
                         "impulse_peak_is_unique", peak.second,
                         {}, {}, ambiguity, "0", toString(result.outputFrames),
                         "output_sample_index");
        if (peak.second)
        {
            recordExactCheck(summary, report, fs, block, layout, signal.name,
                             channelName(active, result.output.size()), "full", result,
                             "unique_impulse_peak_matches_reported_latency",
                             measured == result.latency, {}, {}, ambiguity,
                             "0", toString(result.outputFrames),
                             "output_sample_index");
            // Keep the measured delay explicit alongside the hard exact check.
            recordMeasurement(summary, report, fs, block, layout, signal.name,
                              channelName(active, result.output.size()), "full",
                              result, "impulse_latency",
                              {}, {}, std::abs(static_cast<double>(result.output[active][peak.first])),
                              {}, {}, {}, std::to_string(measured), ambiguity,
                              {}, {}, "peak_position_minus_input_position",
                              "0", toString(result.outputFrames),
                              "output_sample_index");
        }
        else
        {
            recordMeasurement(summary, report, fs, block, layout, signal.name,
                              channelName(active, result.output.size()), "full",
                              result, "impulse_latency",
                              {}, {}, std::abs(static_cast<double>(result.output[active][peak.first])),
                              {}, {}, {}, std::to_string(measured), ambiguity,
                              {}, {}, {}, "0", toString(result.outputFrames),
                              "output_sample_index");
        }
    }

    for (const auto& tone : signal.toneWindows)
    {
        const auto alignedBegin = static_cast<long long>(tone.begin) + result.latency;
        for (std::size_t channel = 0; channel < result.output.size(); ++channel)
        {
            long long firstOffset = std::max(0LL, -alignedBegin);
            long long lastOffset = static_cast<long long>(tone.length);
            lastOffset = std::min(
                lastOffset,
                static_cast<long long>(signal.input[channel].size())
                    - static_cast<long long>(tone.begin));
            lastOffset = std::min(
                lastOffset,
                static_cast<long long>(result.outputFrames) - alignedBegin);
            lastOffset = std::max(0LL, lastOffset);
            firstOffset = std::min(firstOffset, lastOffset);

            std::vector<float> inputWindow;
            std::vector<float> outputWindow;
            inputWindow.reserve(static_cast<std::size_t>(lastOffset - firstOffset));
            outputWindow.reserve(static_cast<std::size_t>(lastOffset - firstOffset));
            long double inputEnergy = 0.0L;
            long double outputEnergy = 0.0L;
            double outputPeak = 0.0;
            for (long long offset = firstOffset; offset < lastOffset; ++offset)
            {
                const auto inputIndex = tone.begin + static_cast<std::size_t>(offset);
                const auto outputIndex = alignedBegin + offset;
                const double input = signal.input[channel][inputIndex];
                const double output = result.output[channel][static_cast<std::size_t>(outputIndex)];
                inputWindow.push_back(static_cast<float>(input));
                outputWindow.push_back(static_cast<float>(output));
                inputEnergy += static_cast<long double>(input) * input;
                outputEnergy += static_cast<long double>(output) * output;
                outputPeak = std::max(outputPeak, std::abs(output));
            }
            if (inputWindow.empty())
                continue;
            const auto valid = inputWindow.size();
            const double inputRms = std::sqrt(static_cast<double>(inputEnergy / valid));
            const double outputRms = std::sqrt(static_cast<double>(outputEnergy / valid));
            const std::optional<double> gain = inputRms > 0.0
                ? std::optional<double>(outputRms / inputRms)
                : std::nullopt;

            const auto inputFrequency = estimateByPositiveZeroCrossings(
                inputWindow, 0, inputWindow.size(), sampleRate);
            const auto outputFrequency = estimateByPositiveZeroCrossings(
                outputWindow, 0, outputWindow.size(), sampleRate);
            const std::optional<double> frequencyDifference =
                inputFrequency.frequencyHz && outputFrequency.frequencyHz
                    ? std::optional<double>(*outputFrequency.frequencyHz
                                            - *inputFrequency.frequencyHz)
                    : std::nullopt;
            const std::string frequencyAmbiguity =
                "input:" + inputFrequency.ambiguity
                + ";output:" + outputFrequency.ambiguity;
            recordMeasurement(summary, report, fs, block, layout, signal.name,
                              channelName(channel, result.output.size()),
                              "tone_last_0p5s_" + toString(tone.frequencyHz),
                              result, "tone_response",
                              {}, {}, outputPeak, gain, tone.frequencyHz,
                              outputFrequency.frequencyHz, {}, frequencyAmbiguity,
                              {}, {}, {},
                              toString(alignedBegin + firstOffset),
                              toString(alignedBegin + lastOffset),
                              "output_sample_index_latency_aligned",
                              inputFrequency.frequencyHz,
                              outputFrequency.frequencyHz,
                              frequencyDifference);
        }
    }
}

void analyzeBlockDifference(const TestSignal& signal,
                            const RunResult& reference,
                            const RunResult& candidate,
                            std::size_t blockSize,
                            double sampleRate,
                            CsvReport& report,
                            Summary& summary)
{
    const std::string fs = toString(sampleRate);
    const std::string block = toString(blockSize);
    const std::string layout = layoutName(signal.input.size());
    for (std::size_t channel = 0; channel < candidate.output.size(); ++channel)
    {
        const auto metrics = latencyAlignedDifference(reference, candidate, channel);
        summary.maxBlockDiffPeak = std::max(summary.maxBlockDiffPeak, metrics.first);
        summary.maxBlockDiffRms = std::max(summary.maxBlockDiffRms, metrics.second);
        recordMeasurement(summary, report, fs, block, layout, signal.name,
                          channelName(channel, candidate.output.size()),
                          "full_latency_aligned_vs_block_1", candidate,
                          "block_difference", metrics.first, metrics.second,
                          {}, {}, {}, {}, {}, {}, {}, {}, {},
                          toString(std::min(-static_cast<long long>(reference.latency),
                                            -static_cast<long long>(candidate.latency))),
                          toString(static_cast<long long>(candidate.inputFrames)
                                   + 4LL * static_cast<long long>(NeutralStft::fftSize)),
                          "input_sample_index_after_reported_latency_alignment");
    }
}

TestSignal makeStateProbe(double sampleRate, std::size_t channelCount)
{
    TestSignal probe = makeBlankSignal("reset_zero_block_reprepare_probe",
                                       channelCount,
                                       framesFor(1.0, sampleRate));
    const auto left = makeNoiseBurst(sampleRate, 0x6D2B79F5u, 0.1f);
    const auto right = makeNoiseBurst(sampleRate, 0x1B873593u, 0.1f);
    probe.input[0] = left;
    if (channelCount == 2)
        probe.input[1] = right;
    return probe;
}

constexpr std::size_t stateProbeFrames = NeutralStft::fftSize + NeutralStft::hopSize + 128;

TestSignal makeStateDisturbance(double sampleRate, std::size_t channelCount)
{
    TestSignal disturbance = makeBlankSignal("state_disturbance_live_overlap_no_flush",
                                             channelCount, stateProbeFrames);
    const auto left = makeNoiseBurst(sampleRate, 0xA341316Cu, 0.2f);
    const auto right = makeNoiseBurst(sampleRate, 0xC8013EA4u, 0.2f);
    std::copy_n(left.begin(), disturbance.input[0].size(), disturbance.input[0].begin());
    if (channelCount == 2)
        std::copy_n(right.begin(), disturbance.input[1].size(), disturbance.input[1].begin());
    return disturbance;
}

TestSignal makeShortProbe(double sampleRate,
                          std::size_t channelCount,
                          const std::string& name,
                          std::uint32_t leftSeed,
                          std::uint32_t rightSeed)
{
    TestSignal probe = makeBlankSignal(name, channelCount, stateProbeFrames);
    const auto left = makeNoiseBurst(sampleRate, leftSeed, 0.1f);
    std::copy_n(left.begin(), probe.input[0].size(), probe.input[0].begin());
    if (channelCount == 2)
    {
        const auto right = makeNoiseBurst(sampleRate, rightSeed, 0.1f);
        std::copy_n(right.begin(), probe.input[1].size(), probe.input[1].begin());
    }
    return probe;
}

RunResult concatenateRuns(const RunResult& first, const RunResult& second)
{
    RunResult combined;
    combined.inputFrames = first.inputFrames + second.inputFrames;
    combined.outputFrames = first.outputFrames + second.outputFrames;
    combined.latency = first.latency;
    combined.output = first.output;
    for (std::size_t channel = 0; channel < combined.output.size(); ++channel)
        combined.output[channel].insert(combined.output[channel].end(),
                                        second.output[channel].begin(),
                                        second.output[channel].end());
    return combined;
}

void checkStateSemantics(double sampleRate,
                         std::size_t channelCount,
                         std::size_t blockSize,
                         CsvReport& report,
                         Summary& summary)
{
    const std::string fs = toString(sampleRate);
    const std::string block = toString(blockSize);
    const std::string layout = layoutName(channelCount);
    const auto probe = makeStateProbe(sampleRate, channelCount);
    const auto disturbance = makeStateDisturbance(sampleRate, channelCount);

    NeutralStft processor;
    processor.prepare(sampleRate, channelCount);
    processor.reset();
    const auto first = processSignal(processor, probe, blockSize);

    processor.reset();
    (void) processSignal(processor, disturbance, blockSize, false, false);
    processor.reset();
    const auto afterReset = processSignal(processor, probe, blockSize);
    for (std::size_t channel = 0; channel < channelCount; ++channel)
    {
        const auto resetError = differenceMetrics(first.output, afterReset.output, channel);
        recordExactCheck(summary, report, fs, block, layout, probe.name,
                         channelName(channel, channelCount), "all_output",
                         afterReset, "reset_reproduces_identical_output",
                         exactlyEqualChannel(first.output[channel],
                                             afterReset.output[channel]),
                         resetError.first, resetError.second,
                         "preceded_by_" + toString(stateProbeFrames) + "frame_live_overlap_no_flush",
                         "0", toString(afterReset.outputFrames),
                         "output_sample_index");
    }

    const auto zeroMain = makeShortProbe(sampleRate, channelCount,
                                         "zero_block_main_live_overlap",
                                         0x9E3779B9u, 0x243F6A88u);
    const auto zeroContinuation = makeShortProbe(sampleRate, channelCount,
                                                 "zero_block_followup_live_overlap",
                                                 0xB7E15162u, 0x8AED2A6Bu);

    NeutralStft withoutNoops;
    withoutNoops.prepare(sampleRate, channelCount);
    withoutNoops.reset();
    const auto baselineFirst = processSignal(withoutNoops, zeroMain,
                                             blockSize, false, false, true);
    const auto baselineFollowup = processSignal(withoutNoops, zeroContinuation, blockSize);
    const auto baselineStream = concatenateRuns(baselineFirst, baselineFollowup);

    NeutralStft withNoops;
    withNoops.prepare(sampleRate, channelCount);
    withNoops.reset();
    const auto noOpFirst = processSignal(withNoops, zeroMain,
                                         blockSize, true, false, true);
    const auto noOpFollowup = processSignal(withNoops, zeroContinuation, blockSize);
    const auto noOpStream = concatenateRuns(noOpFirst, noOpFollowup);
    for (std::size_t channel = 0; channel < channelCount; ++channel)
    {
        const auto zeroError = differenceMetrics(baselineStream.output,
                                                 noOpStream.output, channel);
        recordExactCheck(summary, report, fs, block, layout,
                         zeroMain.name + "_plus_" + zeroContinuation.name,
                         channelName(channel, channelCount),
                         "zero_block_start_middle_end_and_followup",
                         noOpStream,
                         "zero_block_noop_preserves_stream_exactly",
                         exactlyEqualChannel(baselineStream.output[channel],
                                             noOpStream.output[channel]),
                         zeroError.first, zeroError.second,
                         "process(nullptr,0) at start/middle/end; follow-up follows end",
                         "0", toString(noOpStream.outputFrames),
                         "output_sample_index");
    }

    (void) processSignal(processor, disturbance, blockSize, false, false);
    processor.prepare(sampleRate, channelCount);
    const auto afterReprepare = processSignal(processor, probe, blockSize);
    NeutralStft fresh;
    fresh.prepare(sampleRate, channelCount);
    fresh.reset();
    const auto freshOutput = processSignal(fresh, probe, blockSize);
    for (std::size_t channel = 0; channel < channelCount; ++channel)
    {
        const auto reprepareError = differenceMetrics(afterReprepare.output,
                                                      freshOutput.output, channel);
        recordExactCheck(summary, report, fs, block, layout, probe.name,
                         channelName(channel, channelCount), "all_output",
                         afterReprepare, "reprepare_matches_fresh_instance",
                         exactlyEqualChannel(afterReprepare.output[channel],
                                             freshOutput.output[channel]),
                         reprepareError.first, reprepareError.second, {},
                         "0", toString(afterReprepare.outputFrames),
                         "output_sample_index");
    }

    const double sourceSampleRate = sampleRate == 44100.0 ? 48000.0 : 44100.0;
    const std::string transitionName = "rate_transition_" + toString(sourceSampleRate)
                                    + "_to_" + toString(sampleRate)
                                    + "_live_overlap_no_flush";
    const auto sourceProbe = makeShortProbe(
        sourceSampleRate, channelCount, transitionName + "_source",
        0x6A09E667u, 0x3C6EF372u);
    const auto targetProbe = makeShortProbe(
        sampleRate, channelCount, transitionName + "_target",
        0xBB67AE85u, 0xA54FF53Au);

    NeutralStft rateTransition;
    rateTransition.prepare(sourceSampleRate, channelCount);
    rateTransition.reset();
    (void) processSignal(rateTransition, sourceProbe, blockSize, false, false);
    rateTransition.prepare(sampleRate, channelCount);
    const auto afterRateTransition = processSignal(rateTransition, targetProbe, blockSize);

    NeutralStft freshTargetRate;
    freshTargetRate.prepare(sampleRate, channelCount);
    freshTargetRate.reset();
    const auto freshTargetOutput = processSignal(freshTargetRate, targetProbe, blockSize);

    for (std::size_t channel = 0; channel < channelCount; ++channel)
    {
        const auto transitionError = differenceMetrics(afterRateTransition.output,
                                                       freshTargetOutput.output,
                                                       channel);
        recordExactCheck(summary, report, fs, block, layout, transitionName,
                         channelName(channel, channelCount), "full_target_output_after_prepare",
                         afterRateTransition,
                         "sample_rate_transition_matches_fresh_target",
                         exactlyEqualChannel(afterRateTransition.output[channel],
                                             freshTargetOutput.output[channel]),
                         transitionError.first, transitionError.second, {},
                         "0", toString(afterRateTransition.outputFrames),
                         "output_sample_index");
    }
}

const char* tailModeName(TailMode mode)
{
    return mode == TailMode::tonal ? "tonal" : "noise";
}

std::vector<float> makeBandLimitedNoiseLikeBurst(double sampleRate,
                                                 std::size_t frames)
{
    // A deterministic random-phase multisine is bounded to the proposed
    // 500--1500 Hz band without relying on a platform-dependent noise filter.
    constexpr std::size_t partialCount = 49;
    constexpr double firstFrequency = 520.0;
    constexpr double frequencySpacing = 19.5;
    constexpr double targetPeak = 0.2;
    constexpr double uint32Max = 4294967295.0;
    const double twoPi = 2.0 * std::acos(-1.0);
    std::array<double, partialCount> phases {};
    std::uint32_t state = 0x7F4A7C15u;
    for (auto& phase : phases)
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        phase = twoPi * static_cast<double>(state) / uint32Max;
    }

    std::vector<float> result(frames, 0.0f);
    double peak = 0.0;
    for (std::size_t frame = 0; frame < frames; ++frame)
    {
        const double time = static_cast<double>(frame) / sampleRate;
        double value = 0.0;
        for (std::size_t partial = 0; partial < partialCount; ++partial)
        {
            const double frequency = firstFrequency
                                   + frequencySpacing * static_cast<double>(partial);
            value += std::sin(twoPi * frequency * time + phases[partial]);
        }
        result[frame] = static_cast<float>(value);
        peak = std::max(peak, std::abs(value));
    }
    if (peak > 0.0)
    {
        const double scale = targetPeak / peak;
        for (auto& sample : result)
            sample = static_cast<float>(static_cast<double>(sample) * scale);
    }
    return result;
}

TestSignal makeTailProbe(double sampleRate,
                         const std::string& kind,
                         std::size_t channelCount,
                         double burstSeconds = 0.5)
{
    constexpr double silenceSeconds = 2.4;
    constexpr double toneFrequency = 1000.0;
    constexpr double toneAmplitude = 0.2;
    const auto burstFrames = framesFor(burstSeconds, sampleRate);
    const auto silenceFrames = framesFor(silenceSeconds, sampleRate);
    TestSignal signal = makeBlankSignal("tail_" + kind + "_burst_then_2p4s_silence",
                                       channelCount, burstFrames + silenceFrames);
    std::vector<float> mono(burstFrames, 0.0f);
    if (kind == "tonal")
    {
        const double twoPi = 2.0 * std::acos(-1.0);
        for (std::size_t frame = 0; frame < burstFrames; ++frame)
        {
            const double time = static_cast<double>(frame) / sampleRate;
            mono[frame] = static_cast<float>(
                toneAmplitude * std::sin(twoPi * toneFrequency * time));
        }
    }
    else
    {
        mono = makeBandLimitedNoiseLikeBurst(sampleRate, burstFrames);
    }
    for (auto& channel : signal.input)
        std::copy(mono.begin(), mono.end(), channel.begin());
    return signal;
}

struct TailWindowMetrics
{
    double rms {};
    double peak {};
    bool finite { true };
};

TailWindowMetrics tailWindowMetrics(const RunResult& result,
                                    std::size_t channel,
                                    std::size_t inputTimelineBegin,
                                    std::size_t inputTimelineEnd)
{
    TailWindowMetrics metrics;
    const auto begin = std::min(result.outputFrames,
        inputTimelineBegin + static_cast<std::size_t>(result.latency));
    const auto end = std::min(result.outputFrames,
        inputTimelineEnd + static_cast<std::size_t>(result.latency));
    long double energy = 0.0L;
    std::size_t count = 0;
    for (std::size_t frame = begin; frame < end; ++frame)
    {
        const double sample = result.output[channel][frame];
        metrics.finite = metrics.finite && std::isfinite(sample);
        metrics.peak = std::max(metrics.peak, std::abs(sample));
        energy += static_cast<long double>(sample) * sample;
        ++count;
    }
    if (count > 0)
        metrics.rms = std::sqrt(static_cast<double>(energy / count));
    return metrics;
}

bool allZeroFromInputTimeline(const RunResult& result,
                              std::size_t channel,
                              std::size_t inputTimelineStart)
{
    const auto begin = std::min(result.outputFrames,
        inputTimelineStart + static_cast<std::size_t>(result.latency));
    return std::all_of(result.output[channel].begin()
                           + static_cast<std::ptrdiff_t>(begin),
                       result.output[channel].end(),
                       [](float sample) { return sample == 0.0f; });
}

void recordTailMeasurement(Summary& summary,
                           CsvReport& report,
                           double sampleRate,
                           std::size_t blockSize,
                           const std::string& signal,
                           const std::string& channel,
                           const RunResult& result,
                           const std::string& metric,
                           double value,
                           double peak = 0.0,
                           const std::string& window = "tail_window")
{
    ++summary.measurements;
    report.row(toString(sampleRate), toString(blockSize),
               layoutName(result.output.size()), signal, channel, window,
               toString(result.inputFrames), toString(result.outputFrames),
               toString(result.latency), metric, {}, value, peak, {}, {}, {}, {},
               "latency_aligned_input_timeline", {}, {}, {}, "MEASUREMENT_ONLY",
               {}, {}, "input_sample_index");
}

void recordTailFrequencyMeasurement(Summary& summary,
                                    CsvReport& report,
                                    double sampleRate,
                                    std::size_t blockSize,
                                    const std::string& signal,
                                    const RunResult& result,
                                    const FrequencyEstimate& estimate,
                                    std::size_t timelineBegin,
                                    std::size_t timelineEnd)
{
    ++summary.measurements;
    report.row(toString(sampleRate), toString(blockSize),
               layoutName(result.output.size()), signal, "M",
               "0p1_to_0p4s_after_burst",
               toString(result.inputFrames), toString(result.outputFrames),
               toString(result.latency), "tail_frequency_estimate_hz",
               {}, {}, {}, {}, 1000.0, estimate.frequencyHz,
               "latency_aligned_input_timeline", estimate.ambiguity,
               {}, {}, {}, "MEASUREMENT_ONLY",
               toString(timelineBegin), toString(timelineEnd),
               "positive_zero_crossing_regression_on_tail_output");
}

RunResult processTailWithSwitchOff(NeutralStft& processor,
                                  const TestSignal& signal,
                                  std::size_t blockSize,
                                  std::size_t switchPosition,
                                  TailMode mode)
{
    RunResult result;
    result.inputFrames = signal.input.empty() ? 0 : signal.input.front().size();
    result.latency = processor.latencySamples();
    result.outputFrames = result.inputFrames
                        + static_cast<std::size_t>(result.latency)
                        + 4 * NeutralStft::fftSize;
    result.output = signal.input;
    for (auto& channel : result.output)
        channel.resize(result.outputFrames, 0.0f);

    processor.setTail(1.0f, mode);
    std::array<float*, 2> channelPointers {};
    std::size_t offset = 0;
    bool switchedOff = false;
    while (offset < result.outputFrames)
    {
        if (!switchedOff && offset >= switchPosition)
        {
            processor.setTail(0.0f, mode);
            switchedOff = true;
        }
        auto count = std::min(blockSize, result.outputFrames - offset);
        if (!switchedOff)
            count = std::min(count, switchPosition - offset);
        for (std::size_t channel = 0; channel < result.output.size(); ++channel)
            channelPointers[channel] = result.output[channel].data() + offset;
        processor.process(channelPointers.data(), count);
        offset += count;
    }
    return result;
}

void checkTailExperiment(double sampleRate,
                        std::size_t blockSize,
                        CsvReport& report,
                        Summary& summary,
                        std::map<std::string, RunResult>& blockOneReference)
{
    const auto burstFrames = framesFor(0.5, sampleRate);
    const auto energyBegin = burstFrames + framesFor(0.1, sampleRate);
    const auto energyEnd = burstFrames + framesFor(1.5, sampleRate);
    const auto twoSecondEnd = burstFrames + framesFor(2.0, sampleRate);
    const auto postEndCheck = twoSecondEnd;

    for (const std::string kind : { "tonal", "noise_like" })
    {
        const auto signal = makeTailProbe(sampleRate, kind, 1);
        NeutralStft neutral;
        neutral.prepare(sampleRate, 1);
        neutral.reset();
        const auto neutralResult = processSignal(neutral, signal, blockSize);

        for (const auto mode : { TailMode::tonal, TailMode::noise })
        {
            const std::string name = std::string("tail_") + tailModeName(mode) + "_" + kind;
            NeutralStft active;
            active.prepare(sampleRate, 1);
            active.setTail(1.0f, mode);
            active.reset();
            const auto result = processSignal(active, signal, blockSize);

            recordExactCheck(summary, report, toString(sampleRate),
                             toString(blockSize), "mono", name, "M",
                             "reported_latency",
                             result, "tail_mode_does_not_change_reported_latency",
                             result.latency == neutralResult.latency,
                             {}, {}, "compared with unset/default tail path",
                             {}, {}, "reported_latency_samples");

            const auto impulsePosition = framesFor(0.25, sampleRate);
            auto impulse = makeBlankSignal("tail_latency_impulse", 1,
                                            framesFor(2.5, sampleRate));
            impulse.input[0][impulsePosition] = 0.25f;
            NeutralStft impulseProcessor;
            impulseProcessor.prepare(sampleRate, 1);
            impulseProcessor.setTail(1.0f, mode);
            const auto impulseResult = processSignal(
                impulseProcessor, impulse, blockSize);
            const auto impulsePeak = maximumPosition(impulseResult.output[0]);
            const auto measuredLatency = static_cast<long long>(impulsePeak.first)
                                       - static_cast<long long>(impulsePosition);
            recordExactCheck(summary, report, toString(sampleRate),
                             toString(blockSize), "mono", name + "_impulse",
                             "M", "full", impulseResult,
                             "tail_mode_impulse_peak_matches_reported_latency",
                             impulsePeak.second
                                 && measuredLatency == impulseResult.latency,
                             {}, {},
                             impulsePeak.second ? "unique absolute peak"
                                                : "absolute peak is not unique",
                             "0", toString(impulseResult.outputFrames),
                             "output_sample_index");

            const auto activeWindow = tailWindowMetrics(
                result, 0, energyBegin, energyEnd);
            const auto neutralWindow = tailWindowMetrics(
                neutralResult, 0, energyBegin, energyEnd);
            recordTailMeasurement(summary, report, sampleRate, blockSize, name,
                                  "M", result, "tail_window_rms",
                                  activeWindow.rms, activeWindow.peak);
            recordTailMeasurement(summary, report, sampleRate, blockSize,
                                  name + "_neutral_reference", "M",
                                  neutralResult, "tail_window_rms",
                                  neutralWindow.rms, neutralWindow.peak);
            struct TailMeasureWindow
            {
                const char* name;
                double beginSeconds;
                double endSeconds;
            };
            constexpr std::array<TailMeasureWindow, 5> tailMeasureWindows {{
                { "0p1_to_0p2s_after_burst", 0.1, 0.2 },
                { "0p2_to_0p4s_after_burst", 0.2, 0.4 },
                { "0p4_to_0p8s_after_burst", 0.4, 0.8 },
                { "0p8_to_1p5s_after_burst", 0.8, 1.5 },
                { "1p5_to_2s_after_burst", 1.5, 2.0 }
            }};
            for (const auto& window : tailMeasureWindows)
            {
                const auto begin = burstFrames + framesFor(window.beginSeconds, sampleRate);
                const auto end = burstFrames + framesFor(window.endSeconds, sampleRate);
                const auto activeMeasure = tailWindowMetrics(result, 0, begin, end);
                const auto neutralMeasure = tailWindowMetrics(
                    neutralResult, 0, begin, end);
                recordTailMeasurement(summary, report, sampleRate, blockSize, name,
                                      "M", result, "tail_window_rms",
                                      activeMeasure.rms, activeMeasure.peak,
                                      window.name);
                recordTailMeasurement(summary, report, sampleRate, blockSize,
                                      name + "_neutral_reference", "M",
                                      neutralResult, "tail_window_rms",
                                      neutralMeasure.rms, neutralMeasure.peak,
                                      window.name);
            }
            if (kind == "tonal")
            {
                const auto frequencyBegin = burstFrames + framesFor(0.1, sampleRate);
                const auto frequencyEnd = burstFrames + framesFor(0.4, sampleRate);
                const auto outputBegin = std::min(result.outputFrames,
                    frequencyBegin + static_cast<std::size_t>(result.latency));
                const auto outputEnd = std::min(result.outputFrames,
                    frequencyEnd + static_cast<std::size_t>(result.latency));
                const std::vector<float> frequencyWindow(
                    result.output[0].begin()
                        + static_cast<std::ptrdiff_t>(outputBegin),
                    result.output[0].begin()
                        + static_cast<std::ptrdiff_t>(outputEnd));
                const auto estimate = estimateByPositiveZeroCrossings(
                    frequencyWindow, 0, frequencyWindow.size(), sampleRate);
                recordTailFrequencyMeasurement(
                    summary, report, sampleRate, blockSize, name, result,
                    estimate, frequencyBegin, frequencyEnd);
            }
            recordExactCheck(summary, report, toString(sampleRate),
                             toString(blockSize), "mono", name, "M",
                             "0p1_to_1p5s_after_burst",
                             result, "tail_energy_exceeds_neutral_reference",
                             activeWindow.rms > neutralWindow.rms,
                             activeWindow.peak, activeWindow.rms,
                             "RMS values are reported; no open-ended sound-quality threshold imposed",
                             toString(energyBegin), toString(energyEnd),
                             "latency_aligned_input_sample_index");

            bool finite = allFinite(result, 0);
            recordExactCheck(summary, report, toString(sampleRate),
                             toString(blockSize), "mono", name, "M", "full",
                             result, "tail_output_is_finite", finite,
                             {}, {}, {}, "0", toString(result.outputFrames),
                             "output_sample_index");

            // The tail, including its WOLA overlap support, must already be
            // zero at exactly two seconds on the latency-aligned input axis.
            const bool ended = allZeroFromInputTimeline(result, 0, postEndCheck);
            recordExactCheck(summary, report, toString(sampleRate),
                             toString(blockSize), "mono", name, "M",
                             "from_exact_2s_cutoff",
                             result, "tail_is_zero_after_two_second_cutoff",
                             ended, {}, {}, "exactly 2 s after the end of input excitation; no FFT-support grace",
                             toString(postEndCheck), toString(result.inputFrames),
                             "latency_aligned_input_sample_index");

            const auto key = toString(sampleRate) + "_" + name;
            if (blockSize == 1)
            {
                blockOneReference.emplace(key, result);
            }
            else
            {
                const auto reference = blockOneReference.find(key);
                if (reference != blockOneReference.end())
                {
                    for (std::size_t channel = 0;
                         channel < result.output.size(); ++channel)
                    {
                        const bool same = exactlyEqualChannel(
                            reference->second.output[channel], result.output[channel]);
                        const auto diff = differenceMetrics(
                            reference->second.output, result.output, channel);
                        recordExactCheck(summary, report, toString(sampleRate),
                                         toString(blockSize), "mono", name, "M",
                                         "full_vs_block_1", result,
                                         "tail_output_block_size_invariant",
                                         same, diff.first, diff.second,
                                         "bitwise comparison; latency unchanged",
                                         "0", toString(result.outputFrames),
                                         "output_sample_index");
                    }
                }
            }

            active.reset();
            const auto repeat = processSignal(active, signal, blockSize);
            recordExactCheck(summary, report, toString(sampleRate),
                             toString(blockSize), "mono", name, "M",
                             "reset_repeat",
                             repeat, "tail_reset_reproduces_deterministic_output",
                             exactlyEqualChannel(result.output[0], repeat.output[0]),
                             {}, {}, "same fixed input and TailMode after reset",
                             "0", toString(repeat.outputFrames),
                             "output_sample_index");

            active.reset();
            const auto silence = makeBlankSignal("tail_reset_silence", 1,
                                                  framesFor(1.0, sampleRate));
            const auto silentResult = processSignal(active, silence, blockSize);
            const bool silent = std::all_of(
                silentResult.output[0].begin(), silentResult.output[0].end(),
                [](float sample) { return sample == 0.0f; });
            recordExactCheck(summary, report, toString(sampleRate),
                             toString(blockSize), "mono", name, "M",
                             "silence_after_reset",
                             silentResult, "reset_tail_does_not_emit_into_silence",
                             silent, {}, {}, {}, "0",
                             toString(silentResult.outputFrames),
                             "output_sample_index");

            NeutralStft zeroStrength;
            zeroStrength.prepare(sampleRate, 1);
            zeroStrength.setTail(0.0f, mode);
            const auto zeroStrengthResult = processSignal(
                zeroStrength, signal, blockSize);
            recordExactCheck(summary, report, toString(sampleRate),
                             toString(blockSize), "mono", name, "M",
                             "amount_zero_vs_default",
                             zeroStrengthResult,
                             "zero_tail_strength_is_neutral_path",
                             exactlyEqualChannel(neutralResult.output[0],
                                                 zeroStrengthResult.output[0]),
                             {}, {}, "bitwise comparison against unset/default tail",
                             "0", toString(zeroStrengthResult.outputFrames),
                             "output_sample_index");

            // Switching off while the burst is still active must restore the
            // neutral stream once all already-analysed WOLA frames have passed.
            const auto switchAt = burstFrames / 2;
            NeutralStft switched;
            switched.prepare(sampleRate, 1);
            switched.reset();
            const auto switchedResult = processTailWithSwitchOff(
                switched, signal, blockSize, switchAt, mode);
            const auto neutralComparableStart = switchAt
                + static_cast<std::size_t>(switchedResult.latency)
                + NeutralStft::fftSize;
            bool returnedToNeutral = true;
            for (std::size_t frame = neutralComparableStart;
                 frame < switchedResult.outputFrames; ++frame)
                returnedToNeutral = returnedToNeutral
                    && switchedResult.output[0][frame] == neutralResult.output[0][frame];
            recordExactCheck(summary, report, toString(sampleRate),
                             toString(blockSize), "mono", name, "M",
                             "after_live_switch_to_zero",
                             switchedResult, "switching_tail_off_returns_to_neutral",
                             returnedToNeutral, {}, {},
                             "comparison starts after the existing N-sample WOLA support",
                             toString(neutralComparableStart),
                             toString(switchedResult.outputFrames),
                             "output_sample_index");
        }
    }

    // Identical stereo inputs must remain coherent; a single active channel
    // must not synthesize or leak a tail into the other channel.
    for (const auto mode : { TailMode::tonal, TailMode::noise })
    {
        const std::string name = std::string("tail_stereo_") + tailModeName(mode);
        auto identical = makeTailProbe(sampleRate, "noise_like", 2);
        NeutralStft coherent;
        coherent.prepare(sampleRate, 2);
        coherent.setTail(1.0f, mode);
        const auto coherentResult = processSignal(coherent, identical, blockSize);
        const bool sameChannels = exactlyEqualChannel(coherentResult.output[0],
                                                       coherentResult.output[1]);
        recordExactCheck(summary, report, toString(sampleRate),
                         toString(blockSize), "stereo", name, "LR", "full",
                         coherentResult, "identical_stereo_inputs_remain_coherent",
                         sameChannels, {}, {},
                         "bitwise equality for identical inputs",
                         "0", toString(coherentResult.outputFrames),
                         "output_sample_index");

        auto antiPhase = makeTailProbe(sampleRate, "noise_like", 2);
        for (auto& sample : antiPhase.input[1])
            sample = -sample;
        NeutralStft antiPhaseProcessor;
        antiPhaseProcessor.prepare(sampleRate, 2);
        antiPhaseProcessor.setTail(1.0f, mode);
        const auto antiPhaseResult = processSignal(
            antiPhaseProcessor, antiPhase, blockSize);
        const bool oppositeChannels =
            antiPhaseResult.output[0].size() == antiPhaseResult.output[1].size()
            && std::equal(antiPhaseResult.output[0].begin(),
                          antiPhaseResult.output[0].end(),
                          antiPhaseResult.output[1].begin(),
                          [](float left, float right) { return left == -right; });
        recordExactCheck(summary, report, toString(sampleRate),
                         toString(blockSize), "stereo", name, "LR", "full",
                         antiPhaseResult, "opposite_stereo_inputs_remain_opposite",
                         oppositeChannels, {}, {},
                         "R = -L, tested sample-by-sample",
                         "0", toString(antiPhaseResult.outputFrames),
                         "output_sample_index");
        for (std::size_t channel = 0; channel < 2; ++channel)
        {
            recordExactCheck(summary, report, toString(sampleRate),
                             toString(blockSize), "stereo", name,
                             channelName(channel, 2), "full",
                             antiPhaseResult, "anti_phase_tail_output_is_finite",
                             allFinite(antiPhaseResult, channel),
                             {}, {}, {}, "0",
                             toString(antiPhaseResult.outputFrames),
                             "output_sample_index");
        }

        for (std::size_t channel = 0; channel < 2; ++channel)
        {
            recordExactCheck(summary, report, toString(sampleRate),
                             toString(blockSize), "stereo", name,
                             channelName(channel, 2), "full",
                             coherentResult, "tail_stereo_output_is_finite",
                             allFinite(coherentResult, channel),
                             {}, {}, {}, "0",
                             toString(coherentResult.outputFrames),
                             "output_sample_index");
        }
        for (std::size_t activeChannel = 0; activeChannel < 2; ++activeChannel)
        {
            auto oneSided = makeTailProbe(sampleRate, "noise_like", 2);
            std::fill(oneSided.input[1 - activeChannel].begin(),
                      oneSided.input[1 - activeChannel].end(), 0.0f);
            NeutralStft isolated;
            isolated.prepare(sampleRate, 2);
            isolated.setTail(1.0f, mode);
            const auto isolatedResult = processSignal(isolated, oneSided, blockSize);
            const auto inactive = 1 - activeChannel;
            const bool noLeak = std::all_of(
                isolatedResult.output[inactive].begin(),
                isolatedResult.output[inactive].end(),
                [](float sample) { return sample == 0.0f; });
            recordExactCheck(summary, report, toString(sampleRate),
                             toString(blockSize), "stereo", name,
                             activeChannel == 0 ? "L_only" : "R_only", "full",
                             isolatedResult, "inactive_channel_has_no_tail_or_crosstalk",
                             noLeak, {}, {},
                             activeChannel == 0 ? "right output exactly zero"
                                                : "left output exactly zero",
                             "0", toString(isolatedResult.outputFrames),
                             "output_sample_index");
            recordExactCheck(summary, report, toString(sampleRate),
                             toString(blockSize), "stereo", name,
                             activeChannel == 0 ? "L_only" : "R_only", "full",
                             isolatedResult, "one_sided_tail_output_is_finite",
                             allFinite(isolatedResult, activeChannel),
                             {}, {}, {}, "0",
                             toString(isolatedResult.outputFrames),
                             "output_sample_index");
        }
    }
}

void checkShortTail(double sampleRate, CsvReport& report, Summary& summary)
{
    constexpr std::size_t blockSize = 257;
    constexpr double burstSeconds = 0.01;
    const auto begin = framesFor(burstSeconds + 0.1, sampleRate);
    const auto end = framesFor(burstSeconds + 0.2, sampleRate);
    for (const auto& test : {std::pair<const char*, TailMode>{"tonal", TailMode::tonal},
                             {"noise_like", TailMode::noise}})
    {
        const auto signal = makeTailProbe(sampleRate, test.first, 1, burstSeconds);
        NeutralStft neutral;
        neutral.prepare(sampleRate, 1);
        const auto reference = processSignal(neutral, signal, blockSize);
        NeutralStft active;
        active.prepare(sampleRate, 1);
        active.setTail(1.0f, test.second);
        const auto result = processSignal(active, signal, blockSize);
        const auto measured = tailWindowMetrics(result, 0, begin, end);
        const auto neutralMeasured = tailWindowMetrics(reference, 0, begin, end);
        const std::string name = std::string("short_tail_") + test.first;
        recordTailMeasurement(summary, report, sampleRate, blockSize, name, "M",
                              result, "tail_window_rms", measured.rms, measured.peak,
                              "0p1_to_0p2s_after_burst");
        recordExactCheck(summary, report, toString(sampleRate), toString(blockSize),
                         "mono", name, "M", "0p1_to_0p2s_after_burst", result,
                         "short_burst_tail_exceeds_neutral",
                         measured.finite && measured.rms > neutralMeasured.rms,
                         measured.peak, measured.rms,
                         "10 ms burst; neutral reference; sound-quality threshold open",
                         toString(begin), toString(end),
                         "latency_aligned_input_sample_index");
    }
}

struct TailBand
{
    const char* name;
    double lowHz;
    double highHz;
};

constexpr TailBand defaultTailBand { "default_500_1500Hz", 500.0, 1500.0 };
constexpr TailBand alternateTailBand { "alternate_2000_3000Hz", 2000.0, 3000.0 };
constexpr TailBand lowEdgeTailBand { "lower_edge_40_500Hz", 40.0, 500.0 };
constexpr TailBand highEdgeTailBand { "upper_edge_15000_16000Hz", 15000.0, 16000.0 };

void setTailForBand(NeutralStft& processor,
                    float amount,
                    TailMode mode,
                    const TailBand& band,
                    bool reverseBounds = false)
{
    if (!reverseBounds && band.lowHz == defaultTailBand.lowHz
        && band.highHz == defaultTailBand.highHz)
    {
        // Exercise the documented defaults as well as explicit frequency ranges.
        processor.setTail(amount, mode);
        return;
    }

    processor.setTail(amount, mode,
                      static_cast<float>(reverseBounds ? band.highHz : band.lowHz),
                      static_cast<float>(reverseBounds ? band.lowHz : band.highHz));
}

TestSignal makeExponentialBandProbe(double sampleRate,
                                    const TailBand& band,
                                    const std::string& kind,
                                    std::size_t channelCount,
                                    double totalSeconds,
                                    double sourceSeconds,
                                    bool includeForeignTone)
{
    constexpr double decaySeconds = 0.12;
    constexpr double sourcePeak = 0.25;
    constexpr double foreignFrequencyHz = 8000.0;
    constexpr double foreignAmplitude = 0.1;
    const auto totalFrames = framesFor(totalSeconds, sampleRate);
    const auto sourceFrames = std::min(framesFor(sourceSeconds, sampleRate), totalFrames);
    TestSignal signal = makeBlankSignal(
        "exponential_" + kind + "_in_" + band.name
            + (includeForeignTone ? "_with_continuous_8kHz_foreign_tone" : ""),
        channelCount, totalFrames);

    std::vector<double> frequencies;
    std::vector<double> phaseSin;
    std::vector<double> phaseCos;
    std::vector<double> rotationSin;
    std::vector<double> rotationCos;

    if (kind == "tonal")
    {
        const double center = 0.5 * (band.lowHz + band.highHz);
        const double frequency = std::round(center / 4.0) * 4.0;
        frequencies.push_back(frequency);
        phaseSin.push_back(std::sin(0.37));
        phaseCos.push_back(std::cos(0.37));
    }
    else
    {
        // Fixed-seed random-phase partials on a 4-Hz grid form a reproducible,
        // noise-like signal wholly within the selected band.
        constexpr double partialSpacingHz = 16.0;
        const double marginHz = std::max(16.0, 0.1 * (band.highHz - band.lowHz));
        const double first = std::ceil((band.lowHz + marginHz) / partialSpacingHz)
                           * partialSpacingHz;
        const double last = band.highHz - marginHz;
        std::uint32_t state = 0xA341316Cu;
        constexpr double uint32Max = 4294967295.0;
        for (double frequency = first; frequency <= last;
             frequency += partialSpacingHz)
        {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            const double phase = 2.0 * std::acos(-1.0)
                               * static_cast<double>(state) / uint32Max;
            frequencies.push_back(frequency);
            phaseSin.push_back(std::sin(phase));
            phaseCos.push_back(std::cos(phase));
        }
    }

    const double twoPi = 2.0 * std::acos(-1.0);
    for (const double frequency : frequencies)
    {
        const double phaseAdvance = twoPi * frequency / sampleRate;
        rotationSin.push_back(std::sin(phaseAdvance));
        rotationCos.push_back(std::cos(phaseAdvance));
    }

    std::vector<double> source(sourceFrames, 0.0);
    double sourceMaximum = 0.0;
    for (std::size_t frame = 0; frame < sourceFrames; ++frame)
    {
        double value = 0.0;
        for (std::size_t partial = 0; partial < frequencies.size(); ++partial)
        {
            value += phaseSin[partial];
            const double nextSin = phaseSin[partial] * rotationCos[partial]
                                 + phaseCos[partial] * rotationSin[partial];
            const double nextCos = phaseCos[partial] * rotationCos[partial]
                                 - phaseSin[partial] * rotationSin[partial];
            phaseSin[partial] = nextSin;
            phaseCos[partial] = nextCos;
        }
        source[frame] = value;
        sourceMaximum = std::max(sourceMaximum, std::abs(value));
    }

    const double sourceScale = sourceMaximum > 0.0 ? sourcePeak / sourceMaximum : 0.0;
    double foreignSin = std::sin(0.19);
    double foreignCos = std::cos(0.19);
    const double foreignPhaseAdvance = twoPi * foreignFrequencyHz / sampleRate;
    const double foreignRotationSin = std::sin(foreignPhaseAdvance);
    const double foreignRotationCos = std::cos(foreignPhaseAdvance);

    for (std::size_t frame = 0; frame < totalFrames; ++frame)
    {
        const double time = static_cast<double>(frame) / sampleRate;
        double value = 0.0;
        if (frame < sourceFrames)
            value = source[frame] * sourceScale * std::exp(-time / decaySeconds);

        if (includeForeignTone)
        {
            value += foreignAmplitude * foreignSin;
            const double nextSin = foreignSin * foreignRotationCos
                                 + foreignCos * foreignRotationSin;
            const double nextCos = foreignCos * foreignRotationCos
                                 - foreignSin * foreignRotationSin;
            foreignSin = nextSin;
            foreignCos = nextCos;
        }

        for (auto& channel : signal.input)
            channel[frame] = static_cast<float>(value);
    }
    return signal;
}

double goertzelPower(const std::vector<float>& samples,
                     std::size_t begin,
                     std::size_t end,
                     double frequencyHz,
                     double sampleRate)
{
    const double twoPi = 2.0 * std::acos(-1.0);
    const double coefficient = 2.0 * std::cos(twoPi * frequencyHz / sampleRate);
    double previous = 0.0;
    double previousPrevious = 0.0;
    for (std::size_t frame = begin; frame < end; ++frame)
    {
        const double current = static_cast<double>(samples[frame])
                             + coefficient * previous - previousPrevious;
        previousPrevious = previous;
        previous = current;
    }

    const double unnormalised = previous * previous
                              + previousPrevious * previousPrevious
                              - coefficient * previous * previousPrevious;
    const double count = static_cast<double>(end - begin);
    return std::max(0.0, unnormalised) * 4.0 / (count * count);
}

double bandSpectralPower(const RunResult& result,
                         std::size_t channel,
                         double sampleRate,
                         double lowHz,
                         double highHz,
                         std::size_t inputTimelineBegin,
                         std::size_t inputTimelineEnd)
{
    const auto begin = inputTimelineBegin + static_cast<std::size_t>(result.latency);
    const auto end = inputTimelineEnd + static_cast<std::size_t>(result.latency);
    const double binSpacing = sampleRate / static_cast<double>(end - begin);
    const auto firstBin = static_cast<std::size_t>(std::ceil(lowHz / binSpacing));
    const auto lastBin = static_cast<std::size_t>(std::floor(highHz / binSpacing));
    long double power = 0.0L;
    for (std::size_t bin = firstBin; bin <= lastBin; ++bin)
    {
        const double frequency = static_cast<double>(bin) * binSpacing;
        power += goertzelPower(result.output[channel], begin, end,
                               frequency, sampleRate);
    }
    return static_cast<double>(power);
}

void checkFrequencySelectiveBand(double sampleRate,
                                 std::size_t blockSize,
                                 const TailBand& band,
                                 bool checkReversedBounds,
                                 bool checkAmountZero,
                                 bool checkTwoSecondLimit,
                                 CsvReport& report,
                                 Summary& summary)
{
    const auto measurementBegin = framesFor(0.85, sampleRate);
    const auto measurementEnd = framesFor(1.10, sampleRate);
    constexpr double foreignFrequencyHz = 8000.0;

    for (const std::string kind : { "tonal", "noise_like" })
    {
        const auto signal = makeExponentialBandProbe(
            sampleRate, band, kind, 1, 1.6, 1.6, true);
        NeutralStft neutral;
        neutral.prepare(sampleRate, 1);
        neutral.reset();
        const auto neutralResult = processSignal(neutral, signal, blockSize);
        const auto neutralBandPower = bandSpectralPower(
            neutralResult, 0, sampleRate, band.lowHz, band.highHz,
            measurementBegin, measurementEnd);
        const auto neutralForeignPower = goertzelPower(
            neutralResult.output[0],
            measurementBegin + static_cast<std::size_t>(neutralResult.latency),
            measurementEnd + static_cast<std::size_t>(neutralResult.latency),
            foreignFrequencyHz, sampleRate);

        for (const auto mode : { TailMode::tonal, TailMode::noise })
        {
            const std::string name = std::string("selective_") + band.name + "_"
                                   + kind + "_" + tailModeName(mode);
            NeutralStft active;
            active.prepare(sampleRate, 1);
            setTailForBand(active, 1.0f, mode, band);
            active.reset();
            const auto result = processSignal(active, signal, blockSize);
            const auto activeBandPower = bandSpectralPower(
                result, 0, sampleRate, band.lowHz, band.highHz,
                measurementBegin, measurementEnd);
            const auto activeForeignPower = goertzelPower(
                result.output[0],
                measurementBegin + static_cast<std::size_t>(result.latency),
                measurementEnd + static_cast<std::size_t>(result.latency),
                foreignFrequencyHz, sampleRate);
            const auto difference = differenceMetrics(
                result.output, neutralResult.output, 0);
            const double selectedBandGain = neutralBandPower > 0.0
                ? std::sqrt(activeBandPower / neutralBandPower) : 0.0;
            const double foreignToneGain = neutralForeignPower > 0.0
                ? std::sqrt(activeForeignPower / neutralForeignPower)
                : std::numeric_limits<double>::infinity();

            recordTailMeasurement(summary, report, sampleRate, blockSize, name,
                                  "M", neutralResult, "neutral_selected_band_rms",
                                  std::sqrt(neutralBandPower), 0.0,
                                  "0p85_to_1p10s_while_8kHz_tone_continues");
            recordTailMeasurement(summary, report, sampleRate, blockSize, name,
                                  "M", result, "active_selected_band_rms",
                                  std::sqrt(activeBandPower), 0.0,
                                  "0p85_to_1p10s_while_8kHz_tone_continues");
            recordTailMeasurement(summary, report, sampleRate, blockSize, name,
                                  "M", result, "selected_band_power_gain",
                                  selectedBandGain, 0.0, "active_over_neutral");
            recordTailMeasurement(summary, report, sampleRate, blockSize, name,
                                  "M", result, "foreign_8kHz_power_gain",
                                  foreignToneGain, 0.0, "active_over_neutral");

            recordExactCheck(summary, report, toString(sampleRate),
                             toString(blockSize), "mono", name, "M",
                             "0p85_to_1p10s", result,
                             "selected_band_power_increases_during_natural_decay",
                             activeBandPower > neutralBandPower,
                             std::sqrt(activeBandPower) - std::sqrt(neutralBandPower),
                             selectedBandGain,
                             "strict directional comparison; no sound-quality threshold",
                             toString(measurementBegin), toString(measurementEnd),
                             "latency-aligned input timeline; Goertzel-bin band power");
            recordExactCheck(summary, report, toString(sampleRate),
                             toString(blockSize), "mono", name, "M",
                             "0p85_to_1p10s", result,
                             "selected_band_gain_exceeds_continuing_foreign_tone_gain",
                             selectedBandGain > foreignToneGain,
                             selectedBandGain, foreignToneGain,
                             "relative in-band vs 8-kHz Goertzel power; no absolute tolerance",
                             toString(measurementBegin), toString(measurementEnd),
                             "latency-aligned input timeline");
            recordExactCheck(summary, report, toString(sampleRate),
                             toString(blockSize), "mono", name, "M",
                             "full", result, "active_output_differs_from_neutral",
                             difference.first > 0.0,
                             difference.first, difference.second,
                             "same input and reported latency",
                             "0", toString(result.outputFrames),
                             "output_sample_index");
            recordExactCheck(summary, report, toString(sampleRate),
                             toString(blockSize), "mono", name, "M",
                             "full", result, "variable_range_output_is_finite",
                             allFinite(result, 0),
                             {}, {}, {}, "0", toString(result.outputFrames),
                             "output_sample_index");

            if (checkReversedBounds && kind == "tonal"
                && mode == TailMode::tonal)
            {
                NeutralStft reversed;
                reversed.prepare(sampleRate, 1);
                setTailForBand(reversed, 1.0f, mode, band, true);
                reversed.reset();
                const auto reversedResult = processSignal(reversed, signal, blockSize);
                recordExactCheck(summary, report, toString(sampleRate),
                                 toString(blockSize), "mono", name, "M",
                                 "sorted_vs_reversed_bounds", reversedResult,
                                 "reversed_frequency_bounds_match_sorted_bounds",
                                 exactlyEqualChannel(result.output[0],
                                                     reversedResult.output[0]),
                                 {}, {}, "same [low, high] range after ordering",
                                 "0", toString(result.outputFrames),
                                 "output_sample_index");
            }

            if (checkAmountZero && kind == "tonal"
                && mode == TailMode::tonal)
            {
                NeutralStft zeroAmount;
                zeroAmount.prepare(sampleRate, 1);
                setTailForBand(zeroAmount, 0.0f, mode, band);
                zeroAmount.reset();
                const auto zeroResult = processSignal(
                    zeroAmount, signal, blockSize);
                recordExactCheck(summary, report, toString(sampleRate),
                                 toString(blockSize), "mono", name, "M",
                                 "amount_zero", zeroResult,
                                 "amount_zero_is_bitwise_neutral_for_selected_range",
                                 exactlyEqualChannel(neutralResult.output[0],
                                                     zeroResult.output[0]),
                                 {}, {}, "compared with unset/default neutral processor",
                                 "0", toString(zeroResult.outputFrames),
                                 "output_sample_index");
            }
        }
    }

    // A configured band with no in-band input must not emit synthesized audio.
    for (const auto mode : { TailMode::tonal, TailMode::noise })
    {
        const auto silence = makeBlankSignal(
            "variable_range_zero_input_" + std::string(band.name),
            1, framesFor(0.75, sampleRate));
        NeutralStft silentProcessor;
        silentProcessor.prepare(sampleRate, 1);
        setTailForBand(silentProcessor, 1.0f, mode, band);
        silentProcessor.reset();
        const auto silentResult = processSignal(
            silentProcessor, silence, blockSize);
        const bool allZero = std::all_of(
            silentResult.output[0].begin(), silentResult.output[0].end(),
            [](float sample) { return sample == 0.0f; });
        recordExactCheck(summary, report, toString(sampleRate),
                         toString(blockSize), "mono", silence.name, "M",
                         "full", silentResult,
                         "zero_input_emits_no_tail_and_remains_finite",
                         allZero && allFinite(silentResult, 0),
                         {}, {}, "nonzero amount and selected frequency range",
                         "0", toString(silentResult.outputFrames),
                         "output_sample_index");
    }

    if (checkTwoSecondLimit)
    {
        for (const auto mode : { TailMode::tonal, TailMode::noise })
        {
            const auto burst = makeExponentialBandProbe(
                sampleRate, band, "tonal", 1, 2.7, 0.5, false);
            NeutralStft bounded;
            bounded.prepare(sampleRate, 1);
            setTailForBand(bounded, 1.0f, mode, band);
            bounded.reset();
            const auto boundedResult = processSignal(bounded, burst, blockSize);
            const auto cutoffTimeline = framesFor(0.5, sampleRate)
                                      + framesFor(2.0, sampleRate);
            const bool ended = allZeroFromInputTimeline(
                boundedResult, 0, cutoffTimeline);
            recordExactCheck(summary, report, toString(sampleRate),
                             toString(blockSize), "mono", burst.name, "M",
                             "from_two_second_cutoff", boundedResult,
                             "selected_range_tail_is_zero_after_two_seconds",
                             ended && allFinite(boundedResult, 0),
                             {}, {}, "exponential excitation ends at 0.5 s; selected range",
                             toString(cutoffTimeline),
                             toString(boundedResult.inputFrames),
                             "latency-aligned input timeline");
        }
    }
}

void checkVariableRangeStereo(double sampleRate,
                              std::size_t blockSize,
                              CsvReport& report,
                              Summary& summary)
{
    for (const auto& test : {
             std::pair<const char*, TailMode>{"tonal", TailMode::tonal},
             {"noise_like", TailMode::noise}})
    {
        const auto signal = makeExponentialBandProbe(
            sampleRate, alternateTailBand, test.first, 2, 1.6, 1.6, true);
        NeutralStft stereo;
        stereo.prepare(sampleRate, 2);
        setTailForBand(stereo, 1.0f, test.second, alternateTailBand);
        stereo.reset();
        const auto result = processSignal(stereo, signal, blockSize);
        const bool coherent = exactlyEqualChannel(result.output[0], result.output[1]);
        recordExactCheck(summary, report, toString(sampleRate),
                         toString(blockSize), "stereo", signal.name, "LR",
                         "full", result,
                         "identical_inputs_remain_coherent_with_variable_range",
                         coherent && allFinite(result, 0) && allFinite(result, 1),
                         {}, {}, "equal channels; explicit 2000-3000-Hz range",
                         "0", toString(result.outputFrames),
                         "output_sample_index");
    }

    const auto burstLength = 48 * NeutralStft::hopSize;
    auto asymmetrical = makeBlankSignal("stereo_distinct_with_equal_hop_boundary",
                                        2, burstLength + framesFor(0.5, sampleRate));
    for (std::size_t frame = 0; frame < burstLength; ++frame)
    {
        const auto sample = static_cast<float>(
            0.2 * std::sin(2.0 * std::acos(-1.0) * 1000.0 * frame / sampleRate));
        asymmetrical.input[0][frame] = sample;
        asymmetrical.input[1][frame] = frame % NeutralStft::hopSize == NeutralStft::hopSize - 1
                                          ? sample : 0.5f * sample;
    }
    for (const auto mode : {TailMode::tonal, TailMode::noise})
    {
        NeutralStft stereo;
        stereo.prepare(sampleRate, 2);
        stereo.setTail(1.0f, mode);
        const auto result = processSignal(stereo, asymmetrical, blockSize);
        const auto begin = burstLength + framesFor(0.1, sampleRate);
        const auto end = burstLength + framesFor(0.2, sampleRate);
        const auto left = tailWindowMetrics(result, 0, begin, end);
        const auto right = tailWindowMetrics(result, 1, begin, end);
        recordExactCheck(summary, report, toString(sampleRate), toString(blockSize),
                         "stereo", asymmetrical.name, "LR", "0p1_to_0p2s_after_burst",
                         result, "distinct_channels_not_collapsed_by_equal_hop_sample",
                         left.finite && right.finite && left.rms > 1.4 * right.rms
                             && right.rms > 0.0,
                         left.peak, left.rms,
                         "L is twice R except at individual hop-boundary samples; mode="
                             + std::string(tailModeName(mode)),
                         toString(begin), toString(end), "latency-aligned input timeline");
    }
}

std::optional<std::string> reportPath(int argc, char** argv)
{
    std::string path = "offline-metrics.csv";
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) != "--report")
            return {};
        if (i + 1 >= argc)
            return {};
        path = argv[++i];
        if (path.empty())
            return {};
    }
    return path;
}

} // namespace

int main(int argc, char** argv)
{
    const auto path = reportPath(argc, argv);
    if (!path)
    {
        std::cerr << "Usage: spectral_decay_offline_tests [--report <csv-path>]\n";
        return 2;
    }

    std::ofstream file(*path, std::ios::out | std::ios::trunc);
    if (!file)
    {
        std::cerr << "Cannot open metrics report: " << *path << '\n';
        return 2;
    }
    CsvReport report(file);
    Summary summary;

    try
    {
        for (const double sampleRate : sampleRates)
        {
            checkShortTail(sampleRate, report, summary);
            for (const std::size_t channelCount : { std::size_t(1), std::size_t(2) })
            {
                std::map<std::string, RunResult> blockOneReference;
                std::map<std::string, RunResult> tailBlockOneReference;
                for (const std::size_t blockSize : blockSizes)
                {
                    auto signals = makeSignals(sampleRate, channelCount, blockSize);
                    NeutralStft processor;
                    processor.prepare(sampleRate, channelCount);

                    for (const auto& signal : signals)
                    {
                        processor.reset();
                        const auto result = processSignal(processor, signal, blockSize);
                        analyzeSignal(signal, result, sampleRate, blockSize, report, summary);

                        if (blockSize == 1)
                        {
                            if (signal.compareAcrossBlocks)
                                blockOneReference.emplace(signal.name, result);
                        }
                        else if (signal.compareAcrossBlocks)
                        {
                            const auto reference = blockOneReference.find(signal.name);
                            if (reference != blockOneReference.end())
                                analyzeBlockDifference(signal, reference->second, result,
                                                       blockSize, sampleRate, report, summary);
                        }
                    }

                    checkStateSemantics(sampleRate, channelCount, blockSize,
                                        report, summary);

                    if (channelCount == 1)
                    {
                        checkTailExperiment(sampleRate, blockSize, report, summary,
                                            tailBlockOneReference);
                        checkFrequencySelectiveBand(
                            sampleRate, blockSize, defaultTailBand,
                            false, true, false, report, summary);
                        checkFrequencySelectiveBand(
                            sampleRate, blockSize, alternateTailBand,
                            true, true, true, report, summary);
                        checkVariableRangeStereo(
                            sampleRate, blockSize, report, summary);
                        if (blockSize == 257)
                        {
                            checkFrequencySelectiveBand(
                                sampleRate, blockSize, lowEdgeTailBand,
                                false, true, false, report, summary);
                            checkFrequencySelectiveBand(
                                sampleRate, blockSize, highEdgeTailBand,
                                false, true, false, report, summary);
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Offline test harness error: " << exception.what() << '\n';
        return 2;
    }

    file.flush();
    if (!report.good())
    {
        std::cerr << "Failed while writing metrics report: " << *path << '\n';
        return 2;
    }

    std::cout << std::setprecision(10)
              << "Offline measurements written to " << *path << '\n'
              << "Exact structural checks: " << summary.exactPasses << " passed, "
              << summary.exactFailures << " failed.\n"
              << "Measurement-only rows: " << summary.measurements
              << " (null/frequency/block tolerances remain open; these rows are not passes).\n"
              << "Maximum null residual (linear FS): peak=" << summary.maxNullPeak
              << ", RMS=" << summary.maxNullRms << '\n'
              << "Maximum block-size difference (linear FS, latency-aligned): peak="
              << summary.maxBlockDiffPeak << ", RMS=" << summary.maxBlockDiffRms << '\n'
              << "Allocation instrumentation: not enabled; C++ and C-library allocations "
                 "inside process() are not claimed absent.\n";

    return summary.exactFailures == 0 ? 0 : 1;
}
