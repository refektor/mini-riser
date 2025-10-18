#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class MiniRiserAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    MiniRiserAudioProcessorEditor (MiniRiserAudioProcessor&);
    ~MiniRiserAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

private:
    void timerCallback() override;
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);
    juce::String getMimeForExtension(const juce::String& extension);

    MiniRiserAudioProcessor& audioProcessor;
    
    juce::WebSliderRelay impactRelay;
    juce::WebSliderParameterAttachment impactAttachment;
    
    juce::WebBrowserComponent webView;
    
    // Mouse tracking for sas1-plugin style event forwarding
    juce::Point<int> lastDragPosition;
    bool isDragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniRiserAudioProcessorEditor)
};