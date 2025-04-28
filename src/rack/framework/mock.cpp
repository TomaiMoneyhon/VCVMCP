#include "rack/framework/mock.h"
#include <iostream>

namespace rack {

namespace engine {
    thread_local ThreadType currentThread = UNKNOWN_THREAD;
    float sampleRate = 44100.0f;

    ThreadType getThreadType() {
        return currentThread;
    }

    void setThreadType(ThreadType type) {
        currentThread = type;
    }

    void processAudio(std::function<void(float*, int)> callback, int frames) {
        // Save current thread type
        ThreadType previousType = currentThread;
        
        // Set to audio thread for processing
        currentThread = AUDIO_THREAD;
        
        // Allocate buffer for audio processing
        float* buffer = new float[frames];
        
        // Call the processing callback
        callback(buffer, frames);
        
        // Cleanup
        delete[] buffer;
        
        // Restore previous thread type
        currentThread = previousType;
    }
} // namespace engine

// Module implementation
Module::Module(int id) : id(id) {}

Module::~Module() {
    if (added) {
        std::cerr << "Warning: Module " << id << " destroyed while still added to engine" << std::endl;
    }
}

int Module::getId() const {
    return id;
}

void Module::setId(int id) {
    this->id = id;
}

void Module::onAdd() {
    added = true;
}

void Module::onRemove() {
    added = false;
}

void Module::process(float* outputs, int frames) {
    // Default implementation does nothing
}

} // namespace rack 