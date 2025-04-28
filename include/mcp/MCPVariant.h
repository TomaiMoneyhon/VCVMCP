#pragma once

#include <string>
#include <vector>
#include <type_traits>
#include <stdexcept>
#include <utility>
#include <cassert>
#include <memory>

namespace mcp {

/**
 * @brief A simple type-safe union replacement for std::variant (C++17) that supports float, string, and vector<float>
 * 
 * This is a simplified implementation for our specific needs since the project is using C++14.
 */
class MessageVariant {
public:
    enum Type {
        FLOAT,
        STRING,
        VECTOR_FLOAT,
        EMPTY
    };
    
    // Default constructor - empty variant
    MessageVariant() : m_type(EMPTY) {}

    // Destructor
    ~MessageVariant() {
        clear();
    }
    
    // Copy constructor
    MessageVariant(const MessageVariant& other) : m_type(EMPTY) {
        *this = other;
    }
    
    // Move constructor
    MessageVariant(MessageVariant&& other) noexcept : m_type(EMPTY) {
        *this = std::move(other);
    }
    
    // Copy assignment
    MessageVariant& operator=(const MessageVariant& other) {
        if (this != &other) {
            clear();
            m_type = other.m_type;
            switch (m_type) {
                case FLOAT:
                    m_float = other.m_float;
                    break;
                case STRING:
                    new (&m_string) std::string(other.m_string);
                    break;
                case VECTOR_FLOAT:
                    new (&m_vector) std::vector<float>(other.m_vector);
                    break;
                case EMPTY:
                    break;
            }
        }
        return *this;
    }
    
    // Move assignment
    MessageVariant& operator=(MessageVariant&& other) noexcept {
        if (this != &other) {
            clear();
            m_type = other.m_type;
            switch (m_type) {
                case FLOAT:
                    m_float = other.m_float;
                    break;
                case STRING:
                    new (&m_string) std::string(std::move(other.m_string));
                    break;
                case VECTOR_FLOAT:
                    new (&m_vector) std::vector<float>(std::move(other.m_vector));
                    break;
                case EMPTY:
                    break;
            }
            other.m_type = EMPTY;
        }
        return *this;
    }
    
    // Construct from float
    MessageVariant(float value) : m_type(FLOAT), m_float(value) {}
    
    // Construct from string
    MessageVariant(const std::string& value) : m_type(STRING) {
        new (&m_string) std::string(value);
    }
    
    // Construct from string r-value
    MessageVariant(std::string&& value) : m_type(STRING) {
        new (&m_string) std::string(std::move(value));
    }
    
    // Construct from vector<float>
    MessageVariant(const std::vector<float>& value) : m_type(VECTOR_FLOAT) {
        new (&m_vector) std::vector<float>(value);
    }
    
    // Construct from vector<float> r-value
    MessageVariant(std::vector<float>&& value) : m_type(VECTOR_FLOAT) {
        new (&m_vector) std::vector<float>(std::move(value));
    }
    
    // Get type
    Type getType() const {
        return m_type;
    }
    
    // Check if holds float
    bool isFloat() const {
        return m_type == FLOAT;
    }
    
    // Check if holds string
    bool isString() const {
        return m_type == STRING;
    }
    
    // Check if holds vector<float>
    bool isVectorFloat() const {
        return m_type == VECTOR_FLOAT;
    }
    
    // Check if empty
    bool isEmpty() const {
        return m_type == EMPTY;
    }
    
    // Get float value
    float getFloat() const {
        if (m_type != FLOAT) {
            throw std::runtime_error("Variant does not contain a float");
        }
        return m_float;
    }
    
    // Get string value
    const std::string& getString() const {
        if (m_type != STRING) {
            throw std::runtime_error("Variant does not contain a string");
        }
        return m_string;
    }
    
    // Get vector<float> value
    const std::vector<float>& getVectorFloat() const {
        if (m_type != VECTOR_FLOAT) {
            throw std::runtime_error("Variant does not contain a vector<float>");
        }
        return m_vector;
    }
    
private:
    Type m_type;
    
    union {
        float m_float;
        std::string m_string;
        std::vector<float> m_vector;
    };
    
    // Clear the variant
    void clear() {
        switch (m_type) {
            case STRING:
                m_string.~basic_string();
                break;
            case VECTOR_FLOAT:
                m_vector.~vector();
                break;
            default:
                break;
        }
        m_type = EMPTY;
    }
};

} // namespace mcp 