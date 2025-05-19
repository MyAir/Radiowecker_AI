#pragma once

#include <Arduino.h>
#include "AudioGenerator.h"
#include "AudioOutputI2S.h"
#include "AudioFileSourceSD.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioFileSourceHTTPStream.h"
#include <SD_MMC.h>

// Forward declarations
class AudioGenerator;
class AudioFileSource;
class AudioOutputI2S;
class AudioFileSourceBuffer;

class AudioManager {
private:
    static AudioManager* instance;
    
    // Private constructor and destructor
    AudioManager() = default;
    ~AudioManager();
    
    // Prevent copying and assignment
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;
    
public:
    static AudioManager& getInstance() {
        if (!instance) {
            instance = new AudioManager();
        }
        return *instance;
    }

    void begin();
    void loop();
    
    bool playStream(const char* url);
    bool playFile(const char* filename);
    void stop();
    
    void setVolume(uint8_t volume);
    uint8_t getVolume() const { return currentVolume; }
    
    bool isPlaying() const { return (audioGenerator && audioGenerator->isRunning()); }
    
    // Callback types
    typedef void (*PlaybackStateCallback)(bool isPlaying);
    void setPlaybackStateCallback(PlaybackStateCallback cb) { playbackStateCallback = cb; }
    
    // Buffer settings
    void setBufferSize(size_t size) { bufferSize = size; }
    void setPreBufferPercent(uint8_t percent) { preBufferPercent = constrain(percent, 10, 90); }

private:

    
    // Audio components
    AudioGenerator *audioGenerator = nullptr;
    AudioFileSource *fileSource = nullptr;
    AudioFileSourceBuffer *bufferedSource = nullptr;
    AudioOutputI2S *audioOutput = nullptr;
    
    // Playback state
    uint8_t currentVolume = 50;
    bool isStreaming = false;
    unsigned long lastStateChange = 0;
    
    // Buffer settings
    size_t bufferSize = 16 * 1024;  // 16KB buffer
    uint8_t preBufferPercent = 50;  // 50% pre-buffer before starting playback
    
    // Callbacks
    PlaybackStateCallback playbackStateCallback = nullptr;
    
    // Internal methods
    void cleanup();
    void notifyPlaybackState(bool isPlaying);
};

// Initialize static member
inline AudioManager* AudioManager::instance = nullptr;
