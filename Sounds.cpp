#include "Sounds.h"

std::vector<char> GenerateJumpSound() {
    const int SAMPLE_RATE = 44100;
    const int DURATION_MS = 300;
    int totalSamples = (SAMPLE_RATE * DURATION_MS) / 1000;

    // Helper buffer for raw samples
    std::vector<short> samples;
    samples.reserve(totalSamples);

    double frequency = 150.0;
    double volume = 0.5;
    double phase = 0.0;

    for (int i = 0; i < totalSamples; ++i) {
        // Simple Square Wave Logic
        double phaseIncrement = frequency / SAMPLE_RATE;
        phase += phaseIncrement;
        if (phase >= 1.0) phase -= 1.0;

        float sampleValue = (phase < 0.5) ? 1.0f : -1.0f;
        sampleValue *= (FLOAT)volume;

        // Apply NES-style effects
        frequency += 1.5;           // Pitch slide up
        volume -= (0.5 / totalSamples); // Decay
        if (volume < 0) volume = 0;

        samples.push_back(static_cast<short>(sampleValue * 32000));
    }

    // --- PACK INTO BYTE VECTOR WITH HEADER ---
    // We use char vector because PlaySound expects a generic byte stream
    std::vector<char> fileBuffer;

    // Calculate sizes
    int dataSize = totalSamples * sizeof(short);
    int headerSize = sizeof(WAVHeader);
    fileBuffer.resize(headerSize + dataSize);

    // Create Header
    WAVHeader header;
    header.data_size = dataSize;
    header.overall_size = dataSize + 36;

    // Copy Header into buffer
    std::memcpy(fileBuffer.data(), &header, headerSize);

    // Copy Audio Data into buffer (after the header)
    std::memcpy(fileBuffer.data() + headerSize, samples.data(), dataSize);

    return fileBuffer;
}
// ---------------------------------------------------------
// FUNCTION: Generate Muted Knock
// Returns a byte vector ready for PlaySound(SND_MEMORY)
// ---------------------------------------------------------
std::vector<char> GenerateMutedKnock() {
    const int SAMPLE_RATE = 44100;
    const int DURATION_MS = 80; // Short duration (80ms) is key for a "knock"
    int totalSamples = (SAMPLE_RATE * DURATION_MS) / 1000;

    // Buffer to hold raw audio numbers first
    std::vector<short> rawSamples;
    rawSamples.reserve(totalSamples);

    double frequency = 790.0; // Start at 120Hz (Low thud)
    double volume = 0.75;      // Start loud
    double phase = 0.0;

    for (int i = 0; i < totalSamples; ++i) {
        // 1. GENERATE SINE WAVE (Smoother than square wave = "Muted")
        double phaseIncrement = frequency / SAMPLE_RATE;
        phase += phaseIncrement;
        if (phase > 1.0) phase -= 1.0;

        // Sine formula: sin( 2 * PI * phase )
        float sampleValue = (FLOAT)sin(phase * 6.28318f);

        // 2. APPLY ENVELOPE (Volume)
        sampleValue *= (FLOAT)volume;

        // 3. MODIFY SOUND CHARACTERISTICS
        // A. Pitch Drop: Drop frequency quickly to simulate an impact
        frequency -= 0.8;
        if (frequency < 20) frequency = 20;

        // B. Fast Decay: The sound must stop abruptly to sound like a knock
        volume -= (0.4 / totalSamples);
        if (volume < 0) volume = 0;

        // 4. STORE (Convert to 16-bit short)
        rawSamples.push_back(static_cast<short>(sampleValue * 32000));
    }

    // --- PACK INTO WAV FORMAT ---
    // Calculate sizes
    int dataSize = totalSamples * sizeof(short);
    int headerSize = sizeof(WAVHeader);

    // Create final byte buffer
    std::vector<char> wavBuffer(headerSize + dataSize);

    // Fill Header
    WAVHeader header;
    header.data_size = dataSize;
    header.overall_size = dataSize + 36;

    // Copy Header + Data into one buffer
    std::memcpy(wavBuffer.data(), &header, headerSize);
    std::memcpy(wavBuffer.data() + headerSize, rawSamples.data(), dataSize);

    return wavBuffer;
}

