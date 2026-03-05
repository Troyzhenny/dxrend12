cmake_minimum_required (VERSION 3.10)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("dxrend12")

# Add source to this project's executable.
add_executable (dxrend12 
	"src/main.cpp"
)

target_link_libraries(dxrend12 PRIVATE
	d3d12.lib
	dxgi.lib
	d3dcompiler.lib
	user32.lib
)

target_include_directories(dxrend12 PUBLIC
	include/DirectX-Headers/include/directx/
)

# Copy shader files to build directory so the exe can find them
file(GLOB SHADER_FILES "${CMAKE_SOURCE_DIR}/src/*.hlsl")
foreach(SHADER ${SHADER_FILES})
    configure_file(${SHADER} ${CMAKE_BINARY_DIR} COPYONLY)
endforeach()

if (CMAKE_VERSION VERSION_GREATER 3.10)
  set_property(TARGET dxrend12 PROPERTY CXX_STANDARD 20)
endif()