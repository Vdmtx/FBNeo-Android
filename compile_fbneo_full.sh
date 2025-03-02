#!/bin/bash

# Configuração do ambiente
export ANDROID_NDK=$ANDROID_NDK_ROOT
export PATH=$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH

# Criando diretório de compilação
mkdir -p build_android && cd build_android

# Configurando e compilando FBNeo para Android
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=armeabi-v7a \
      -DANDROID_PLATFORM=android-21 \
      -DCMAKE_BUILD_TYPE=Release ..

make -j$(nproc)