std::vector<char> GeneratePlasmaShot() {
    const int SAMPLE_RATE = 44100;
    const int DURATION_MS = 250;
    int totalSamples = (SAMPLE_RATE * DURATION_MS) / 1000;

    std::vector<short> rawSamples;
    rawSamples.reserve(totalSamples);

    double frequency = 1300.0; // Start HIGH (1500Hz = piercing tone)
    double volume = 0.73;
    double phase = 0.0;

    for (int i = 0; i < totalSamples; ++i) {
        // 1. UPDATE PHASE
        phase += frequency / SAMPLE_RATE;
        if (phase >= 1.0) phase -= 1.0;

        // 2. GENERATE SAWTOOTH WAVE
        // Math: Linearly goes from -1.0 to 1.0 based on phase
        float sampleValue = (float)(phase * 2.0 - 1.0);

        // 3. ADD "PLASMA JITTER" (Optional)
        // Adding a high-speed sine modulation makes it sound "unstable" and electric
        sampleValue += (float)(sin(phase * 20.0) * 0.3);

        // 4. APPLY VOLUME
        sampleValue *= (FLOAT)volume;

        // 5. FX: EXPONENTIAL PITCH DROP
        // Instead of subtracting (linear), we multiply (exponential).
        // This creates a "Pyuuum" curve rather than a "Boooop" slide.
        frequency *= 0.9997;

        // 6. FX: LINEAR DECAY
        volume -= (0.6 / totalSamples);
        if (volume < 0) volume = 0;

        rawSamples.push_back(static_cast<short>(sampleValue * 32000));
    }

    // --- PACK HEADER ---
    int dataSize = totalSamples * sizeof(short);
    int headerSize = sizeof(WAVHeader);
    std::vector<char> wavBuffer(headerSize + dataSize);

    WAVHeader header;
    header.data_size = dataSize;
    header.overall_size = dataSize + 36;

    std::memcpy(wavBuffer.data(), &header, headerSize);
    std::memcpy(wavBuffer.data() + headerSize, rawSamples.data(), dataSize);

    return wavBuffer;
}

std::vector<char> GeneratePowerupSound() {
    const int SAMPLE_RATE = 44100;
    // A powerup is usually slightly longer (300-400ms) to fit the notes in
    const int DURATION_MS = 400;
    int totalSamples = (SAMPLE_RATE * DURATION_MS) / 1000;

    std::vector<short> rawSamples;
    rawSamples.reserve(totalSamples);

    double phase = 0.0;
    double volume = 0.35;

    // Define a C Major Chord: C5, E5, G5, C6 (One octave up)
    double notes[] = { 523.25, 659.25, 783.99, 1046.50 };
    int noteCount = 4;

    // Calculate how many samples each note gets
    int samplesPerNote = totalSamples / noteCount;

    for (int i = 0; i < totalSamples; ++i) {
        // 1. DETERMINE CURRENT PITCH
        // Based on which "quarter" of the sound we are in, pick the note
        int currentNoteIndex = i / samplesPerNote;
        if (currentNoteIndex >= noteCount) currentNoteIndex = noteCount - 1;

        double currentFreq = notes[currentNoteIndex];

        // 2. OSCILLATOR (Square Wave with 25% Duty Cycle)
        // A 25% duty cycle sounds slightly more "nasal" and "retro" than 50%
        phase += currentFreq / SAMPLE_RATE;
        if (phase >= 1.0) phase -= 1.0;

        float sampleValue = (phase < 0.25) ? 1.0f : -1.0f;

        // 3. ENVELOPE (Volume)
        // We want each note to have a tiny "fade out" to sound distinct,
        // rather than one continuous slur.

        // Calculate progress through the *current note* (0.0 to 1.0)
        int samplesIntoCurrentNote = i % samplesPerNote;
        float noteProgress = (float)samplesIntoCurrentNote / samplesPerNote;

        // Linear decay for each individual note
        float currentVolume = (FLOAT)volume * (1.0f - noteProgress * 0.5f);

        sampleValue *= currentVolume;

        rawSamples.push_back(static_cast<short>(sampleValue * 32000));
    }

    // --- PACK HEADER ---
    int dataSize = totalSamples * sizeof(short);
    int headerSize = sizeof(WAVHeader);
    std::vector<char> wavBuffer(headerSize + dataSize);

    WAVHeader header;
    header.data_size = dataSize;
    header.overall_size = dataSize + 36;

    std::memcpy(wavBuffer.data(), &header, headerSize);
    std::memcpy(wavBuffer.data() + headerSize, rawSamples.data(), dataSize);

    return wavBuffer;
}

