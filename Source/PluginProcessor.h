#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <vector>

// =============================================================================
// Schroeder delay-based allpass section
//
// Transfer function: H(z) = (g + z^-N) / (1 + g*z^-N)
// State equation:    v[n] = x[n] - g*v[n-N]
//                    y[n] = g*v[n] + v[n-N]
//
// Used in the spring tank to provide dense, diffuse reflections.
// =============================================================================
struct AllpassSection
{
    std::vector<float> buf;
    int writePos { 0 };
    int maxSize  { 0 };

    void prepare (int maxDelaySamples)
    {
        maxSize = maxDelaySamples + 4;   // headroom for interpolation
        buf.assign ((size_t) maxSize, 0.0f);
        writePos = 0;
    }

    // Fixed integer delay allpass
    float process (float input, int delaySamples, float g) noexcept
    {
        delaySamples = juce::jlimit (1, maxSize - 2, delaySamples);
        const int readPos = (writePos - delaySamples + maxSize) % maxSize;
        const float vDelayed = buf[(size_t) readPos];
        const float v = input - g * vDelayed;
        buf[(size_t) writePos] = v;
        writePos = (writePos + 1) % maxSize;
        return g * v + vDelayed;
    }

    // Linear-interpolated allpass (for LFO modulation — avoids clicks)
    float processInterp (float input, float delaySamples, float g) noexcept
    {
        delaySamples = juce::jlimit (1.0f, (float)(maxSize - 3), delaySamples);
        const int   intD = (int) delaySamples;
        const float frac = delaySamples - (float) intD;

        const int r0 = (writePos - intD     + maxSize) % maxSize;
        const int r1 = (writePos - intD - 1 + maxSize) % maxSize;
        const float vDelayed = buf[(size_t) r0] * (1.0f - frac)
                             + buf[(size_t) r1] * frac;

        const float v = input - g * vDelayed;
        buf[(size_t) writePos] = v;
        writePos = (writePos + 1) % maxSize;
        return g * v + vDelayed;
    }

    void reset() noexcept
    {
        std::fill (buf.begin(), buf.end(), 0.0f);
        writePos = 0;
    }
};

// =============================================================================
// Simple mono circular pre-delay buffer
// =============================================================================
struct PreDelayBuffer
{
    std::vector<float> buf;
    int writePos { 0 };
    int maxSize  { 0 };

    void prepare (int maxDelaySamples)
    {
        maxSize = maxDelaySamples + 2;
        buf.assign ((size_t) maxSize, 0.0f);
        writePos = 0;
    }

    void write (float sample) noexcept
    {
        buf[(size_t) writePos] = sample;
        writePos = (writePos + 1) % maxSize;
    }

    float read (int delaySamples) const noexcept
    {
        delaySamples = juce::jlimit (0, maxSize - 1, delaySamples);
        const int readPos = (writePos - 1 - delaySamples + maxSize) % maxSize;
        return buf[(size_t) readPos];
    }

    void reset() noexcept
    {
        std::fill (buf.begin(), buf.end(), 0.0f);
        writePos = 0;
    }
};

// =============================================================================
// MyReverbAudioProcessor — Deep House Spring Reverb
//
// Signal chain (per channel):
//   Input → PreDelay → Drive (tanh) → Spring Tank → Mix Blend → Output
//
// Spring Tank (2 parallel strings, String A = left, String B = right):
//   Input + Feedback → AP1 → AP2 → AP3[LFO] → Output
//                                            → LP(damping) → × fbGain → Feedback
// =============================================================================
class MyReverbAudioProcessor : public juce::AudioProcessor
{
public:
    MyReverbAudioProcessor();
    ~MyReverbAudioProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override  { return JucePlugin_Name; }
    bool acceptsMidi()  const override           { return false; }
    bool producesMidi() const override           { return false; }
    bool isMidiEffect() const override           { return false; }
    double getTailLengthSeconds() const override { return 8.0; }  // max decay

    int getNumPrograms() override                             { return 1; }
    int getCurrentProgram() override                          { return 0; }
    void setCurrentProgram (int) override                     {}
    const juce::String getProgramName (int) override          { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==========================================================================
    // Runtime state
    double currentSampleRate { 44100.0 };

    // ─── Pre-delay (one buffer per channel, max 100 ms) ───────────────────────
    std::array<PreDelayBuffer, 2> preDelay;

    // ─── Spring tank: String A (left) ─────────────────────────────────────────
    std::array<AllpassSection, 3> apA;
    juce::dsp::FirstOrderTPTFilter<float> dampA;   // LP filter in feedback path
    float feedbackA { 0.0f };

    // ─── Spring tank: String B (right, +2 ms offset for decorrelation) ────────
    std::array<AllpassSection, 3> apB;
    juce::dsp::FirstOrderTPTFilter<float> dampB;
    float feedbackB { 0.0f };

    // ─── LFO (one per string, rates slightly detuned) ─────────────────────────
    float lfoPhaseA  { 0.0f };
    float lfoPhaseB  { 0.0f };
    static constexpr float LFO_RATE_A = 0.50f;   // Hz
    static constexpr float LFO_RATE_B = 0.71f;   // Hz

    // ─── Allpass delay lengths (samples, computed in prepareToPlay) ───────────
    // String A: ~5 ms, ~9 ms, ~14 ms
    // String B: ~7 ms, ~11 ms, ~16 ms  (+2 ms offset)
    int apDelayA[3] { 220, 397, 617 };
    int apDelayB[3] { 308, 485, 705 };

    // Max LFO wobble depth in samples (= 3 ms at current sample rate)
    float maxWobbleSamples { 132.0f };

    // ─── Parameter smoothers (10 ms ramp, prevents zipper noise) ─────────────
    juce::LinearSmoothedValue<float> smoothMix;
    juce::LinearSmoothedValue<float> smoothDrive;

    // ─── Helpers ──────────────────────────────────────────────────────────────
    static float applyDrive (float x, float driveGain) noexcept;
    void resetSpringTank() noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MyReverbAudioProcessor)
};
