#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.hpp"

// =============================================================================
// Parameter Layout
// =============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
NFReverbAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        ParameterIDs::mix, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.5f, juce::AudioParameterFloatAttributes{}.withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        ParameterIDs::decay, "Decay",
        juce::NormalisableRange<float> (0.1f, 8.0f, 0.01f, 0.4f),   // skewed for fine control at short values
        2.0f, juce::AudioParameterFloatAttributes{}.withLabel ("s")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        ParameterIDs::tension, "Tension",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        ParameterIDs::pre_delay, "Pre-Delay",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        10.0f, juce::AudioParameterFloatAttributes{}.withLabel ("ms")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        ParameterIDs::damping, "Damping",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.4f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        ParameterIDs::wobble, "Wobble",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.3f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        ParameterIDs::drive, "Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.2f));

    return { params.begin(), params.end() };
}

// =============================================================================
// Constructor / Destructor
// =============================================================================
NFReverbAudioProcessor::NFReverbAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
#else
    :
#endif
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

NFReverbAudioProcessor::~NFReverbAudioProcessor() {}

// =============================================================================
// prepareToPlay
// =============================================================================
void NFReverbAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // ─── Compute allpass delay lengths from sample rate ────────────────────
    // String A: 5 ms, 9 ms, 14 ms
    apDelayA[0] = (int) (0.005 * sampleRate);
    apDelayA[1] = (int) (0.009 * sampleRate);
    apDelayA[2] = (int) (0.014 * sampleRate);

    // String B: 7 ms, 11 ms, 16 ms  (+2 ms offset for decorrelation)
    apDelayB[0] = (int) (0.007 * sampleRate);
    apDelayB[1] = (int) (0.011 * sampleRate);
    apDelayB[2] = (int) (0.016 * sampleRate);

    // Max wobble = 3 ms
    maxWobbleSamples = (float) (0.003 * sampleRate);

    // ─── Pre-delay buffers (max 100 ms per channel) ───────────────────────
    const int maxPreDelaySamples = (int) (0.1 * sampleRate) + 1;
    for (auto& pd : preDelay)
        pd.prepare (maxPreDelaySamples);

    // ─── Spring tank allpass sections ─────────────────────────────────────
    // AP1 and AP2: fixed delay, no modulation
    // AP3: modulated, needs headroom for LFO (base + 3 ms)
    for (int i = 0; i < 2; ++i)
    {
        apA[i].prepare (apDelayA[i] + 4);
        apB[i].prepare (apDelayB[i] + 4);
    }
    apA[2].prepare ((int) (apDelayA[2] + maxWobbleSamples) + 4);
    apB[2].prepare ((int) (apDelayB[2] + maxWobbleSamples) + 4);

    // ─── Damping LP filters (mono, one per string) ────────────────────────
    juce::dsp::ProcessSpec monoSpec;
    monoSpec.sampleRate       = sampleRate;
    monoSpec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    monoSpec.numChannels      = 1;

    dampA.setType (juce::dsp::FirstOrderTPTFilterType::lowpass);
    dampA.setCutoffFrequency (8000.0f);
    dampA.prepare (monoSpec);
    dampA.reset();

    dampB.setType (juce::dsp::FirstOrderTPTFilterType::lowpass);
    dampB.setCutoffFrequency (8000.0f);
    dampB.prepare (monoSpec);
    dampB.reset();

    // ─── Parameter smoothers ──────────────────────────────────────────────
    smoothMix.reset  (sampleRate, 0.010);   // 10 ms ramp
    smoothDrive.reset (sampleRate, 0.010);

    smoothMix.setCurrentAndTargetValue (
        apvts.getRawParameterValue ("mix")->load());
    smoothDrive.setCurrentAndTargetValue (
        apvts.getRawParameterValue ("drive")->load());

    // ─── Reset feedback and LFO state ─────────────────────────────────────
    resetSpringTank();
}

void NFReverbAudioProcessor::releaseResources()
{
    resetSpringTank();
    dampA.reset();
    dampB.reset();
}

void NFReverbAudioProcessor::resetSpringTank() noexcept
{
    for (auto& pd : preDelay) pd.reset();
    for (auto& ap : apA)      ap.reset();
    for (auto& ap : apB)      ap.reset();
    feedbackA = 0.0f;
    feedbackB = 0.0f;
    lfoPhaseA = 0.0f;
    lfoPhaseB = 0.0f;
}

// =============================================================================
// Bus layout
// =============================================================================
bool NFReverbAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}

// =============================================================================
// Drive (tanh soft saturation, unity-gain normalised for small signals)
// tanh(x * g) / g  →  approaches x as g → 1, clips softly as g increases
// =============================================================================
float NFReverbAudioProcessor::applyDrive (float x, float driveGain) noexcept
{
    return std::tanh (x * driveGain) / driveGain;
}

