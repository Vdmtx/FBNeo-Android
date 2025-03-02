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

# Verifica se a pasta `src` existe, caso contrário, tenta `libretro`
if(EXISTS "${CMAKE_SOURCE_DIR}/src")
    add_subdirectory(src)
elseif(EXISTS "${CMAKE_SOURCE_DIR}/libretro")
    add_subdirectory(libretro)
else()
    message(FATAL_ERROR "Erro: Nenhuma pasta válida de código-fonte encontrada! Certifique-se de que `src` ou `libretro` existem.")
endif()

# Arquivos fonte do projeto
file(GLOB_RECURSE SOURCES 
    "${CMAKE_SOURCE_DIR}/src/*.cpp" 
    "${CMAKE_SOURCE_DIR}/src/*.c" 
    "${CMAKE_SOURCE_DIR}/src/*.h"
    "${CMAKE_SOURCE_DIR}/libretro/*.cpp"
    "${CMAKE_SOURCE_DIR}/libretro/*.c"
    "${CMAKE_SOURCE_DIR}/libretro/*.h"
)

# Criar o executável
add_executable(fbneo ${SOURCES})

# Incluir diretórios
include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/libretro)

# Configuração de bibliotecas externas
target_link_libraries(fbneo)
