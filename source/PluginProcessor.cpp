#include "PluginProcessor.h"
#include "PluginEditor.h"

GrooveGlideAudioProcessor::GrooveGlideAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
      state{*this, nullptr, "PARAMETERS", createParameterLayout(parameters)}
{
    state.addParameterListener("impact", this);
    impactSmoothed.setCurrentAndTargetValue(0.0f);
    
    delayParams.wetLevel.setCurrentAndTargetValue(0.0f);
    delayParams.feedback.setCurrentAndTargetValue(0.0f);
}

GrooveGlideAudioProcessor::~GrooveGlideAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout 
GrooveGlideAudioProcessor::createParameterLayout(Parameters& parameters)
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    auto impactParam = std::make_unique<juce::AudioParameterFloat>(
        "impact", "Impact", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 
        0.0f
    );
    parameters.impact = impactParam.get();
    layout.add(std::move(impactParam));
    
    return layout;
}

void GrooveGlideAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "impact") {
        impactSmoothed.setTargetValue(newValue);
        updateEffectParameters(newValue);
    }
}

const juce::String GrooveGlideAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GrooveGlideAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool GrooveGlideAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool GrooveGlideAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double GrooveGlideAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int GrooveGlideAudioProcessor::getNumPrograms()
{
    return 1;
}

int GrooveGlideAudioProcessor::getCurrentProgram()
{
    return 0;
}

void GrooveGlideAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String GrooveGlideAudioProcessor::getProgramName (int index)
{
    return {};
}

void GrooveGlideAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void GrooveGlideAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = static_cast<float>(sampleRate);
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;
    
    leftChain.processorChain.prepare(spec);
    rightChain.processorChain.prepare(spec);
    
    auto& leftHighPass = leftChain.processorChain.template get<EffectChain::highPassIndex>();
    auto& rightHighPass = rightChain.processorChain.template get<EffectChain::highPassIndex>();
    leftHighPass.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);
    rightHighPass.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);
    
    auto& leftReverb = leftChain.processorChain.template get<EffectChain::reverbIndex>();
    auto& rightReverb = rightChain.processorChain.template get<EffectChain::reverbIndex>();
    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize = 0.8f;
    reverbParams.damping = 0.3f;
    reverbParams.wetLevel = 0.0f;
    reverbParams.dryLevel = 1.0f;
    reverbParams.width = 1.0f;
    leftReverb.setParameters(reverbParams);
    rightReverb.setParameters(reverbParams);
    
    lfoForPanning.initialise([](float x) { return std::sin(x); });
    lfoForPanning.setFrequency(2.0f);
    lfoForPanning.prepare(spec);
    
    impactSmoothed.reset(sampleRate, 0.05);
    delayParams.wetLevel.reset(sampleRate, 0.05);
    delayParams.feedback.reset(sampleRate, 0.05);
    
    delayParams.delayTimeInSamples = static_cast<float>(sampleRate * 0.125);
    delayParams.delayBuffer.setSize(2, static_cast<int>(sampleRate * 2.0));
    delayParams.delayBuffer.clear();
    delayParams.writeIndex = 0;
}

void GrooveGlideAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool GrooveGlideAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void GrooveGlideAudioProcessor::updateEffectParameters(float impactValue)
{
    float normalizedImpact = impactValue / 100.0f;
    
    auto& leftHighPass = leftChain.processorChain.template get<EffectChain::highPassIndex>();
    auto& rightHighPass = rightChain.processorChain.template get<EffectChain::highPassIndex>();
    float cutoffFreq = 20.0f + (normalizedImpact * (1500.0f - 20.0f));
    leftHighPass.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, cutoffFreq);
    rightHighPass.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, cutoffFreq);
    
    auto& leftTransientShaper = leftChain.processorChain.template get<EffectChain::transientShaperIndex>();
    auto& rightTransientShaper = rightChain.processorChain.template get<EffectChain::transientShaperIndex>();
    float transientGain = 1.0f - (normalizedImpact * 0.5f);
    leftTransientShaper.setGainLinear(transientGain);
    rightTransientShaper.setGainLinear(transientGain);
    
    bitCrusherParams.bitDepth = 24.0f - (normalizedImpact * 18.0f);
    if (bitCrusherParams.bitDepth < 1.0f) bitCrusherParams.bitDepth = 1.0f;
    bitCrusherParams.quantizationStep = 2.0f / std::pow(2.0f, bitCrusherParams.bitDepth);
    
    auto& leftPanner = leftChain.processorChain.template get<EffectChain::autoPannerIndex>();
    auto& rightPanner = rightChain.processorChain.template get<EffectChain::autoPannerIndex>();
    float panAmount = normalizedImpact;
    leftPanner.setPan(-panAmount);
    rightPanner.setPan(panAmount);
    
    auto& leftReverb = leftChain.processorChain.template get<EffectChain::reverbIndex>();
    auto& rightReverb = rightChain.processorChain.template get<EffectChain::reverbIndex>();
    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize = 0.8f;
    reverbParams.damping = 0.3f;
    reverbParams.wetLevel = normalizedImpact * 0.5f;
    reverbParams.dryLevel = 1.0f;  // Keep dry signal at full level always
    reverbParams.width = 1.0f;
    leftReverb.setParameters(reverbParams);
    rightReverb.setParameters(reverbParams);
    
    delayParams.wetLevel.setTargetValue(normalizedImpact * 0.4f);
    delayParams.feedback.setTargetValue(normalizedImpact * 0.75f);
}

