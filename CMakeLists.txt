cmake_minimum_required( VERSION 3.21.0 )
project( TsuHanTools CXX )

### Standard
set( CMAKE_CXX_STANDARD 23 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

set( CMAKE_COLOR_MAKEFILE ON )

# Generate 'compile_commands.json' for clang_complete
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

if( MSVC )
	add_compile_options(
		/MP # Parallel builds 
		/permissive- # Stricter C++ conformance

		# Warnings
		/W3

		# Consider these warnings as errors
		/we4018 # 'expression': signed/unsigned mismatch
		/we4062 # Enumerator 'identifier' in a switch of enum 'enumeration' is not handled
		/we4101 # 'identifier': unreferenced local variable
		/we4265 # 'class': class has virtual functions, but destructor is not virtual
		/we4305 # 'context': truncation from 'type1' to 'type2'
		/we4388 # 'expression': signed/unsigned mismatch
		/we4389 # 'operator': signed/unsigned mismatch

		/we4456 # Declaration of 'identifier' hides previous local declaration
		/we4457 # Declaration of 'identifier' hides function parameter
		/we4458 # Declaration of 'identifier' hides class member
		/we4459 # Declaration of 'identifier' hides global declaration

		/we4505 # 'function': unreferenced local function has been removed
		/we4547 # 'operator': operator before comma has no effect; expected operator with side-effect
		/we4549 # 'operator1': operator before comma has no effect; did you intend 'operator2'?
		/we4555 # Expression has no effect; expected expression with side-effect
		/we4715 # 'function': not all control paths return a value
		/we4834 # Discarding return value of function with 'nodiscard' attribute
		/we5038 # data member 'member1' will be initialized after data member 'member2'
		/we5245 # 'function': unreferenced function with internal linkage has been removed

	)
elseif( CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
	# Force colored diagnostic messages in Ninja's output
	if( CMAKE_GENERATOR STREQUAL "Ninja" )
		add_compile_options( -fdiagnostics-color=always )
	endif()

	add_compile_options(
		-Wall
		-Warray-bounds
		-Wextra
		-Wimplicit-fallthrough
		-Wmissing-declarations
		-Wmissing-declarations
		-Wmissing-field-initializers
		-Wno-attributes
		-Wno-invalid-offsetof
		-Wno-unused-parameter
		-Wreorder
		-Wshadow
		-Wsign-compare
		-Wswitch
		-Wuninitialized
		-Wunused-function
		-Wunused-result
		-Wunused-variable
	)
endif()

find_package( glm 0.9.9.9 QUIET )

if( glm_FOUND )
else()
	add_subdirectory( external/glm EXCLUDE_FROM_ALL )
endif()

# mio
add_subdirectory( external/mio EXCLUDE_FROM_ALL )

# tinygltf
add_subdirectory( external/tinygltf EXCLUDE_FROM_ALL )

# TsuHan
add_library(
	TsuHan
	source/TsuHan/TsuHan.cpp
)

target_include_directories(
	TsuHan
	PRIVATE
	include
)

target_link_libraries(
	TsuHan
	PRIVATE
	tinygltf
	mio
)

# Dump
add_executable(
	Dump
	source/Tools/Dump.cpp
)
target_include_directories(
	Dump
	PRIVATE
	include
)
target_link_libraries(
	Dump
	PRIVATE
	TsuHan
	mio
)