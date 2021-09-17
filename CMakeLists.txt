cmake_minimum_required(VERSION 3.16)

project(VoiceVoxCPP)
set(USE_CUDA OFF CACHE BOOL "use CUDA")

file(GLOB SRC_FILES "src/*.cc")
add_executable(vv ${SRC_FILES})
add_compile_options("-g -O2")
add_subdirectory(third_party)
include(ExternalProject)
add_dependencies(vv open_jtalk)
set_property(TARGET vv PROPERTY CXX_STANDARD 17)
target_include_directories(vv PRIVATE third_party/include)
target_include_directories(vv PUBLIC include)
if (USE_CUDA)
	add_definitions(-DUSE_CUDA)
endif ()
message("mecab dir: ${MECAB_DIR_PATH}")
add_definitions(-DMECAB_DIR=\"${MECAB_DIR_PATH}\")

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

# find_library(CORE_LIBRARY core PATHS third_party/lib)
# find_library(OPENJTALK_LIBRARY openjtalk PATHS third_party/lib)
target_link_directories(vv PRIVATE third_party/lib)
target_link_libraries(vv core openjtalk)