float GrooveGlideAudioProcessor::applyBitCrushing(float sample, float bitDepth)
{
    if (bitDepth >= 24.0f) return sample;
    
    float quantizationStep = bitCrusherParams.quantizationStep;
    return std::round(sample / quantizationStep) * quantizationStep;
}

void GrooveGlideAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        impactSmoothed.getNextValue();
        delayParams.wetLevel.getNextValue();
        delayParams.feedback.getNextValue();
    }

    float currentImpact = impactSmoothed.getCurrentValue() / 100.0f;
    
    // Complete bypass when Impact = 0
    if (currentImpact <= 0.001f) {
        // Audio passes through completely unprocessed
        return;
    }
    
    juce::dsp::AudioBlock<float> block(buffer);
    
    if (buffer.getNumChannels() >= 2) {
        auto leftBlock = block.getSingleChannelBlock(0);
        auto rightBlock = block.getSingleChannelBlock(1);
        
        juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
        juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
        
        leftChain.processorChain.template get<EffectChain::highPassIndex>().process(leftContext);
        rightChain.processorChain.template get<EffectChain::highPassIndex>().process(rightContext);
        
        leftChain.processorChain.template get<EffectChain::transientShaperIndex>().process(leftContext);
        rightChain.processorChain.template get<EffectChain::transientShaperIndex>().process(rightContext);
        
        auto* leftData = buffer.getWritePointer(0);
        auto* rightData = buffer.getWritePointer(1);
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            leftData[sample] = applyBitCrushing(leftData[sample], bitCrusherParams.bitDepth);
            rightData[sample] = applyBitCrushing(rightData[sample], bitCrusherParams.bitDepth);
        }
        
        if (currentImpact > 0.0f) {
            float lfoValue = lfoForPanning.processSample(0.0f) * currentImpact;
            float leftPan = 1.0f - (lfoValue * 0.5f);
            float rightPan = 1.0f + (lfoValue * 0.5f);
            
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                leftData[sample] *= leftPan;
                rightData[sample] *= rightPan;
            }
        }
        
        leftChain.processorChain.template get<EffectChain::reverbIndex>().process(leftContext);
        rightChain.processorChain.template get<EffectChain::reverbIndex>().process(rightContext);
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            float currentWetLevel = delayParams.wetLevel.getCurrentValue();
            float currentFeedback = delayParams.feedback.getCurrentValue();
            
            for (int channel = 0; channel < 2; ++channel) {
                auto* channelData = buffer.getWritePointer(channel);
                auto* delayData = delayParams.delayBuffer.getWritePointer(channel);
                
                int readIndex = (delayParams.writeIndex - static_cast<int>(delayParams.delayTimeInSamples) + delayParams.delayBuffer.getNumSamples()) % delayParams.delayBuffer.getNumSamples();
                
                float delayedSample = delayData[readIndex];
                float input = channelData[sample];
                float output = input + (delayedSample * currentWetLevel);
                
                delayData[delayParams.writeIndex] = input + (delayedSample * currentFeedback);
                channelData[sample] = output;
            }
            
            delayParams.writeIndex = (delayParams.writeIndex + 1) % delayParams.delayBuffer.getNumSamples();
        }
    }
}

bool GrooveGlideAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* GrooveGlideAudioProcessor::createEditor()
{
    return new GrooveGlideAudioProcessorEditor (*this);
}

void GrooveGlideAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto xml = state.copyState().createXml();
    copyXmlToBinary(*xml, destData);
}

void GrooveGlideAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xmlState = getXmlFromBinary(data, sizeInBytes);
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(state.state.getType()))
            state.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GrooveGlideAudioProcessor();
}