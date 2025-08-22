# JUCE Audio Plugin Template

This template provides a comprehensive foundation for building modern JUCE audio plugins with web-based UIs. The architecture is based on the sas1-plugin project structure and design patterns.

## Project Structure

```
audio-plugin-template/
├── CMakeLists.txt                 # Build configuration
├── source/
│   ├── PluginProcessor.h         # Audio processing core
│   ├── PluginProcessor.cpp       # Audio processing implementation
│   ├── PluginEditor.h            # UI management
│   ├── PluginEditor.cpp          # UI implementation
│   └── ui/
│       └── public/               # Web UI assets (embedded as binary data)
│           ├── index.html        # Main UI layout
│           ├── css/
│           │   └── styles.css    # UI styling
│           ├── js/
│           │   └── index.js      # UI behavior & JUCE integration
│           └── imgs/             # Images and graphics
└── presets/                      # Audio samples (.wav files)
```

## Key Architecture Components

### 1. CMakeLists.txt Configuration

The build system uses CMake with these essential components:

**CPM Package Manager**: Automatically downloads and manages JUCE dependency
```cmake
CPMAddPackage(
    NAME juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG origin/master
)
```

**Plugin Configuration**: Define plugin metadata and formats
```cmake
juce_add_plugin(${PROJECT_NAME}
    COMPANY_NAME "Your Company"
    BUNDLE_ID "com.yourcompany.pluginname" 
    IS_SYNTH TRUE                    # Set to FALSE for effects
    NEEDS_MIDI_INPUT TRUE            # Enable MIDI input
    NEEDS_WEBVIEW2 TRUE              # Essential for web UI
    FORMATS VST3 AU Standalone       # Target plugin formats
)
```

**Binary Data Integration**: Automatically embed web assets and presets
```cmake
# Collect web UI files and audio samples
file(GLOB_RECURSE RESOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/source/ui/public/*")
file(GLOB WAV_FILES "${CMAKE_CURRENT_SOURCE_DIR}/presets/*.wav")

juce_add_binary_data(BinaryData
    SOURCES
    ${WAV_FILES}
    ${RESOURCE_FILES}
)
target_link_libraries(${PROJECT_NAME} PRIVATE BinaryData)
```

**JUCE Library Dependencies**:
```cmake
target_link_libraries(${PROJECT_NAME}
    PUBLIC
    juce::juce_audio_basics
    juce::juce_audio_devices  
    juce::juce_audio_utils        # For AudioProcessorValueTreeState
    juce::juce_gui_extra          # For WebBrowserComponent
    juce::juce_dsp               # For audio effects
)
```

**WebView2 Support**: Enable web UI functionality
```cmake
target_compile_definitions(${PROJECT_NAME}
    PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
)
```

### 2. PluginProcessor Architecture

**Core Structure**:
```cpp
class PluginProcessor : public juce::AudioProcessor,
                       public juce::AudioProcessorValueTreeState::Listener
{
private:
    // Parameter management
    struct Parameters {
        juce::AudioParameterFloat* param1{nullptr};
        juce::AudioParameterChoice* presetIndex{nullptr};
        // Add more parameters as needed
    };
    Parameters parameters;
    juce::AudioProcessorValueTreeState state;
    
    // Audio processing components
    juce::Synthesiser synth;              // For instruments
    juce::dsp::Reverb reverb;            // Effects
    juce::dsp::DelayLine<float> delay;   // More effects
};
```

**Parameter System Setup**:
```cpp
// In constructor
PluginProcessor::PluginProcessor() 
    : state{*this, nullptr, "PARAMETERS", createParameterLayout(parameters)}
{
    // Register as parameter listener
    state.addParameterListener("parameterID", this);
}

// Parameter layout creation
juce::AudioProcessorValueTreeState::ParameterLayout 
PluginProcessor::createParameterLayout(Parameters& parameters) {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Add float parameter
    auto param = std::make_unique<juce::AudioParameterFloat>(
        "paramID", "Display Name", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 
        0.5f  // default value
    );
    parameters.param1 = param.get();
    layout.add(std::move(param));
    
    return layout;
}

// Parameter change handling
void parameterChanged(const juce::String& parameterID, float newValue) override {
    if (parameterID == "paramID") {
        // Update internal state
        setInternalParameter(newValue);
    }
}
```

### 3. PluginEditor & WebBrowserComponent Setup

**WebBrowserComponent Integration**:
```cpp
class PluginEditor : public juce::AudioProcessorEditor, private juce::Timer
{
private:
    PluginProcessor& processor;
    
    // Web UI relays for parameter communication
    juce::WebSliderRelay parameterRelay;
    juce::WebComboBoxRelay presetRelay;
    
    // Parameter attachments (bidirectional parameter sync)
    juce::WebSliderParameterAttachment parameterAttachment;
    juce::WebComboBoxParameterAttachment presetAttachment;
    
    // The web browser component
    juce::WebBrowserComponent webView;
    
    // Resource provider for serving embedded assets
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);
};
```

