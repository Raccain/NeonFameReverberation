#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

// =============================================================================
// Constructor
// =============================================================================
MyReverbAudioProcessorEditor::MyReverbAudioProcessorEditor (MyReverbAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    DBG ("MyReverb: Editor constructor started");

    // =========================================================================
    // CRITICAL CREATION ORDER (matches CloudWash working pattern):
    //   1. Relays already constructed as member variables (above)
    //   2. Create parameter ATTACHMENTS first (connect relays to APVTS params)
    //   3. Create WebBrowserComponent with .withOptionsFrom() for each relay
    //   4. addAndMakeVisible
    //   5. goToURL
    // =========================================================================

    // Step 2: Create attachments (before WebView)
    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("mix"),       mixRelay);
    decayAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("decay"),     decayRelay);
    tensionAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("tension"),   tensionRelay);
    preDelayAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("pre_delay"), preDelayRelay);
    dampingAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("damping"),   dampingRelay);
    wobbleAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("wobble"),    wobbleRelay);
    driveAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *audioProcessor.apvts.getParameter ("drive"),     driveRelay);

    // Step 3: Create WebBrowserComponent with Windows WebView2 backend
    webView = std::make_unique<juce::WebBrowserComponent> (
        juce::WebBrowserComponent::Options{}
            .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options (
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder (juce::File::getSpecialLocation (
                        juce::File::SpecialLocationType::tempDirectory)))
            .withNativeIntegrationEnabled()
            .withResourceProvider ([this](const auto& url) { return getResource (url); })
            .withOptionsFrom (mixRelay)
            .withOptionsFrom (decayRelay)
            .withOptionsFrom (tensionRelay)
            .withOptionsFrom (preDelayRelay)
            .withOptionsFrom (dampingRelay)
            .withOptionsFrom (wobbleRelay)
            .withOptionsFrom (driveRelay)
    );

    // Step 4: Add to component hierarchy
    addAndMakeVisible (*webView);

    // Step 5: Load web content through resource provider (NOT a data URI)
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    // Window size matches approved design (680 × 280 px)
    setSize (680, 280);

    DBG ("MyReverb: Editor constructor completed");
}

// =============================================================================
// Paint / Resized
// =============================================================================
void MyReverbAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void MyReverbAudioProcessorEditor::resized()
{
    if (webView)
        webView->setBounds (getLocalBounds());
}

// =============================================================================
// Resource Provider
//
// Maps request URL paths to embedded BinaryData constants.
//
// IMPORTANT: JUCE mangles duplicate filenames when embedding:
//   Source/ui/public/js/index.js       → BinaryData::index_js
//   Source/ui/public/js/juce/index.js  → BinaryData::index_js2   (mangled!)
//   check_native_interop.js            → BinaryData::check_native_interop_js
// =============================================================================
std::optional<juce::WebBrowserComponent::Resource>
MyReverbAudioProcessorEditor::getResource (const juce::String& url)
{
    auto path = url.fromFirstOccurrenceOf (
        juce::WebBrowserComponent::getResourceProviderRoot(), false, false);

    if (path.isEmpty() || path == "/")
        path = "/index.html";

    DBG ("MyReverb resource: " + path);

    const char* data = nullptr;
    int         size = 0;
    juce::String mime;

    const auto p = path.substring (1);   // remove leading '/'

    if (p == "index.html")
    {
        data = BinaryData::index_html;
        size = BinaryData::index_htmlSize;
        mime = "text/html";
    }
    else if (p == "js/index.js")
    {
        data = BinaryData::index_js;
        size = BinaryData::index_jsSize;
        mime = "text/javascript";
    }
    else if (p == "js/juce/index.js")
    {
        data = BinaryData::index_js2;       // mangled by CMake — same filename
        size = BinaryData::index_js2Size;
        mime = "text/javascript";
    }
    else if (p == "js/juce/check_native_interop.js")
    {
        data = BinaryData::check_native_interop_js;
        size = BinaryData::check_native_interop_jsSize;
        mime = "text/javascript";
    }

    if (data != nullptr && size > 0)
    {
        std::vector<std::byte> bytes ((size_t) size);
        std::memcpy (bytes.data(), data, (size_t) size);
        return juce::WebBrowserComponent::Resource { std::move (bytes), mime };
    }

    // Fallback: resource not found
    DBG ("MyReverb: resource NOT found — " + p);
    const juce::String fallback = "<!DOCTYPE html><html><body style='background:#111;color:#0ff;font-family:monospace;padding:20px'>"
                                  "<h3>MyReverb</h3><p>Resource not found: " + p + "</p></body></html>";
    std::vector<std::byte> fb ((size_t) fallback.getNumBytesAsUTF8());
    std::memcpy (fb.data(), fallback.toRawUTF8(), fb.size());
    return juce::WebBrowserComponent::Resource { std::move (fb), "text/html" };
}

const char* MyReverbAudioProcessorEditor::getMimeForExtension (const juce::String& ext)
{
    if (ext == "html") return "text/html";
    if (ext == "js")   return "text/javascript";
    if (ext == "css")  return "text/css";
    if (ext == "json") return "application/json";
    if (ext == "svg")  return "image/svg+xml";
    if (ext == "png")  return "image/png";
    return "text/plain";
}

juce::String MyReverbAudioProcessorEditor::getExtension (const juce::String& filename)
{
    return filename.fromLastOccurrenceOf (".", false, false);
}
