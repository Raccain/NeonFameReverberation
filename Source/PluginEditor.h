#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "ParameterIDs.hpp"

// =============================================================================
// MyReverbAudioProcessorEditor
//
// WebView2 editor hosting the NeoGrid Minimal HTML/JS interface.
//
// CRITICAL MEMBER ORDER (prevents DAW crash on unload):
//   C++ destroys members in REVERSE order of declaration.
//   1. Relays declared FIRST  → destroyed LAST  (nothing references them when destroyed)
//   2. webView declared SECOND → destroyed MIDDLE (relays still alive)
//   3. Attachments declared LAST → destroyed FIRST (safe to release first)
//
// See: .claude/troubleshooting/resolutions/webview-member-order-crash.md
// =============================================================================
class MyReverbAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MyReverbAudioProcessorEditor (MyReverbAudioProcessor&);
    ~MyReverbAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MyReverbAudioProcessor& audioProcessor;

    // =========================================================================
    // 1. PARAMETER RELAYS FIRST (no dependencies — destroyed last)
    //    Each relay is a direct member, initialised with its parameter ID string.
    // =========================================================================
    juce::WebSliderRelay mixRelay      { "mix"       };
    juce::WebSliderRelay decayRelay    { "decay"     };
    juce::WebSliderRelay tensionRelay  { "tension"   };
    juce::WebSliderRelay preDelayRelay { "pre_delay" };
    juce::WebSliderRelay dampingRelay  { "damping"   };
    juce::WebSliderRelay wobbleRelay   { "wobble"    };
    juce::WebSliderRelay driveRelay    { "drive"     };

    // =========================================================================
    // 2. WEBVIEW SECOND (references relays via .withOptionsFrom — destroyed middle)
    // =========================================================================
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // =========================================================================
    // 3. PARAMETER ATTACHMENTS LAST (depend on relays + APVTS — destroyed first)
    // =========================================================================
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> decayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> tensionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> preDelayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> dampingAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> wobbleAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> driveAttachment;

    // =========================================================================
    // Resource provider
    // =========================================================================
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    static const char*   getMimeForExtension (const juce::String& extension);
    static juce::String  getExtension (const juce::String& filename);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MyReverbAudioProcessorEditor)
};
