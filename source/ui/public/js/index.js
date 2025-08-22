import * as Juce from "./juce/juce_index.js";

document.addEventListener("DOMContentLoaded", () => {
  // Get the knob element
  const knob = document.getElementById("impact-knob");
  
  // Get the JUCE parameter state
  const impactSliderState = Juce.getSliderState("impact");
  
  // Rotation settings
  const MIN_ROTATION = -135; // Bottom left (0 value)
  const MAX_ROTATION = 135;  // Bottom right (100 value)
  const ROTATION_RANGE = MAX_ROTATION - MIN_ROTATION; // 270 degrees total
  
  // Current state
  let currentValue = 0; // 0-100
  let isDragging = false;
  let lastMouseY = 0;
  
  // Function to convert value (0-100) to rotation (-135 to 135)
  function valueToRotation(value) {
    return MIN_ROTATION + (value / 100) * ROTATION_RANGE;
  }
  
  // Function to convert rotation (-135 to 135) to value (0-100)
  function rotationToValue(rotation) {
    return ((rotation - MIN_ROTATION) / ROTATION_RANGE) * 100;
  }
  
  // Function to update knob visual
  function updateKnobVisual(value) {
    currentValue = Math.max(0, Math.min(100, value));
    const rotation = valueToRotation(currentValue);
    knob.style.transform = `rotate(${rotation}deg)`;
  }
  
  // Function to send value to JUCE
  function sendToJUCE(value) {
    const normalizedValue = value / 100;
    impactSliderState.setNormalisedValue(normalizedValue);
  }
  
  // Listen for parameter changes from JUCE (automation, preset loading, etc.)
  impactSliderState.valueChangedEvent.addListener(() => {
    const juceValue = impactSliderState.getScaledValue();
    updateKnobVisual(juceValue);
  });
  
  // Mouse down - start dragging
  knob.addEventListener("mousedown", (e) => {
    e.preventDefault();
    isDragging = true;
    lastMouseY = e.clientY;
    knob.classList.add("active");
  });
  
  // Mouse move - update knob while dragging
  document.addEventListener("mousemove", (e) => {
    if (!isDragging) return;
    
    e.preventDefault();
    
    // Calculate mouse movement (inverted Y so up = increase)
    const deltaY = lastMouseY - e.clientY;
    lastMouseY = e.clientY;
    
    // Update value based on mouse movement
    const sensitivity = 0.5; // Adjust this for faster/slower knob response
    const valueChange = deltaY * sensitivity;
    const newValue = currentValue + valueChange;
    
    // Clamp to 0-100 range
    const clampedValue = Math.max(0, Math.min(100, newValue));
    
    // Update knob and send to JUCE
    updateKnobVisual(clampedValue);
    sendToJUCE(clampedValue);
  });
  
  // Mouse up - stop dragging
  document.addEventListener("mouseup", () => {
    if (isDragging) {
      isDragging = false;
      knob.classList.remove("active");
    }
  });
  
  // Initialize knob to 0 position
  updateKnobVisual(0);
});