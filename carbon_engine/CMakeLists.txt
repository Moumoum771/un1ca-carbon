cmake_minimum_required(VERSION 3.20)
project(CarbonEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Flags de otimização e rigor
add_compile_options(-Wall -Wextra -O3 -pipe -pthread)

include_directories(include)

find_package(OpenSSL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

set(CARBON_LIB_SOURCES
    src/fingerprint/input_hasher.cpp
    src/ledger/sqlite_ledger.cpp
    src/dag/dependency_graph.cpp
    src/supervisor/process_supervisor.cpp
    src/security/key_manager.cpp
    src/core/pipeline.cpp
    src/core/recipe_executor.cpp
)

# Biblioteca estática com o core do Carbon Engine
add_library(carbon_core STATIC ${CARBON_LIB_SOURCES})
target_link_libraries(carbon_core PUBLIC 
    OpenSSL::Crypto 
    SQLite::SQLite3 
    Threads::Threads
)

# Executável Principal (CLI)
add_executable(carbon-engine src/main.cpp)
target_link_libraries(carbon-engine PRIVATE carbon_core)

# Executável de Testes Unitários e Validação
add_executable(test_engine tests/test_engine.cpp)
target_link_libraries(test_engine PRIVATE carbon_core)