**Editor Constructor Pattern**:
```cpp
PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(&p), processor(p),
    parameterRelay{"parameterID"},  // Must match parameter ID
    webView{
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withResourceProvider([this](const juce::String& url) { return getResource(url); })
            .withNativeIntegrationEnabled()
            .withOptionsFrom(parameterRelay)  // Register relay
    },
    parameterAttachment{*processor.getState().getParameter("parameterID"), parameterRelay}
{
    addAndMakeVisible(webView);
    webView.goToURL(webView.getResourceProviderRoot());
    setSize(800, 600);
    startTimerHz(60);  // For VU meters or other real-time updates
}
```

### 4. Parameter Relays & Attachments System

**WebSliderRelay**: Connects JUCE parameters to JavaScript sliders
- Handles bidirectional communication between C++ and JS
- Automatically syncs parameter changes from both sides

**WebSliderParameterAttachment**: Links relays to AudioProcessorValueTreeState
- Ensures parameter changes are properly saved/restored
- Maintains consistency across plugin instances

**Usage Pattern**:
```cpp
// In header
juce::WebSliderRelay gainRelay;
juce::WebSliderParameterAttachment gainAttachment;

// In constructor  
gainRelay{"gain"},  // Parameter ID
webView{/* ... */.withOptionsFrom(gainRelay)},
gainAttachment{*processor.getState().getParameter("gain"), gainRelay}
```

### 5. Binary Data Integration

**Automatic Asset Embedding**:
The CMake configuration automatically converts all files in `source/ui/public/` into C++ binary data, making them accessible through the `BinaryData` namespace.

**Resource Provider Implementation**:
```cpp
std::optional<juce::WebBrowserComponent::Resource> 
PluginEditor::getResource(const juce::String& url) {
    // Parse URL and extract filename
    auto lastSlash = url.lastIndexOf("/");
    auto filename = url.substring(lastSlash + 1);
    
    // Convert filename to binary data identifier
    auto resourceName = filename.replaceCharacter('.', '_').replaceCharacter('-', '_');
    
    // Serve from embedded binary data
    if (auto* data = BinaryData::getNamedResource(resourceName.toRawUTF8(), size)) {
        auto mimeType = getMimeForExtension(filename.fromLastOccurrenceOf(".", false, false));
        return juce::WebBrowserComponent::Resource{
            std::vector<std::byte>(reinterpret_cast<const std::byte*>(data), 
                                 reinterpret_cast<const std::byte*>(data) + size),
            mimeType
        };
    }
    return std::nullopt;
}
```

### 6. JavaScript UI Integration

**JUCE Native Integration**:
```javascript
import * as Juce from "./juce/juce_index.js";

// Get parameter state object
const parameterState = Juce.getSliderState("parameterID");

// Listen for parameter changes from C++
parameterState.valueChangedEvent.addListener(() => {
    const value = parameterState.getValue();
    updateUIElement(value);
});

// Send parameter changes to C++
function setParameter(newValue) {
    parameterState.setValue(newValue);
}

// Combo box (for presets)
const presetState = Juce.getComboBoxState("presetIndex");
presetState.propertiesChangedEvent.addListener(() => {
    // Update dropdown options
    const choices = presetState.properties.choices;
    populateDropdown(choices);
});
```

**Real-time Data (VU Meters, etc.)**:
Use Timer in C++ to periodically send data to JavaScript:
```cpp
// In PluginEditor
void timerCallback() override {
    // Get current audio level
    float level = processor.getCurrentLevel();
    
    // Send to JavaScript
    webView.emitEvent("updateVUMeter", juce::var(level));
}
```

```javascript
// In JavaScript
window.addEventListener("updateVUMeter", (event) => {
    const level = event.detail;
    updateVUMeterDisplay(level);
});
```

## Build Process

1. **Configure**: `cmake -B build`
2. **Build**: `cmake --build build --config Release`
3. **Install**: Plugins automatically copied to system directories when `COPY_PLUGIN_AFTER_BUILD TRUE`

## Implementation Checklist

When creating a new plugin using this template:

### Setup Phase
- [ ] Update project name in CMakeLists.txt
- [ ] Set company name, bundle ID, and plugin metadata
- [ ] Configure plugin type (IS_SYNTH TRUE/FALSE)
- [ ] Set MIDI input/output requirements

### Parameter System
- [ ] Define Parameters struct in PluginProcessor.h
- [ ] Implement createParameterLayout() method
- [ ] Add parameter listener registration
- [ ] Implement parameterChanged() callback

### Web UI Components  
- [ ] Create WebSliderRelay for each parameter
- [ ] Set up WebSliderParameterAttachment in constructor
- [ ] Register relays with WebBrowserComponent options
- [ ] Design HTML layout with matching element IDs

### Audio Processing
- [ ] Initialize audio components (synthesizer, effects, etc.)
- [ ] Implement processBlock() for real-time audio
- [ ] Add prepareToPlay() for sample rate/buffer size setup
- [ ] Configure effect parameters and routing

