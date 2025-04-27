add_executable(broker_registration
  broker-registration/main.cpp
)

target_link_libraries(broker_registration
  PRIVATE
  mcp
)

# Add serialization example
add_executable(serialization_example SerializationExample.cpp)
target_link_libraries(serialization_example mcp) 