std::vector<char> GenerateExplosion() {
    // Seed the random number generator so it sounds different every time
    std::srand(static_cast<unsigned int>(std::time(0)));

    const int SAMPLE_RATE = 44100;
    const int DURATION_MS = 1200; // Explosions are longer (~600ms)
    int totalSamples = (SAMPLE_RATE * DURATION_MS) / 1000;

    std::vector<short> rawSamples;
    rawSamples.reserve(totalSamples);

    double volume = 2.5;

    // --- NOISE PARAMETERS ---
    float currentSample = 0.0f;

    // "Hold" controls the pitch. 
    // If hold = 1, we change the random number every sample (High Hiss)
    // If hold = 20, we keep the same number for 20 samples (Low Rumble)
    double holdDuration = 1.0;
    double holdCounter = 0.0;

    for (int i = 0; i < totalSamples; ++i) {

        // 1. UPDATE THE NOISE GENERATOR
        // Instead of generating a new number every single loop, we wait.
        holdCounter -= 1.0;

        if (holdCounter <= 0) {
            // Generate a new random float between -1.0 and 1.0
            // This is the "White Noise"
            currentSample = (float)(rand() % 2000) / 1000.0f - 1.0f;

            // Reset the counter
            holdCounter = holdDuration;
        }

        // 2. APPLY VOLUME
        float output = currentSample * (FLOAT)volume;

        // 3. FX: CRUNCHY PITCH DROP (The "BOOOOM" effect)
        // We slowly increase the hold duration.
        // This makes the noise get "slower" and "deeper" over time.
        holdDuration += 0.02;

        // 4. FX: EXPONENTIAL DECAY
        // Linear decay sounds unnatural for explosions. 
        // Exponential decay fades out smoothly.
        volume *= 0.99992;
        if (volume < 0) volume = 0;

        rawSamples.push_back(static_cast<short>(output * 32000)); // Slightly lower amplitude to prevent clipping
    }

    // --- PACK HEADER ---
    int dataSize = totalSamples * sizeof(short);
    int headerSize = sizeof(WAVHeader);
    std::vector<char> wavBuffer(headerSize + dataSize);

    WAVHeader header;
    header.data_size = dataSize;
    header.overall_size = dataSize + 36;

    std::memcpy(wavBuffer.data(), &header, headerSize);
    std::memcpy(wavBuffer.data() + headerSize, rawSamples.data(), dataSize);

    return wavBuffer;
}

