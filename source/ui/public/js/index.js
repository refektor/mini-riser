import * as Juce from "./juce/juce_index.js";

document.addEventListener("DOMContentLoaded", () => {
  // Get the knob element
  const knob = document.getElementById("impact-knob");
  const characterContainer = document.getElementById("character-container");
  
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
  
  // Dancing characters
  const NUM_DANCERS = 250;
  let dancers = [];
  let animationFrameId;
  let currentBPM = 120; // Default BPM
  
  // An array of different shades of white and light gray
  const whiteShades = ['#ffffff', '#f5f5f5', '#eeeeee', '#e0e0e0', '#dcdcdc'];
  
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
  
  /**
   * Generates the SVG string for a single dancer with random shades of white.
   * @returns {string} The SVG markup for a dancer.
   */
  function createDancerSVG() {
    // Pick random shades for the head, body, and legs
    const headColor = whiteShades[Math.floor(Math.random() * whiteShades.length)];
    const bodyColor = whiteShades[Math.floor(Math.random() * whiteShades.length)];
    const legColor = whiteShades[Math.floor(Math.random() * whiteShades.length)];

    return `
      <svg class="dancer-svg" width="40" height="50" viewBox="0 0 40 50">
        <rect x="15" y="15" width="10" height="20" fill="${bodyColor}" rx="3" ry="3" />
        <circle cx="20" cy="10" r="8" fill="${headColor}" />
        <rect x="16" y="35" width="3" height="15" fill="${legColor}" rx="2" ry="2" />
        <rect x="21" y="35" width="3" height="15" fill="${legColor}" rx="2" ry="2" />
      </svg>
    `;
  }
  
  /**
   * Creates and scatters dancers on the screen with random positions and initial angles.
   */
  function initializeDancers() {
    characterContainer.innerHTML = '';
    dancers = [];

    for (let i = 0; i < NUM_DANCERS; i++) {
      const dancerWrapper = document.createElement('div');
      dancerWrapper.innerHTML = createDancerSVG();
      const dancerSvg = dancerWrapper.firstElementChild;
      
      // Set a random position for each dancer
      const x = Math.random() * (characterContainer.offsetWidth - 40);
      const y = Math.random() * (characterContainer.offsetHeight - 50);
      dancerSvg.style.left = `${x}px`;
      dancerSvg.style.top = `${y}px`;
      
      // Store the dancer element and a unique phase for the animation
      dancers.push({
        element: dancerSvg,
        phaseOffset: Math.random() * Math.PI * 2,
        direction: Math.random() > 0.5 ? 1 : -1 // Start swaying left or right
      });
      characterContainer.appendChild(dancerSvg);
    }
  }
  
  /**
   * Main animation loop. Updates the sway angle and speed for all dancers
   * based on the current BPM and Impact parameter value.
   */
  function animateDancers() {
    const bpm = currentBPM;
    const paramValue = currentValue; // Use the knob's current value

    // Map BPM to animation speed
    const beatInterval = (60 / bpm) * 1000;
    const time = performance.now();

    // Map parameter value to sway amplitude and speed multiplier
    const maxSway = 40; // More dramatic sway
    const minSway = 10;  // Still visible at minimum
    const swayRotation = maxSway - ((paramValue / 100) * (maxSway - minSway));

    const maxSpeedMultiplier = 8; // Slower speed
    const minSpeedMultiplier = 1; // Normal speed
    const speedMultiplier = minSpeedMultiplier + ((paramValue / 100) * (maxSpeedMultiplier - minSpeedMultiplier));

    dancers.forEach(dancer => {
      // Calculate the current rotation based on BPM, speed multiplier, and individual phase
      const angle = Math.sin((time / beatInterval) * Math.PI * 2 / speedMultiplier + dancer.phaseOffset) * swayRotation;
      dancer.element.style.transform = `rotate(${angle}deg)`;
    });

    animationFrameId = requestAnimationFrame(animateDancers);
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
  
  // Initialize dancers and start animation
  initializeDancers();
  animateDancers();
  
  // Initialize knob to 0 position
  updateKnobVisual(0);
  
  // Re-initialize dancers on window resize
  window.addEventListener('resize', () => {
    if (animationFrameId) {
      cancelAnimationFrame(animationFrameId);
    }
    initializeDancers();
    animateDancers();
  });
});