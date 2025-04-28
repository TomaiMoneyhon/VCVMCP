#include "mcp/MCPReferenceProvider.h"
#include <iostream>
#include <random>
#include <cmath>

namespace mcp {

MCPReferenceProvider::MCPReferenceProvider(int id)
    : rack::Module(id), 
      m_topics({
          "reference/parameter1",     // Single float parameter
          "reference/parameter2",     // Single float parameter
          "reference/preset",         // String value
          "reference/parameters"      // Array of parameters
      })
{
    // Initialize with default values (done in header)
}

MCPReferenceProvider::~MCPReferenceProvider() {
    // Make sure the publishing thread is stopped before any other cleanup
    bool wasRunning = m_publishRunning.exchange(false);
    if (wasRunning) {
        // Signal condition variable
        {
            std::lock_guard<std::mutex> lock(m_publishMutex);
            m_publishCondition.notify_all();
        }
        
        // Join the thread if it's joinable
        if (m_publishThread.joinable()) {
            m_publishThread.join();
        }
        
        std::cout << "Provider " << getId() << " stopped periodic publishing" << std::endl;
    }
    
    // Do not attempt to unregister here - should be done in onRemove
    // This avoids using shared_from_this during destruction, which can cause bad_weak_ptr
}

std::vector<std::string> MCPReferenceProvider::getProvidedTopics() const {
    return m_topics;
}

void MCPReferenceProvider::onAdd() {
    rack::Module::onAdd();
    
    // Get broker instance
    auto broker = MCPBroker::getInstance();
    if (!broker) {
        std::cerr << "Failed to get broker instance" << std::endl;
        return;
    }
    
    // Create a local shared_ptr to this object for registration
    auto selfPtr = std::dynamic_pointer_cast<IMCPProvider_V1>(rack::Module::shared_from_this());
    
    if (!selfPtr) {
        std::cerr << "Failed to get shared_ptr to provider" << std::endl;
        return;
    }
    
    // Register for all topics
    for (const auto& topic : m_topics) {
        broker->registerContext(topic, selfPtr);
        std::cout << "Provider " << getId() << " registered for topic: " << topic << std::endl;
    }
    
    // Start publishing thread with 1 second interval
    startPeriodicPublishing(1000);
}

void MCPReferenceProvider::onRemove() {
    // Stop the publishing thread
    stopPeriodicPublishing();
    
    // Get broker instance
    auto broker = MCPBroker::getInstance();
    if (!broker) {
        std::cerr << "Failed to get broker instance" << std::endl;
        return;
    }
    
    // Create a local shared_ptr to this object for unregistration
    auto selfPtr = std::dynamic_pointer_cast<IMCPProvider_V1>(rack::Module::shared_from_this());
    
    if (!selfPtr) {
        std::cerr << "Failed to get shared_ptr to provider for unregistration" << std::endl;
        // Continue with cleanup even if we can't get the shared_ptr
    }
    else {
        // Unregister from all topics
        for (const auto& topic : m_topics) {
            broker->unregisterContext(topic, selfPtr);
            std::cout << "Provider " << getId() << " unregistered from topic: " << topic << std::endl;
        }
    }
    
    rack::Module::onRemove();
}

void MCPReferenceProvider::process(float* outputs, int frames) {
    // This method is called from the audio thread
    // We don't do any MCP work here, just demonstrate thread identification
    auto threadType = rack::engine::getThreadType();
    if (threadType != rack::engine::AUDIO_THREAD) {
        std::cerr << "Warning: process() called from non-audio thread!" << std::endl;
    }
    
    // In a real module, you would process audio here
    // For this example, we just zero the output buffer
    for (int i = 0; i < frames; ++i) {
        outputs[i] = 0.0f;
    }
}

void MCPReferenceProvider::startPeriodicPublishing(int intervalMs) {
    // Lock to ensure thread-safe update of publishing state
    std::lock_guard<std::mutex> lock(m_publishMutex);
    
    // Set the interval
    m_publishInterval = intervalMs;
    
    // If already running, no need to start again
    if (m_publishRunning) {
        return;
    }
    
    // Mark as running and start the thread
    m_publishRunning = true;
    m_publishThread = std::thread(&MCPReferenceProvider::publishThreadFunc, this);
    
    std::cout << "Provider " << getId() << " started periodic publishing" << std::endl;
}

void MCPReferenceProvider::stopPeriodicPublishing() {
    // Set the flag to stop
    m_publishRunning = false;
    
    // Notify the thread to wake up if it's waiting
    {
        std::lock_guard<std::mutex> lock(m_publishMutex);
        m_publishCondition.notify_all();
    }
    
    // Wait for the thread to exit
    if (m_publishThread.joinable()) {
        m_publishThread.join();
    }
    
    std::cout << "Provider " << getId() << " stopped periodic publishing" << std::endl;
}

void MCPReferenceProvider::updateParameters() {
    // Simple synthetic parameter updates
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    // Update parameter 1 (simple sine wave oscillation)
    static float phase = 0.0f;
    m_parameter1 = 0.5f + 0.5f * std::sin(phase);
    phase += 0.05f;
    
    // Update parameter 2 (random walk with occasional sudden changes)
    if (dist(gen) < 0.05f) {
        // Occasionally make a sudden jump
        m_parameter2 = dist(gen);
    } else {
        // Usually do a small random walk
        float change = 0.1f * (dist(gen) - 0.5f);
        m_parameter2 = std::max(0.0f, std::min(1.0f, m_parameter2 + change));
    }
    
    // Update preset name occasionally
    static int presetCounter = 0;
    if (++presetCounter % 10 == 0) {
        static const std::vector<std::string> presetNames = {
            "Warm Pad", "Bright Lead", "Deep Bass", "Plucky Keys", "Ambient Texture",
            "Synth Brass", "Clean Piano", "Evolving Scape", "Percussive Pluck", "Sequenced Arp"
        };
        m_preset = presetNames[presetCounter % presetNames.size()];
    }
    
    // Update parameter array with different patterns for each element
    for (size_t i = 0; i < m_parameterArray.size(); ++i) {
        // Different patterns for different array elements
        switch (i % 5) {
            case 0: // Sine oscillation
                m_parameterArray[i] = 0.5f + 0.4f * std::sin(phase + i * 0.5f);
                break;
            case 1: // Random walk
                m_parameterArray[i] += 0.08f * (dist(gen) - 0.5f);
                m_parameterArray[i] = std::max(0.0f, std::min(1.0f, m_parameterArray[i]));
                break;
            case 2: // Sawtooth pattern
                m_parameterArray[i] += 0.01f;
                if (m_parameterArray[i] > 1.0f) m_parameterArray[i] = 0.0f;
                break;
            case 3: // Square wave
                if (presetCounter % 20 == 0) {
                    m_parameterArray[i] = m_parameterArray[i] < 0.5f ? 1.0f : 0.0f;
                }
                break;
            case 4: // Gradual fade
                m_parameterArray[i] = 0.8f * m_parameterArray[i] + 0.2f * dist(gen);
                break;
        }
    }
}

void MCPReferenceProvider::publishThreadFunc() {
    // Set thread type for proper identification
    rack::engine::setThreadType(rack::engine::WORKER_THREAD);
    
    int publishCount = 0;
    std::cout << "Provider " << getId() << " publishing thread started" << std::endl;
    
    while (m_publishRunning) {
        try {
            // Update the parameters
            updateParameters();
            
            // Publish each parameter on its topic
            publishMessage("reference/parameter1", m_parameter1);
            publishMessage("reference/parameter2", m_parameter2);
            publishMessage("reference/preset", m_preset);
            publishMessage("reference/parameters", m_parameterArray);
            
            // Increment publish count
            publishCount++;
            
            // Log occasionally
            if (publishCount % 10 == 0) {
                std::cout << "Provider " << getId() << " published " << publishCount 
                          << " updates, latest parameter1: " << m_parameter1 << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error in publisher thread: " << e.what() << std::endl;
            // Continue running despite error
        }
        
        // Wait for the specified interval or until stopped
        std::unique_lock<std::mutex> lock(m_publishMutex);
        m_publishCondition.wait_for(lock, 
                                   std::chrono::milliseconds(m_publishInterval),
                                   [this]{ return !m_publishRunning; });
    }
    
    std::cout << "Provider " << getId() << " publishing thread stopped after " 
              << publishCount << " updates" << std::endl;
}

} // namespace mcp 