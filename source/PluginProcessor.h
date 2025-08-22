#pragma once

#include <JuceHeader.h>

class GrooveGlideAudioProcessor : public juce::AudioProcessor,
                                  public juce::AudioProcessorValueTreeState::Listener
{
public:
    GrooveGlideAudioProcessor();
    ~GrooveGlideAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getState() { return state; }

private:
    struct Parameters {
        juce::AudioParameterFloat* impact{nullptr};
    };
    Parameters parameters;
    juce::AudioProcessorValueTreeState state;
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(Parameters& parameters);
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    juce::SmoothedValue<float> impactSmoothed;

    struct EffectChain {
        juce::dsp::ProcessorChain<
            juce::dsp::IIR::Filter<float>,           // High-pass filter
            juce::dsp::Gain<float>,                  // Transient shaper (simplified as gain reduction)
            juce::dsp::Gain<float>,                  // Bit crusher placeholder
            juce::dsp::Panner<float>,                // Auto panner
            juce::dsp::Reverb,                       // Reverb
            juce::dsp::DelayLine<float>              // Delay
        > processorChain;
        
        enum {
            highPassIndex = 0,
            transientShaperIndex,
            bitCrusherIndex,
            autoPannerIndex,
            reverbIndex,
            delayIndex
        };
    };
    
    EffectChain leftChain, rightChain;
    
    juce::dsp::Oscillator<float> lfoForPanning;
    
    float currentSampleRate = 44100.0f;
    
    struct DelayParams {
        juce::SmoothedValue<float> wetLevel;
        juce::SmoothedValue<float> feedback;
        float delayTimeInSamples = 0.0f;
        juce::AudioBuffer<float> delayBuffer;
        int writeIndex = 0;
    };
    DelayParams delayParams;
    
    struct BitCrusherParams {
        float bitDepth = 24.0f;
        float quantizationStep = 0.0f;
    };
    BitCrusherParams bitCrusherParams;
    
    float applyBitCrushing(float sample, float bitDepth);
    void updateEffectParameters(float impactValue);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrooveGlideAudioProcessor)
};