### Asset Integration
- [ ] Place web assets in source/ui/public/
- [ ] Add audio samples to presets/ directory  
- [ ] Implement getResource() method for asset serving
- [ ] Test binary data embedding

### Testing & Polish
- [ ] Test parameter automation
- [ ] Verify preset loading/saving
- [ ] Check plugin formats (VST3, AU, Standalone)
- [ ] Test UI responsiveness and real-time updates

This template provides a solid foundation for modern JUCE plugins with web-based UIs, following industry best practices for parameter management, asset embedding, and cross-platform compatibility.

## Implementation Insights & Common Issues

### Critical SmoothedValue Usage Pattern

**Problem**: Parameters are received but don't affect audio processing.

When using `juce::SmoothedValue` for parameter interpolation, you MUST call `getNextValue()` for each sample in your processing functions, not just `getCurrentValue()`.

**Incorrect**:
```cpp
void processEffect(juce::AudioBuffer<float>& buffer) {
    float paramValue = parameterSmoothed.getCurrentValue(); // WRONG - doesn't update!
    // Apply effect using paramValue
}
```

**Correct**:
```cpp
void processEffect(juce::AudioBuffer<float>& buffer) {
    // Update smoothed values during processing
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        parameterSmoothed.getNextValue(); // Essential for proper interpolation
    }
    
    float paramValue = parameterSmoothed.getCurrentValue();
    // Apply effect using paramValue
}
```

### Parameter Communication Debugging

**File-based Debug Logging**:
For debugging parameter communication issues, use direct file logging instead of `juce::Logger::writeToLog()` which may not appear in DAW logs:

```cpp
void parameterChanged(const juce::String& parameterID, float newValue) override {
    // Debug logging to file
    juce::File debugFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                                   .getChildFile("PluginDebug.txt");
    juce::String debugMessage = juce::Time::getCurrentTime().toString(true, true) + 
                               ": Parameter changed: " + parameterID + " = " + 
                               juce::String(newValue) + "\n";
    debugFile.appendText(debugMessage);
    
    // Rest of parameter handling...
}
```

### JavaScript Parameter Integration

**Proper Parameter State Management**:
```javascript
// Get individual slider states (not a global state)
const parameterSliderState = Juce.getSliderState("parameterID");

// Listen for changes from C++
parameterSliderState.valueChangedEvent.addListener(() => {
    const value = parameterSliderState.getScaledValue();
    updateKnobVisual("parameter-knob", value, minRange, maxRange, "unit");
});

// Send changes to C++ using normalized values
function setParameterFromUI(normalizedValue) {
    parameterSliderState.setNormalisedValue(normalizedValue);
}

// Properly handle drag interactions
parameterSliderState.sliderDragStarted(); // Call when drag begins
// ... update parameter during drag ...
parameterSliderState.sliderDragEnded();   // Call when drag ends
```

### CMake JUCE JavaScript Integration

**Essential for WebBrowserComponent**:
```cmake
# Copy JUCE JavaScript files from build directory to source
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/juce-src/modules/juce_gui_extra/native/javascript/" 
     DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/source/ui/public/js/juce/")
file(RENAME "${CMAKE_CURRENT_SOURCE_DIR}/source/ui/public/js/juce/index.js" 
            "${CMAKE_CURRENT_SOURCE_DIR}/source/ui/public/js/juce/juce_index.js")
```

### Audio Processing Chain Best Practices

**Systematic Debugging Approach**:
1. **Test Simple Effects First**: Use obvious effects (gain boost, volume reduction) to verify parameter communication
2. **Isolate Each Processing Function**: Test one effect at a time with extreme settings
3. **Verify Smoothed Value Updates**: Ensure `getNextValue()` is called in processing loops
4. **Check Processing Order**: Verify audio chain order matches expected signal flow

**Example Test Effects**:
```cpp
// Clarity test: obvious gain boost
void processHighPass(juce::AudioBuffer<float>& buffer) {
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        claritySmoothed.getNextValue();
    }
    
    float clarityValue = claritySmoothed.getCurrentValue();
    float testGain = 1.0f + (clarityValue * 2.0f); // 1x to 3x gain
    
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        auto* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            channelData[sample] *= testGain;
        }
    }
}
```

### Common Troubleshooting Steps

1. **Parameters not received**: Check WebSliderRelay registration with `.withOptionsFrom(relay)` in WebBrowserComponent options
2. **Parameters received but no audio effect**: Verify SmoothedValue `getNextValue()` calls in processing functions  
3. **UI not responding**: Ensure JavaScript import paths are correct and JUCE files are copied
4. **Build errors**: Check CMakeLists.txt for proper JUCE module dependencies and WebView2 compilation definitions

### Implementation Success Indicators

- **Parameter Communication**: Debug file shows parameter changes in real-time
- **Audio Processing**: Obvious test effects (gain/distortion/volume) work immediately  
- **UI Responsiveness**: Knobs visually update when parameters change from automation
- **Plugin Loading**: VST3/AU loads without errors in DAW logs