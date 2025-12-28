#pragma once
#include"stdafx.h"


inline void PlayAudioFromMemory(const std::vector<char>& wavData) {
	// 1. ptr: Pointer to the raw bytes in memory
	// 2. hmod: NULL (not loading from an executable resource)
	// 3. fdwSound: Flags
	//    - SND_MEMORY: First parameter is a pointer to memory, not a filename
	//    - SND_ASYNC: Play in background (don't freeze the app)
	//    - SND_NODEFAULT: Don't play the system "ding" if it fails

	PlaySound(
		(LPCWSTR)wavData.data(),
		NULL,
		SND_MEMORY | SND_ASYNC | SND_NODEFAULT
	);
}

// Standard WAV Header Struct
struct WAVHeader {
    char riff[4] = { 'R', 'I', 'F', 'F' };
    int overall_size;
    char wave[4] = { 'W', 'A', 'V', 'E' };
    char fmt_chunk_marker[4] = { 'f', 'm', 't', ' ' };
    int length_of_fmt = 16;
    short format_type = 1;
    short channels = 1;
    int sample_rate = 44100;
    int byterate = 44100 * 2;
    short block_align = 2;
    short bits_per_sample = 16;
    char data_chunk_header[4] = { 'd', 'a', 't', 'a' };
    int data_size;
};

std::vector<char> GenerateJumpSound();
std::vector<char> GenerateMutedKnock();
std::vector<char> GeneratePlasmaShot();
std::vector<char> GeneratePowerupSound();


