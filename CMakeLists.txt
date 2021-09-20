cmake_minimum_required(VERSION 3.16)

project(VoiceVoxCPP)
set(USE_CUDA OFF CACHE BOOL "use CUDA")

add_subdirectory(third_party)
find_package(Boost 1.71 REQUIRED) # header only

add_library(vvengine STATIC
	src/audio_writer.cc
	src/engine.cc
	src/full_context_label.cc
	src/openjtalk_wrapper.cc
)
add_dependencies(vvengine open_jtalk)
set_property(TARGET vvengine PROPERTY CXX_STANDARD 17)
target_compile_options(vvengine PUBLIC -g -O2 -Wall)

target_include_directories(vvengine PRIVATE third_party/include)
target_include_directories(vvengine PUBLIC include)
target_link_directories(vvengine PRIVATE third_party/lib)
target_link_libraries(vvengine PRIVATE core openjtalk)

message("mecab dir: ${MECAB_DIR_PATH}")
add_definitions(-DMECAB_DIR=\"${MECAB_DIR_PATH}\")

add_executable(vv src/vv.cc)
add_dependencies(vv vvengine)
set_property(TARGET vv PROPERTY CXX_STANDARD 17)
target_compile_options(vv PUBLIC -g -O2 -Wall)

target_include_directories(vv PRIVATE include)
target_link_directories(vv PRIVATE third_party/lib)
target_link_libraries(vv PRIVATE vvengine)

if (USE_CUDA)
	add_definitions(-DUSE_CUDA)
endif ()

if (MSVC)
	file(GLOB TORCH_DLLS "third_party/lib/*.dll")
	add_custom_command(
		TARGET vv
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		${TORCH_DLLS}
		$<TARGET_FILE_DIR:vv>
	)
endif (MSVC)

