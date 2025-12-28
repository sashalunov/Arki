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
        sampleValue *= volume;

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
    double volume = 0.5;      // Start loud
    double phase = 0.0;

    for (int i = 0; i < totalSamples; ++i) {
        // 1. GENERATE SINE WAVE (Smoother than square wave = "Muted")
        double phaseIncrement = frequency / SAMPLE_RATE;
        phase += phaseIncrement;
        if (phase > 1.0) phase -= 1.0;

        // Sine formula: sin( 2 * PI * phase )
        float sampleValue = sin(phase * 6.28318f);

        // 2. APPLY ENVELOPE (Volume)
        sampleValue *= volume;

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
    double volume = 0.23;
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
        sampleValue *= volume;

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
    double volume = 0.15;

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
        float currentVolume = volume * (1.0f - noteProgress * 0.5f);

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