// =============================================================================
// processBlock
// =============================================================================
void NFReverbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);

    if (numSamples == 0)
        return;

    // Clear any extra output channels
    for (int ch = numChannels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    // ─── Read parameters (once per block) ─────────────────────────────────
    const float tension_n  = apvts.getRawParameterValue ("tension")->load();
    const float damping_n  = apvts.getRawParameterValue ("damping")->load();
    const float wobble_n   = apvts.getRawParameterValue ("wobble")->load();
    const float decay_n    = juce::jmax (0.01f, apvts.getRawParameterValue ("decay")->load());
    const float preDelay_ms = apvts.getRawParameterValue ("pre_delay")->load();

    // Update smoothed targets
    smoothMix.setTargetValue  (apvts.getRawParameterValue ("mix")->load());
    smoothDrive.setTargetValue (apvts.getRawParameterValue ("drive")->load());

    // ─── Derive block-level DSP values ────────────────────────────────────

    // Allpass coefficient: tension maps [0,1] → [0.30, 0.75]
    const float apCoeff = juce::jlimit (0.2f, 0.8f, 0.30f + tension_n * 0.45f);

    // Damping LP cutoff: damping 0 = 16 kHz (bright), 1 = 2 kHz (dark)
    const float lpCutoff = 16000.0f - damping_n * 14000.0f;
    dampA.setCutoffFrequency (lpCutoff);
    dampB.setCutoffFrequency (lpCutoff);

    // Feedback gain from RT60 formula:  fb = 10^(-3 * T_loop / T_60)
    const float loopTimeA = (float)(apDelayA[0] + apDelayA[1] + apDelayA[2])
                          / (float) currentSampleRate;
    const float loopTimeB = (float)(apDelayB[0] + apDelayB[1] + apDelayB[2])
                          / (float) currentSampleRate;
    const float fbGainA = juce::jlimit (0.0f, 0.95f,
        std::pow (10.0f, -3.0f * loopTimeA / decay_n));
    const float fbGainB = juce::jlimit (0.0f, 0.95f,
        std::pow (10.0f, -3.0f * loopTimeB / decay_n));

    // Wobble LFO depth (samples) — up to 3 ms
    const float wobDepth = wobble_n * maxWobbleSamples;

    // Pre-delay in samples (clamped to buffer size)
    const int preDelSamples = juce::jlimit (0, preDelay[0].maxSize - 2,
        (int) (preDelay_ms * (float) currentSampleRate * 0.001f));

    // LFO phase increment per sample
    const float lfoIncA = LFO_RATE_A / (float) currentSampleRate;
    const float lfoIncB = LFO_RATE_B / (float) currentSampleRate;
    const float twoPi   = juce::MathConstants<float>::twoPi;

    // ─── Per-sample loop ──────────────────────────────────────────────────
    for (int s = 0; s < numSamples; ++s)
    {
        // Per-sample smoothed values
        const float mix      = smoothMix.getNextValue();
        const float driveGain = 1.0f + smoothDrive.getNextValue() * 3.0f;

        // ── Channel A — left ──────────────────────────────────────────────
        const float inA  = (numChannels > 0) ? buffer.getSample (0, s) : 0.0f;
        const float dryA = inA;

        // Pre-delay
        preDelay[0].write (inA);
        float vA = preDelay[0].read (preDelSamples);

        // Drive
        vA = applyDrive (vA, driveGain);

        // Spring string A
        vA += feedbackA;
        vA = apA[0].process (vA, apDelayA[0], apCoeff);
        vA = apA[1].process (vA, apDelayA[1], apCoeff);

        // AP3 with LFO modulation
        const float modA = juce::jlimit (1.0f, (float)(apA[2].maxSize - 3),
            (float) apDelayA[2] + wobDepth * std::sin (twoPi * lfoPhaseA));
        vA = apA[2].processInterp (vA, modA, apCoeff);

        // Update feedback through damping LP
        feedbackA = dampA.processSample (0, vA) * fbGainA;

        // ── Channel B — right ─────────────────────────────────────────────
        const float inB  = (numChannels > 1) ? buffer.getSample (1, s) : inA;
        const float dryB = inB;

        preDelay[1].write (inB);
        float vB = preDelay[1].read (preDelSamples);
        vB = applyDrive (vB, driveGain);

        vB += feedbackB;
        vB = apB[0].process (vB, apDelayB[0], apCoeff);
        vB = apB[1].process (vB, apDelayB[1], apCoeff);

        const float modB = juce::jlimit (1.0f, (float)(apB[2].maxSize - 3),
            (float) apDelayB[2] + wobDepth * std::sin (twoPi * lfoPhaseB));
        vB = apB[2].processInterp (vB, modB, apCoeff);

        feedbackB = dampB.processSample (0, vB) * fbGainB;

        // ── Mix blend and write output ─────────────────────────────────────
        const float dry = 1.0f - mix;
        if (numChannels > 0)
            buffer.setSample (0, s, dryA * dry + vA * mix);
        if (numChannels > 1)
            buffer.setSample (1, s, dryB * dry + vB * mix);

        // ── Advance LFO phases ─────────────────────────────────────────────
        lfoPhaseA += lfoIncA;
        if (lfoPhaseA >= 1.0f) lfoPhaseA -= 1.0f;
        lfoPhaseB += lfoIncB;
        if (lfoPhaseB >= 1.0f) lfoPhaseB -= 1.0f;
    }
}

// =============================================================================
// State persistence
// =============================================================================
void NFReverbAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void NFReverbAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// =============================================================================
// Plugin entry point
// =============================================================================
juce::AudioProcessorEditor* NFReverbAudioProcessor::createEditor()
{
    return new NFReverbAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NFReverbAudioProcessor();
}
