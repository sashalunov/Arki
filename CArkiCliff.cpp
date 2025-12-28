#include "CArkiCliff.h"

void InitNoise()
{
    if (s_noiseInitialized) return;

    // 1. Set "Keyframes" (Random values every 10 steps)
    // This creates "Low Frequency" noise (Smooth, large shapes)
    for (int i = 0; i < 1024; i += 10) {
        float val = ((rand() % 100) / 100.0f - 0.5f) * 4.0f; // Range -2.0 to +2.0

        // Ensure the loop wraps around smoothly at the end
        if (i >= 1020) val = s_noiseBuffer[0];

        s_noiseBuffer[i] = val;
    }

    // 2. Fill the gaps (Linear Interpolation)
    for (int i = 0; i < 1024 - 10; i += 10)
    {
        float startVal = s_noiseBuffer[i];
        float endVal = s_noiseBuffer[i + 10];

        for (int k = 1; k < 10; k++) {
            float t = (float)k / 10.0f;
            s_noiseBuffer[i + k] = startVal + t * (endVal - startVal);
        }
    }

    s_noiseInitialized = true;
}
// Get jagged offset for a specific vertical position
float GetNoiseAt(float y)
{
    // 1. Map Y to the buffer size
    // We multiply by a smaller number (e.g., 2.0f) to "stretch" the noise vertically.
    // Smaller multiplier = taller, smoother cliffs.
    float samplePos = abs(y) * 5.0f;

    // 2. Wrap around 1024
    // fmod gives the floating point remainder
    samplePos = fmod(samplePos, 1023.0f);

    // 3. Find the two closest indices
    int indexA = (int)samplePos;
    int indexB = indexA + 1;

    // 4. Calculate the fraction (0.0 to 1.0)
    // If samplePos is 5.3, fraction is 0.3
    float t = samplePos - indexA;

    // 5. LERP (Linear Interpolate)
    float valA = s_noiseBuffer[indexA];
    float valB = s_noiseBuffer[indexB];

    // Result is blended smoothly between A and B
    return valA + t * (valB - valA);
}