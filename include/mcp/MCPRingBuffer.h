#pragma once

#include <vector>
#include <cstdlib>
#include <atomic>
#include <algorithm>
#include <cstring>

namespace mcp {

/**
 * @brief A lock-free ring buffer for thread-safe data passing between worker and audio threads.
 * 
 * This implementation is inspired by VCV Rack's dsp::RingBuffer and provides a lock-free
 * mechanism for passing data between the MCP worker thread (which calls onMCPMessage)
 * and the audio thread that processes module data.
 * 
 * @tparam T Data type to store in the ring buffer
 */
template <typename T>
class RingBuffer {
public:
    /**
     * @brief Constructs a RingBuffer with the given capacity.
     * 
     * @param capacity The maximum number of elements the buffer can hold.
     */
    RingBuffer(size_t capacity = 16) : 
        m_capacity(capacity + 1), // Use actual capacity of capacity+1 to distinguish empty from full
        m_buffer(m_capacity) {
        clear();
    }

    /**
     * @brief Returns the number of elements in the buffer.
     * 
     * This is thread-safe but the returned value may change by the time it's used.
     * 
     * @return size_t Number of elements currently in the buffer
     */
    size_t size() const {
        // Get head and tail atomically with appropriate memory ordering
        size_t head = m_head.load(std::memory_order_acquire);
        size_t tail = m_tail.load(std::memory_order_acquire);
        
        if (head >= tail) {
            return head - tail;
        }
        else {
            return m_capacity + head - tail;
        }
    }

    /**
     * @brief Checks if the buffer is empty.
     * 
     * @return true if the buffer is empty, false otherwise
     */
    bool empty() const {
        return m_head.load(std::memory_order_acquire) == 
               m_tail.load(std::memory_order_acquire);
    }

    /**
     * @brief Checks if the buffer is full.
     * 
     * @return true if the buffer is full, false otherwise
     */
    bool full() const {
        size_t head = m_head.load(std::memory_order_acquire);
        size_t nextHead = (head + 1) % m_capacity;
        return nextHead == m_tail.load(std::memory_order_acquire);
    }

    /**
     * @brief Attempts to push an element into the buffer.
     * 
     * This is intended to be called from the producer thread (worker thread).
     * 
     * @param value The value to push
     * @return true if the value was pushed, false if the buffer was full
     */
    bool push(const T& value) {
        // Load head with relaxed ordering as we'll check it again inside the loop
        size_t head = m_head.load(std::memory_order_relaxed);
        size_t nextHead;
        bool success = false;
        
        do {
            nextHead = (head + 1) % m_capacity;
            
            // Check if the buffer is full
            if (nextHead == m_tail.load(std::memory_order_acquire)) {
                return false;
            }
            
            // Try to atomically update head to nextHead if it's still equal to head
            success = m_head.compare_exchange_weak(head, nextHead,
                                                  std::memory_order_release,
                                                  std::memory_order_relaxed);
        } while (!success);
        
        // Store the value
        m_buffer[head] = value;
        
        // Ensure the store to m_buffer is visible before returning
        std::atomic_thread_fence(std::memory_order_release);
        
        return true;
    }

    /**
     * @brief Attempts to pop an element from the buffer.
     * 
     * This is intended to be called from the consumer thread (audio thread).
     * 
     * @param value Reference where the popped value will be stored
     * @return true if a value was popped, false if the buffer was empty
     */
    bool pop(T& value) {
        // Load tail with relaxed ordering as we'll check it again inside the loop
        size_t tail = m_tail.load(std::memory_order_relaxed);
        size_t nextTail;
        
        do {
            // Check if the buffer is empty
            if (tail == m_head.load(std::memory_order_acquire)) {
                return false;
            }
            
            // Read the value
            value = m_buffer[tail];
            
            // Calculate the next tail position
            nextTail = (tail + 1) % m_capacity;
            
            // Try to atomically update tail if it hasn't changed
        } while (!m_tail.compare_exchange_weak(tail, nextTail,
                                              std::memory_order_release,
                                              std::memory_order_relaxed));
        
        return true;
    }

    /**
     * @brief Clears the buffer, removing all elements.
     * 
     * This is not thread-safe and should only be called when no
     * other threads are accessing the buffer.
     */
    void clear() {
        m_head.store(0, std::memory_order_relaxed);
        m_tail.store(0, std::memory_order_relaxed);
    }

private:
    const size_t m_capacity;
    std::vector<T> m_buffer;
    std::atomic<size_t> m_head; // Producer writes here
    std::atomic<size_t> m_tail; // Consumer reads from here
};

} // namespace mcp 