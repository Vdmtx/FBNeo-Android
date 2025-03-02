cmake_minimum_required(VERSION 3.10)

# Nome do projeto
project(FBNeo-Android)

# Configuração do compilador
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Definição da arquitetura para Android
if(ANDROID)
    set(CMAKE_SYSTEM_NAME Android)
    set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK})
    set(CMAKE_ANDROID_STL_TYPE c++_static)
    set(CMAKE_ANDROID_ARCH_ABI armeabi-v7a)
    set(CMAKE_ANDROID_PLATFORM android-21)
endif()

# Diretório de código-fonte
add_subdirectory(src)

# Arquivos fonte do projeto
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c" "src/*.h")

# Criar o executável
add_executable(fbneo ${SOURCES})

# Incluir diretórios
include_directories(${CMAKE_SOURCE_DIR}/src)

# Configuração de bibliotecas externas
target_link_libraries(fbneo)
