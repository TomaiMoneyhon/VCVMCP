#pragma once

#include <vector>
#include <cstdlib>
#include <atomic>
#include <algorithm>
#include <cstring>

namespace mcp {

/**
 * @brief A lock-free ring buffer specifically designed for Single-Producer/Single-Consumer (SPSC) scenarios.
 * 
 * IMPORTANT: This implementation is ONLY thread-safe when used with:
 * - ONE thread calling push() (producer thread)
 * - ONE thread calling pop() (consumer thread)
 * 
 * Usage pattern in MCP:
 * - Worker thread (producer) calls push() in onMCPMessage
 * - Audio thread (consumer) calls pop() in process()
 * 
 * Thread Safety Requirements:
 * - NEVER call push() from multiple threads
 * - NEVER call pop() from multiple threads
 * - NEVER call push() and pop() from the same thread
 * - ONLY the producer thread should call push()
 * - ONLY the consumer thread should call pop()
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
     *                 One slot is always kept empty to distinguish between empty and full states.
     */
    RingBuffer(size_t capacity = 16) : 
        m_buffer(capacity + 1) {
        // Actual capacity is capacity + 1 (one slot always kept empty)
        m_capacity = capacity + 1;
        
        // Initialize indices
        clear();
    }

    /**
     * @brief Returns the number of elements in the buffer.
     * 
     * Thread-safe, but the result may be outdated by the time it's used.
     * 
     * @return size_t Number of elements currently in the buffer
     */
    size_t size() const {
        // Use memory_order_seq_cst for both loads to ensure consistency
        size_t head = m_head.load(std::memory_order_seq_cst);
        size_t tail = m_tail.load(std::memory_order_seq_cst);
        
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
        // Use memory_order_seq_cst for consistent state observation
        return m_head.load(std::memory_order_seq_cst) == 
               m_tail.load(std::memory_order_seq_cst);
    }

    /**
     * @brief Checks if the buffer is full.
     * 
     * @return true if the buffer is full, false otherwise
     */
    bool full() const {
        // Use memory_order_seq_cst for consistent state observation
        size_t head = m_head.load(std::memory_order_seq_cst);
        size_t tail = m_tail.load(std::memory_order_seq_cst);
        
        return (head + 1) % m_capacity == tail;
    }

    /**
     * @brief Attempts to push an element into the buffer.
     * 
     * IMPORTANT: This method must ONLY be called by the producer thread.
     * 
     * @param value The value to push
     * @return true if the value was pushed, false if the buffer was full
     */
    bool push(const T& value) {
        // Load head with relaxed ordering - only producer thread updates this
        const size_t head = m_head.load(std::memory_order_relaxed);
        const size_t next_head = (head + 1) % m_capacity;
        
        // Check if buffer is full - need seq_cst to ensure accurate tail observation
        if (next_head == m_tail.load(std::memory_order_seq_cst)) {
            return false;  // Buffer is full
        }
        
        // Store the value
        m_buffer[head] = value;
        
        // Memory barrier to ensure value is visible to consumer
        std::atomic_thread_fence(std::memory_order_seq_cst);
        
        // Update head with release semantics to make the change visible
        m_head.store(next_head, std::memory_order_seq_cst);
        
        return true;
    }

    /**
     * @brief Attempts to pop an element from the buffer.
     * 
     * IMPORTANT: This method must ONLY be called by the consumer thread.
     * 
     * @param value Reference where the popped value will be stored
     * @return true if a value was popped, false if the buffer was empty
     */
    bool pop(T& value) {
        // Load tail with relaxed ordering - only consumer thread updates this
        const size_t tail = m_tail.load(std::memory_order_relaxed);
        
        // Check if buffer is empty - need seq_cst to ensure accurate head observation
        if (tail == m_head.load(std::memory_order_seq_cst)) {
            return false;  // Buffer is empty
        }
        
        // Read the value
        value = m_buffer[tail];
        
        // Memory barrier to ensure proper ordering
        std::atomic_thread_fence(std::memory_order_seq_cst);
        
        // Update tail with release semantics
        m_tail.store((tail + 1) % m_capacity, std::memory_order_seq_cst);
        
        return true;
    }

    /**
     * @brief Clears the buffer, removing all elements.
     * 
     * NOT thread-safe - only call when no other threads are accessing the buffer.
     */
    void clear() {
        m_head.store(0, std::memory_order_seq_cst);
        m_tail.store(0, std::memory_order_seq_cst);
    }

private:
    size_t m_capacity;
    std::vector<T> m_buffer;
    
    // Atomics for thread-safe access
    // IMPORTANT: Only the producer updates head, only the consumer updates tail
    alignas(64) std::atomic<size_t> m_head;  // Cache line alignment to prevent false sharing
    alignas(64) std::atomic<size_t> m_tail;  // Cache line alignment to prevent false sharing
};

} // namespace mcp 