cmake_minimum_required(VERSION 3.16)

set(VOICEVOX_CORE_VERSION 0.5.4)
set(MECAB_DIR_NAME open_jtalk_dic_utf_8-1.11)
set(MECAB_DIR_PATH "${CMAKE_BINARY_DIR}/${MECAB_DIR_NAME}" CACHE PATH "")

include(ExternalProject)
include(FetchContent)

### hts engine API (dependency of OpenJTalk) ###
ExternalProject_Add(
	hts_engine_API
	GIT_REPOSITORY https://github.com/r9y9/hts_engine_API
	GIT_TAG b7e1c8b
	GIT_PROGRESS True
	SOURCE_SUBDIR src
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}
)

### OpenJTalk ###
ExternalProject_Add(
	open_jtalk
	GIT_REPOSITORY https://github.com/r9y9/open_jtalk
	GIT_TAG 9572293
	GIT_PROGRESS True
	SOURCE_SUBDIR src
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}
	DEPENDS hts_engine_API
	BUILD_COMMAND ${CMAKE_COMMAND} --build . --target install
	INSTALL_COMMAND ""
)

FetchContent_Declare(
	mecabdir
	URL https://downloads.sourceforge.net/open-jtalk/${MECAB_DIR_NAME}.tar.gz
)
FetchContent_MakeAvailable(mecabdir)
file(GLOB MECABDIR_CONTENTS "${mecabdir_SOURCE_DIR}/*")
file(COPY
	${MECABDIR_CONTENTS}
	DESTINATION ${MECAB_DIR_PATH}
)

### JSON parser ###
file(DOWNLOAD
	https://github.com/nlohmann/json/releases/download/v3.10.2/json.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/nlohmann/json.hpp
	EXPECTED_HASH SHA256=059743e48b37e41579ee3a92e82e984bfa0d2a9a2b20b175d04db8089f46f047
	SHOW_PROGRESS
)

### LibTorch ###
message("downloading LibTorch...")
if (USE_CUDA)
	if (WIN32)
		message( "Windows gpu" )
		set(TORCH_URL https://download.pytorch.org/libtorch/cu111/libtorch-win-shared-with-deps-1.9.0%2Bcu111.zip)
	elseif (APPLE)
		message( "MacOS gpu" )
		message( FATAL_ERROR "MacOS binaries do not support LibTorch-CUDA!" )
	elseif (UNIX)
		message( "Linux gpu" )
		set(TORCH_URL https://download.pytorch.org/libtorch/cu111/libtorch-cxx11-abi-shared-with-deps-1.9.0%2Bcu111.zip)
	else ()
		message( FATAL_ERROR "Unknown arch" )
	endif ()
else ()
	if (WIN32)
		message( "Windows cpu" )
		set(TORCH_URL https://download.pytorch.org/libtorch/cpu/libtorch-win-shared-with-deps-1.9.0%2Bcpu.zip)
	elseif (APPLE)
		message( "MacOS cpu" )
		set(TORCH_URL https://download.pytorch.org/libtorch/cpu/libtorch-macos-1.9.0.zip)
	elseif (UNIX)
		message( "Linux cpu" )
		set(TORCH_URL https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-1.9.0%2Bcpu.zip)
	else ()
		message( FATAL_ERROR "Unknown arch" )
	endif ()
endif ()
FetchContent_Declare(
	LibTorch
	URL ${TORCH_URL}
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/torch
)
FetchContent_MakeAvailable(LibTorch)

if (UNIX AND USE_CUDA)
	file(CREATE_LINK
		${libtorch_SOURCE_DIR}/lib/libnvToolsExt-24de1d56.so.1
		${libtorch_SOURCE_DIR}/lib/libnvToolsExt.so.1
	)
	file(CREATE_LINK
		${libtorch_SOURCE_DIR}/lib/libcudart-6d56b25a.so.11.0
		${libtorch_SOURCE_DIR}/lib/libcudart.so.11.0
	)
endif ()

### VoiceVox Core ###
message("downloading VoiceVoxCore...")
FetchContent_Declare(
	voicevox_core
	GIT_REPOSITORY https://github.com/Hiroshiba/voicevox_core
	GIT_TAG ${VOICEVOX_CORE_VERSION}
)
FetchContent_MakeAvailable(voicevox_core)
file(COPY
	${voicevox_core_SOURCE_DIR}/core.h
	DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/include/voicevox_core
)

FetchContent_Declare(
	voicevox_core_lib
	URL https://github.com/Hiroshiba/voicevox_core/releases/download/${VOICEVOX_CORE_VERSION}/core.zip
)
FetchContent_MakeAvailable(voicevox_core_lib)
if (UNIX)
	file(COPY
		${voicevox_core_lib_SOURCE_DIR}/libcore.so
		${voicevox_core_lib_SOURCE_DIR}/libcore_cpu.so
		${voicevox_core_lib_SOURCE_DIR}/core.dll
		${voicevox_core_lib_SOURCE_DIR}/core_cpu.dll
		DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/lib
	)
endif (UNIX)
file(GLOB VV_BINS "${voicevox_core_lib_SOURCE_DIR}/*.bin")
file(COPY
	${VV_BINS}
	DESTINATION ${CMAKE_BINARY_DIR}
)

if (UNIX)
	execute_process(COMMAND
		chrpath -r ${libtorch_SOURCE_DIR}/lib
			${CMAKE_CURRENT_SOURCE_DIR}/lib/libcore.so
	)
	execute_process(COMMAND
		chrpath -r ${libtorch_SOURCE_DIR}/lib
			${CMAKE_CURRENT_SOURCE_DIR}/lib/libcore_cpu.so
	)
	execute_process(COMMAND
		patchelf --set-soname libcore_cpu.so
			${CMAKE_CURRENT_SOURCE_DIR}/lib/libcore_cpu.so
	)
endif (UNIX)