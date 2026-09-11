##==============================================================================
## General.
##==============================================================================
cmake_minimum_required(VERSION 3.19)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/UnigineCMakeVariables.cmake)

project(unigine-d3d11-cpp-integration-sample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)

##==============================================================================
## Windows-only.
##==============================================================================
# The sample shares a texture between the engine (Direct3D 12 backend) and a Direct3D 11
# device using Win32 shared handles, so it only builds and runs on Windows. On other
# platforms the build is skipped gracefully (no target, no binary).
if (NOT WIN32)
	message(STATUS "unigine-d3d11-cpp-integration-sample is Windows-only. Skipped.")
	return()
endif()

##==============================================================================
## Environment.
##==============================================================================
set(UNIGINE_DOUBLE False CACHE BOOL "Double coords")
set(UNIGINE_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/../)
# Multi-sample repo fallback: if the per-sample SDK payload was not restored by
# SDK Browser (Repair), use the shared SDK at the repository root (two levels up).
# SDK Browser users (per-sample SDK present) are unaffected.
if(NOT EXISTS "${UNIGINE_SDK_PATH}include/UnigineEngine.h")
	get_filename_component(_repo_root "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)
	set(UNIGINE_SDK_PATH "${_repo_root}/")
endif()
set(UNIGINE_BIN_DIR ${UNIGINE_SDK_PATH}bin)
set(UNIGINE_LIB_DIR ${UNIGINE_SDK_PATH}lib)
set(UNIGINE_INCLUDE_DIR ${UNIGINE_SDK_PATH}include)

##==============================================================================
## Dependencies.
##==============================================================================
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

# Engine.
find_package(Engine REQUIRED MODULE QUIET)

##==============================================================================
## Target.
##==============================================================================
set(target "unigine-d3d11-cpp-integration-sample")

# Console app in Debug; windowed in Release. The app entry point is main().
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

add_executable(${target}
		${CMAKE_CURRENT_LIST_DIR}/main.cpp
		${CMAKE_CURRENT_LIST_DIR}/AppSystemLogic.cpp
		${CMAKE_CURRENT_LIST_DIR}/AppSystemLogic.h
		${CMAKE_CURRENT_LIST_DIR}/AppWorldLogic.cpp
		${CMAKE_CURRENT_LIST_DIR}/AppWorldLogic.h

		${CMAKE_CURRENT_LIST_DIR}/demos/D3D11Demo.h
		${CMAKE_CURRENT_LIST_DIR}/demos/DemoSelection.h
		${CMAKE_CURRENT_LIST_DIR}/demos/TextureTransferDemo.cpp
		${CMAKE_CURRENT_LIST_DIR}/demos/TextureTransferDemo.h
		${CMAKE_CURRENT_LIST_DIR}/demos/TextureWriteDemo.cpp
		${CMAKE_CURRENT_LIST_DIR}/demos/TextureWriteDemo.h

		${CMAKE_CURRENT_LIST_DIR}/d3d11/D3D11Utils.cpp
		${CMAKE_CURRENT_LIST_DIR}/d3d11/D3D11Utils.h
		${CMAKE_CURRENT_LIST_DIR}/d3d11/TextureSharedD3D11.cpp
		${CMAKE_CURRENT_LIST_DIR}/d3d11/TextureSharedD3D11.h
)

target_include_directories(${target}
	PRIVATE
	${UNIGINE_INCLUDE_DIR}
	${CMAKE_CURRENT_LIST_DIR}
	)

target_link_libraries(${target}
	PRIVATE
	Unigine::Engine
	d3d11.lib
	dxgi.lib
	d3dcompiler.lib
	)

target_compile_definitions(${target}
	PRIVATE
	$<$<CONFIG:Debug>:DEBUG>
	$<$<NOT:$<CONFIG:Debug>>:NDEBUG>
	)

##==============================================================================
## Compiler constants.
##==============================================================================
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(UNIGINE_COMPILER_IS_MSVC TRUE)
endif()

if (UNIGINE_COMPILER_IS_MSVC)
    target_compile_definitions(${target}
	PRIVATE
	_CRT_SECURE_NO_DEPRECATE
	)
    include(ProcessorCount)
    ProcessorCount(proc_count)
    target_compile_options(${target}
	PRIVATE
	/TP               # Specifies all source files are C++.
	/FS               # Forces writes to the program database (PDB) file to be serialized through MSPDBSRV.EXE.
	/fp:fast          # Specifies floating-point behavior.
	/bigobj           # Increases the number of addressable sections in an .obj file.
	/EHsc             # Specifies the kind of exception handling used by the compiler.
	/MP${proc_count}  # Build with Multiple Processes.
	)
    unset(proc_count)
    target_link_options(${target} INTERFACE "/FIXED:NO")
endif ()

set(binary_name ${target})
if (UNIGINE_DOUBLE)
	string(APPEND binary_name "_double")
endif()
string(APPEND binary_name "_x64")
set_target_properties(${target} PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(${target} PROPERTIES OUTPUT_NAME ${binary_name})

set_target_properties(${target}
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${UNIGINE_BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_DEBUG ${UNIGINE_BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ${UNIGINE_BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${UNIGINE_BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${UNIGINE_BIN_DIR}
	)

unset(UNIGINE_DOUBLE CACHE)
