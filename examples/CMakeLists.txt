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

# Add reference implementation example
add_executable(reference_implementation
  reference-implementation/main.cpp
)

target_link_libraries(reference_implementation
  PRIVATE
  mcp
)

# Add reference modules example
add_executable(reference_modules_example ReferenceModulesExample.cpp)
target_link_libraries(reference_modules_example mcp) 