std::vector<char> GenerateMetallicDing() {
    const int SAMPLE_RATE = 44100;
    const int DURATION_MS = 850; // Metals ring out longer (1 second)
    int totalSamples = (SAMPLE_RATE * DURATION_MS) / 1000;

    std::vector<short> rawSamples;
    rawSamples.reserve(totalSamples);

    // --- SOUND PARAMETERS ---
    double baseFreq = 600.0;  // High pitch fundamental (2kHz)

    // The "Metal" Ratio: 
    // Multiplying by a non-integer (like 2.6 or 3.5) creates the "clang"
    double overtoneFreq = baseFreq * 2.62;

    double phase1 = 0.0;
    double phase2 = 0.0;
    double volume = 0.6;

    for (int i = 0; i < totalSamples; ++i) {
        // 1. UPDATE PHASES
        phase1 += baseFreq / SAMPLE_RATE;
        if (phase1 > 1.0) phase1 -= 1.0;

        phase2 += overtoneFreq / SAMPLE_RATE;
        if (phase2 > 1.0) phase2 -= 1.0;

        // 2. GENERATE SINE WAVES
        float sine1 = (float)sin(phase1 * 6.28318);
        float sine2 = (float)sin(phase2 * 6.28318);

        // 3. MIX THEM
        // We mix the overtone slightly quieter (0.5).
        // This combination creates the metallic timbre.
        float sampleValue = sine1 + (sine2 * 0.35f);

        // 4. APPLY DECAY (The "Ring")
        // Exponential decay mimics real physics better than linear.
        // We multiply volume by 0.99995 every sample.
        volume *= 0.99991;

        // 5. CLAMP AND STORE
        // Since we added two waves, max amplitude could be 1.5, so we divide by 1.5 to normalize
        sampleValue = (sampleValue / 1.5f) * (FLOAT)volume;

        rawSamples.push_back(static_cast<short>(sampleValue * 32000));
    }

    // --- PACK INTO WAV HEADER ---
    int dataSize = totalSamples * sizeof(short);
    int headerSize = sizeof(WAVHeader);
    std::vector<char> wavBuffer(headerSize + dataSize);

    WAVHeader header;
    header.data_size = dataSize;
    header.overall_size = dataSize + 36;

    std::memcpy(wavBuffer.data(), &header, headerSize);
    std::memcpy(wavBuffer.data() + headerSize, rawSamples.data(), dataSize);

    return wavBuffer;
}


std::vector<char> GenerateBallLost() {
    const int SAMPLE_RATE = 44100;
    const int DURATION_MS = 1200; // Long decay (1.2 seconds)
    int totalSamples = (SAMPLE_RATE * DURATION_MS) / 1000;

    std::vector<short> rawSamples;
    rawSamples.reserve(totalSamples);

    // --- SOUND PARAMETERS ---
    double frequency = 600.0; // Start at a mid-tone
    double phase = 0.0;

    // LFO (Low Frequency Oscillator) for the "Wobble" effect
    double lfoPhase = 0.0;
    double lfoSpeed = 30.0; // Starts wobbling fast (30Hz)

    for (int i = 0; i < totalSamples; ++i) {
        // 1. GENERATE MAIN TONE (Sawtooth)
        // Sawtooth has a harsh, buzzy sound suitable for "mechanical failure"
        phase += frequency / SAMPLE_RATE;
        if (phase >= 1.0) phase -= 1.0;
        float baseSample = (float)(phase * 2.0 - 1.0);

        // 2. CALCULATE WOBBLE (Tremolo)
        // This makes the volume go Up/Down rapidly
        lfoPhase += lfoSpeed / SAMPLE_RATE;
        if (lfoPhase >= 1.0) lfoPhase -= 1.0;
        float wobble = (float)sin(lfoPhase * 6.28318);

        // 3. MIX AND APPLY
        // We multiply the base tone by the wobble.
        // (wobble + 2.0) ensures the volume stays mostly positive but pulses.
        float finalSample = baseSample * (wobble * 0.5f + 0.5f);

        // 4. THE "POWER DOWN" PHYSICS
        // A. Pitch drops exponentially (Engine dying)
        frequency *= 0.99994;

        // B. The wobble itself slows down (Spinning parts stopping)
        lfoSpeed *= 0.99990;

        // C. Hard limit frequency to prevent artifacts at the very end
        if (frequency < 20.0) frequency = 0;

        rawSamples.push_back(static_cast<short>(finalSample * 25000));
    }

    // --- PACK INTO WAV HEADER ---
    int dataSize = totalSamples * sizeof(short);
    int headerSize = sizeof(WAVHeader);
    std::vector<char> wavBuffer(headerSize + dataSize);

    WAVHeader header;
    header.data_size = dataSize;
    header.overall_size = dataSize + 36;

    std::memcpy(wavBuffer.data(), &header, headerSize);
    std::memcpy(wavBuffer.data() + headerSize, rawSamples.data(), dataSize);

    return wavBuffer;
}