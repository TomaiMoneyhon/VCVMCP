cmake_minimum_required(VERSION 3.15)
project(VCV_MCP VERSION 0.1.0 LANGUAGES CXX)

# C++14 required for compatibility with VCV Rack
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# Configure output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add MCP library
add_library(mcp 
  src/mcp/IMCPBroker.cpp
  src/mcp/MCPBroker.cpp
)

# Set include directory for MCP
target_include_directories(mcp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Documentation directory
add_custom_target(docs SOURCES
  docs/api_documentation.md
)

# Setup testing
option(BUILD_TESTING "Build the testing tree" ON)
if(BUILD_TESTING)
  enable_testing()
  
  # Fetch Google Test
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.11.0
  )
  FetchContent_MakeAvailable(googletest)
  
  # Add test directory
  add_subdirectory(test)
endif()

# Build examples
add_subdirectory(examples) 