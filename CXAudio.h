#pragma once
#include "stdafx.h"
#include "Logger.h"
#include "btBulletDynamicsCommon.h"
#include <mmdeviceapi.h>

// Helper struct to hold audio data
struct SoundData {
    WAVEFORMATEX wfx = { 0 };
    std::vector<BYTE> audioBytes;
};
static const X3DAUDIO_CONE Listener_DirectionalCone = { X3DAUDIO_PI * 5.0f / 6.0f, X3DAUDIO_PI * 11.0f / 6.0f, 1.0f, 0.75f, 0.0f, 0.25f, 0.708f, 1.0f };

class CXAudio
{
private:
    IXAudio2* pXAudio2;
    IXAudio2MasteringVoice* pMasterVoice;
    std::map<std::string, SoundData> soundBank;
    X3DAUDIO_HANDLE x3DInstance; // The 3D Engine Handle
    btVector3 m_listenerPos;

public:
    CXAudio() : pXAudio2(nullptr), pMasterVoice(nullptr) {
    }

    ~CXAudio() { 
        Shutdown();
    }

    // 1. Initialize the Engine
    BOOL Initialize() {
        HRESULT hr;

        // Initialize COM (Required for XAudio2)
        hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr)) return false;

        IMMDeviceEnumerator* pEnum = nullptr;
        CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator), (void**)&pEnum);

        IMMDevice* pDevice = nullptr;
        pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice); // default output device

        LPWSTR pwszID = nullptr;
        pDevice->GetId(&pwszID); // device ID string


		_log(L"Using Audio Device ID: %s\n", pwszID);

        // Create the XAudio2 Engine
        hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
        if (FAILED(hr)) return false;

        // Create the Mastering Voice (The "Speakers")
        hr = pXAudio2->CreateMasteringVoice(&pMasterVoice,0U,0U,0U, pwszID);
        if (FAILED(hr)) return false;

        // - Initialize X3DAudio ---
        // We need to know the channel mask (e.g., Stereo vs 5.1 Surround)
        DWORD channelMask;
        pMasterVoice->GetChannelMask(&channelMask);

        // This handle is used for all 3D calculations
        X3DAudioInitialize(channelMask, X3DAUDIO_SPEED_OF_SOUND, x3DInstance);
        return true;
    }

    void Shutdown()
    {
        if (pMasterVoice) {
            pMasterVoice->DestroyVoice();
            pMasterVoice = nullptr;
        }
        if (pXAudio2) {
            pXAudio2->Release();
            pXAudio2 = nullptr;
        }
		CoUninitialize();
    }

    void SetListenerPos(btVector3 v)
    {
        m_listenerPos = v;
    }
    // --------------------------------------------------------
    // NEW: Load raw WAV bytes directly from memory (RAM)
    // --------------------------------------------------------
    BOOL LoadSoundFromMemory(const std::string& id, const std::vector<char>& rawData) {
        if (rawData.empty()) return false;

        SoundData data;

        // We need to manually parse the WAV header from the byte vector
        // Pointers for reading through the buffer
        const char* ptr = rawData.data();
        const char* end = ptr + rawData.size();

        // 1. Check RIFF Header
        if (memcmp(ptr, "RIFF", 4) != 0) return false;
        ptr += 8; // Skip "RIFF" and Total Size
        if (memcmp(ptr, "WAVE", 4) != 0) return false;
        ptr += 4; // Skip "WAVE"

        // 2. Scan Chunks
        while (ptr < end) {
            // Read Chunk ID and Size
            char chunkId[4];
            memcpy(chunkId, ptr, 4);
            ptr += 4;

            int chunkSize;
            memcpy(&chunkSize, ptr, 4);
            ptr += 4;

            if (memcmp(chunkId, "fmt ", 4) == 0) {
                // Found Format Chunk - Copy it to our XAudio structure
                memcpy(&data.wfx, ptr, sizeof(WAVEFORMATEX));
            }
            else if (memcmp(chunkId, "data", 4) == 0) {
                // Found Data Chunk - Copy the actual sound waves
                data.audioBytes.resize(chunkSize);
                memcpy(data.audioBytes.data(), ptr, chunkSize);

                // We have what we need, save it to the bank and return
                soundBank[id] = data;
                return true;
            }

            // Move pointer to next chunk
            ptr += chunkSize;
        }

        return false; // Failed to find data chunk
    }
    // 2. Load a WAV file into memory
    // Returns 'true' if successful
    BOOL LoadSound(const std::string& id, const std::wstring& filename) {
        SoundData data;
        if (!LoadWAVFile(filename, data)) {
            //_log( L"Failed to load WAV: %s %s",filename.begin(), filename.end());
            return false;
        }
        soundBank[id] = data;
        return true;
    }

    // -- - NEW: Play 3D Sound-- -
    void Play3D(const std::string & id, btVector3 soundPos) {
        if (soundBank.find(id) == soundBank.end()) return;
        SoundData& sound = soundBank[id];

        // 1. Create Voice (Must be MONO for best 3D results)
        IXAudio2SourceVoice* pSourceVoice = nullptr;
        if (FAILED(pXAudio2->CreateSourceVoice(&pSourceVoice, &sound.wfx))) return;

        // 2. Submit Audio Data
        XAUDIO2_BUFFER buffer = { 0 };
        buffer.AudioBytes = (UINT32)sound.audioBytes.size();
        buffer.pAudioData = sound.audioBytes.data();
        buffer.Flags = XAUDIO2_END_OF_STREAM;
        pSourceVoice->SubmitSourceBuffer(&buffer);

        // 3. CALCULATE 3D SETTINGS
        X3DAUDIO_LISTENER listener = {};
        listener.OrientFront = { 0.0f, 0.0f, 1.0f };
        listener.OrientTop   = { 0.0f, 1.0f, 0.0f };
        listener.Position    = { static_cast<float>(m_listenerPos.x()), static_cast<float>(m_listenerPos.y()), static_cast<float>(m_listenerPos.z()) };
        listener.Velocity    = { 0.0f, 0.0f, 0.0f };
        listener.pCone       = (X3DAUDIO_CONE*)&Listener_DirectionalCone;

        X3DAUDIO_EMITTER emitter = { 0 };
        // Setup Emitter (Sound Source)
        emitter.Position = { static_cast<float>(soundPos.x()), 
            static_cast<float>(soundPos.y()), 
            static_cast<float>(soundPos.z())};
        emitter.OrientFront = { 0, 0, 1 };
        emitter.OrientTop = { 0, 1, 0 };
        emitter.pCone = nullptr;

        // IMPORTANT: Tell X3DAudio about the sound file format
        emitter.ChannelCount = sound.wfx.nChannels;
        emitter.CurveDistanceScaler = 10.0f; // 1 unit = 1 meter

        // Prepare the DSP Settings (Volume Matrix)
        X3DAUDIO_DSP_SETTINGS dspSettings = { 0 };
        // We need to calculate volume for the Master Voice input channels (usually 2 for stereo speakers)
        FLOAT32 matrix[8];
        dspSettings.SrcChannelCount = sound.wfx.nChannels;
        dspSettings.DstChannelCount = 2; // Assuming Stereo Output
        dspSettings.pMatrixCoefficients = matrix;

        // Perform the Math
        // FLAGS: Calculate Matrix (Panning) and Doppler (Pitch Shift)
        X3DAudioCalculate(x3DInstance, &listener, &emitter,
            X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER,
            &dspSettings);

        // 4. APPLY 3D SETTINGS TO VOICE
        // Apply Volume Panning
        pSourceVoice->SetOutputMatrix(pMasterVoice, sound.wfx.nChannels, 2, dspSettings.pMatrixCoefficients);

        // Apply Doppler (Pitch shift based on speed - simplified here)
        pSourceVoice->SetFrequencyRatio(dspSettings.DopplerFactor);

        // 5. Play
        pSourceVoice->Start(0);
    }

    // 3. Play a sound by ID
    void Play(const std::string& id) {
        if (soundBank.find(id) == soundBank.end()) return;

        SoundData& sound = soundBank[id];
        IXAudio2SourceVoice* pSourceVoice = nullptr;

        // A. Create a "Source Voice" (The player for this specific sound)
        if (FAILED(pXAudio2->CreateSourceVoice(&pSourceVoice, &sound.wfx))) return;

        // B. Submit the Audio Data to the voice
        XAUDIO2_BUFFER buffer = { 0 };
        buffer.AudioBytes = (UINT32)sound.audioBytes.size();
        buffer.pAudioData = sound.audioBytes.data();
        buffer.Flags = XAUDIO2_END_OF_STREAM; // Tell it this is the whole sound

        if (FAILED(pSourceVoice->SubmitSourceBuffer(&buffer))) {
            pSourceVoice->DestroyVoice();
            return;
        }

        // C. Start Playing
        pSourceVoice->Start(0);

        // NOTE: In a real engine, you should track 'pSourceVoice' and destroy it 
        // when the sound finishes. For this simple example, we let XAudio2 handle it 
        // loosely, but strictly speaking, this leaks voice objects if you play infinite sounds.
        // A common fix is to re-use a pool of pre-created voices.
    }


    // --- INTERNAL WAV PARSER ---
    // Minimal RIFF/WAVE parser to avoid external dependencies
    bool LoadWAVFile(const std::wstring& filename, SoundData& outData) {
        HANDLE hFile = CreateFileW(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile == INVALID_HANDLE_VALUE) return false;

        DWORD dwChunkId = 0, dwChunkSize = 0, dwBytesRead = 0, dwRiffType = 0;

        // 1. Read RIFF Header
        ReadFile(hFile, &dwChunkId, 4, &dwBytesRead, NULL);
        ReadFile(hFile, &dwChunkSize, 4, &dwBytesRead, NULL);
        ReadFile(hFile, &dwRiffType, 4, &dwBytesRead, NULL);

        // Check for "RIFF" and "WAVE" tags
        if (dwChunkId != 'FFIR' || dwRiffType != 'EVAW') { // 'RIFF', 'WAVE' in little-endian
            CloseHandle(hFile); return false;
        }

        // 2. Scan for chunks (fmt, data)
        while (ReadFile(hFile, &dwChunkId, 4, &dwBytesRead, NULL) && dwBytesRead > 0) {
            ReadFile(hFile, &dwChunkSize, 4, &dwBytesRead, NULL);

            if (dwChunkId == ' tmf') { // "fmt "
                // Read format
                ReadFile(hFile, &outData.wfx, sizeof(WAVEFORMATEX), &dwBytesRead, NULL);
                // Skip remaining format bytes if any (e.g. alignment bytes)
                if (dwChunkSize > sizeof(WAVEFORMATEX)) {
                    SetFilePointer(hFile, dwChunkSize - sizeof(WAVEFORMATEX), NULL, FILE_CURRENT);
                }
            }
            else if (dwChunkId == 'atad') { // "data"
                // Read audio data
                outData.audioBytes.resize(dwChunkSize);
                ReadFile(hFile, outData.audioBytes.data(), dwChunkSize, &dwBytesRead, NULL);
                CloseHandle(hFile);
                return true; // Success!
            }
            else {
                // Skip unknown chunk
                SetFilePointer(hFile, dwChunkSize, NULL, FILE_CURRENT);
            }
        }
        CloseHandle(hFile);
        return false;
    }
};
extern CXAudio* xau;