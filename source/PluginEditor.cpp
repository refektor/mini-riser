#include "PluginProcessor.h"
#include "PluginEditor.h"

GrooveGlideAudioProcessorEditor::GrooveGlideAudioProcessorEditor (GrooveGlideAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      impactRelay{"impact"},
      impactAttachment{*audioProcessor.getState().getParameter("impact"), impactRelay},
      webView{
          juce::WebBrowserComponent::Options{}
              .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
          #ifdef _WIN32
              .withWinWebView2Options(juce::WebBrowserComponent::Options::WinWebView2{}.withUserDataFolder
              (juce::File::getSpecialLocation(juce::File::tempDirectory)))
          #endif
              .withResourceProvider([this](const juce::String& url) { return getResource(url); })
              .withNativeIntegrationEnabled()
              .withOptionsFrom(impactRelay)
      }
{
    addAndMakeVisible (webView);
    webView.goToURL(webView.getResourceProviderRoot());
    setSize (310, 310);
    //setResizable(true, true);
    //setResizeLimits(600, 400, 1200, 800);
}

GrooveGlideAudioProcessorEditor::~GrooveGlideAudioProcessorEditor()
{
}

void GrooveGlideAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void GrooveGlideAudioProcessorEditor::resized()
{
    webView.setBounds(getLocalBounds());
}

void GrooveGlideAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    // Store the initial position of the mouse
    lastDragPosition = e.getPosition();
    isDragging = false;
    
    // Pass the event to the component underneath
    webView.mouseDown(e.getEventRelativeTo(&webView));
}

void GrooveGlideAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    // Calculate drag distance to detect significant movement
    auto currentPos = e.getPosition();
    auto dragDistance = currentPos.getDistanceFrom(lastDragPosition);
    
    // Only process drag if it's a meaningful movement (prevents tiny movements from causing issues)
    if (dragDistance > 2)
    {
        isDragging = true;
        
        // Safely forward the event to the web view
        auto relativeEvent = e.getEventRelativeTo(&webView);
        if (webView.isShowing())
            webView.mouseDrag(relativeEvent);
            
        // Update the last position
        lastDragPosition = currentPos;
    }
}

void GrooveGlideAudioProcessorEditor::mouseUp(const juce::MouseEvent& e)
{
    // Reset drag state
    isDragging = false;
    
    // Forward the event to the web view
    webView.mouseUp(e.getEventRelativeTo(&webView));
}

void GrooveGlideAudioProcessorEditor::timerCallback()
{
    // Timer callback for potential future use
}

juce::String GrooveGlideAudioProcessorEditor::getMimeForExtension(const juce::String& extension)
{
    static juce::HashMap<juce::String, juce::String> mimeMap;
    static bool initialized = false;
    
    if (!initialized) {
        mimeMap.set("html", "text/html");
        mimeMap.set("css", "text/css");
        mimeMap.set("js", "application/javascript");
        mimeMap.set("png", "image/png");
        mimeMap.set("jpg", "image/jpeg");
        mimeMap.set("jpeg", "image/jpeg");
        mimeMap.set("gif", "image/gif");
        mimeMap.set("svg", "image/svg+xml");
        mimeMap.set("ico", "image/x-icon");
        mimeMap.set("woff", "font/woff");
        mimeMap.set("woff2", "font/woff2");
        mimeMap.set("ttf", "font/ttf");
        mimeMap.set("otf", "font/otf");
        initialized = true;
    }
    
    return mimeMap[extension.toLowerCase()];
}

std::optional<juce::WebBrowserComponent::Resource> 
GrooveGlideAudioProcessorEditor::getResource(const juce::String& url)
{
    // Debug logging to file
    juce::File debugFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                                   .getChildFile("PluginResourceDebug.txt");
    juce::String debugMessage = juce::Time::getCurrentTime().toString(true, true) + 
                               ": Requested URL: " + url + "\n";
    debugFile.appendText(debugMessage);
    
    const auto resourceToRetrieve = url == "/" ? "index.html" : url.fromLastOccurrenceOf("/", false, false);
    
    // Convert resourceToRetrieve to a valid C++ identifier (exactly like JUCE does)
    juce::String resourceName = resourceToRetrieve.replaceCharacter('/', '_')
                                                  .replaceCharacter('.', '_')
                                                  .removeCharacters("-")       // Remove dashes (don't replace with underscores)
                                                  .removeCharacters(" ");      // Remove spaces
    
    debugFile.appendText("  Resource to retrieve: " + resourceToRetrieve + "\n");
    debugFile.appendText("  Converted resource name: " + resourceName + "\n");
    
    // Get the resource data and size
    int resourceSize = 0;
    const char* resourceData = BinaryData::getNamedResource(resourceName.toRawUTF8(), resourceSize);

    if (resourceData != nullptr && resourceSize > 0)
    {
        debugFile.appendText("  Found resource, size: " + juce::String(resourceSize) + "\n");
        const auto extension = resourceToRetrieve.fromLastOccurrenceOf(".", false, false);
        return juce::WebBrowserComponent::Resource{
            std::vector<std::byte>(reinterpret_cast<const std::byte*>(resourceData), 
                                 reinterpret_cast<const std::byte*>(resourceData) + resourceSize), 
            getMimeForExtension(extension)
        };
    }

    debugFile.appendText("  Resource NOT FOUND!\n");
    return std::nullopt;
}