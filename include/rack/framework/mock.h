#pragma once

#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <cstdint>

// Simple mock of VCV Rack framework classes to use for reference implementation
namespace rack {

/**
 * @brief Mock implementation of VCV Rack's engine-related types
 * 
 * This namespace contains a simple mock of the engine and related types
 * for use in the reference implementation. It provides enough functionality
 * to demonstrate thread-safety and integration with the MCP framework.
 */
namespace engine {
    // Thread types
    enum ThreadType {
        AUDIO_THREAD,
        UI_THREAD,
        WORKER_THREAD,
        UNKNOWN_THREAD
    };

    // Current thread indicator
    extern thread_local ThreadType currentThread;

    // Audio sample rate
    extern float sampleRate;

    /**
     * @brief Identifies which thread the current code is running on
     * @return ThreadType indicating current thread
     */
    ThreadType getThreadType();

    /**
     * @brief Sets the current thread type
     * @param type ThreadType to set for the current thread
     */
    void setThreadType(ThreadType type);

    /**
     * @brief Simulates the audio thread processing
     * @param callback Function to call for audio processing
     * @param frames Number of frames to process
     */
    void processAudio(std::function<void(float*, int)> callback, int frames);
} // namespace engine

/**
 * @brief Base class for all modules in VCV Rack
 * 
 * This is a simplified mock of the Module class used in VCV Rack.
 * It provides just enough functionality to demonstrate integration
 * with the MCP system and proper thread handling.
 */
class Module : public std::enable_shared_from_this<Module> {
public:
    /**
     * @brief Constructor
     * @param id Unique ID for this module instance
     */
    explicit Module(int id = -1);

    /**
     * @brief Virtual destructor
     */
    virtual ~Module();

    /**
     * @brief Get the module ID
     * @return Unique ID for this module
     */
    int getId() const;

    /**
     * @brief Set the module ID
     * @param id Unique ID to assign to this module
     */
    void setId(int id);

    /**
     * @brief Called when module is added to engine
     */
    virtual void onAdd();

    /**
     * @brief Called when module is removed from engine
     */
    virtual void onRemove();

    /**
     * @brief Called to process audio
     * @param outputs Pointer to output buffer
     * @param frames Number of frames to process
     * 
     * This method is always called from the audio thread.
     */
    virtual void process(float* outputs, int frames);

protected:
    int id;
    std::atomic<bool> added{false};
};

} // namespace rack 