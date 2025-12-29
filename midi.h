#pragma once
#include "stdafx.h"


HMIDIOUT hMidiOut;

// --- MIDI HELPERS ---
void SendMidiMsg(unsigned char status, unsigned char data1, unsigned char data2) 
{
    DWORD message = status | (data1 << 8) | (data2 << 16);
    midiOutShortMsg(hMidiOut, message);
}

void SetInstrument(unsigned char channel, unsigned char instrumentId) 
{
    // Status 0xC0 + Channel = Program Change
    SendMidiMsg(0xC0 | channel, instrumentId, 0);
}

void NoteOn(unsigned char channel, unsigned char note, unsigned char velocity) 
{
    SendMidiMsg(0x90 | channel, note, velocity);
}

void NoteOff(unsigned char channel, unsigned char note) 
{
    SendMidiMsg(0x80 | channel, note, 0);
}

// --- PLASMA EFFECT FUNCTION ---
void PlayMidiPlasma() 
{
    // 1. SELECT INSTRUMENT
    // Channel 0. Instrument 81 is "Lead 2 (Sawtooth)" - ideal for energy weapons.
    // Try 80 (Square) for a more NES-Mario fireball sound.
    SetInstrument(0, 81);

    // 2. THE "PEW" LOOP (Glissando)
    // Start at a high note (85) and slide down to a mid note (45)
    // Decreasing by 2 semitones each step makes it faster/smoother.
    int startNote = 90;
    int endNote = 40;

    for (int note = startNote; note > endNote; note -= 2) {
        // Play note
        NoteOn(0, note, 127); // Max velocity (loudness)

        // Wait extremely briefly (this creates the slide effect)
        Sleep(15);

        // Stop note (cleanup) so we don't have 50 notes playing at once
        NoteOff(0, note);
    }
}

