# third_party_effects_data_gen — CMake 构建脚本
# 依赖: fmt (header-only), xlntx (自研 xlsx 只读库)
# Release 输出: ~2MB 独立 EXE, 仅依赖 KERNEL32.dll + USER32.dll
cmake_minimum_required(VERSION 3.15.0)

project(third_party_effects_data_gen VERSION 0.1.0)

# Use static MSVC runtime (/MT instead of /MD)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(third_party_effects_data_gen EffectInfo.cpp third_party_effects_data_gen_vs.cpp resource.rc)
target_compile_features(third_party_effects_data_gen PRIVATE cxx_std_17)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(third_party_effects_data_gen PRIVATE fmt::fmt-header-only)

add_subdirectory(xlntx)

target_link_libraries(third_party_effects_data_gen PRIVATE xlntx)

# xlntx test executable
add_executable(test_xlntx test_xlntx.cpp)
target_compile_features(test_xlntx PRIVATE cxx_std_17)
find_package(fmt CONFIG REQUIRED)
target_link_libraries(test_xlntx PRIVATE fmt::fmt-header-only)

find_package(xlnt CONFIG REQUIRED)
target_link_libraries(test_xlntx PRIVATE xlntx)
target_link_libraries(test_xlntx PRIVATE xlnt::xlnt)
if(MSVC)
    target_compile_options(test_xlntx PRIVATE $<$<CONFIG:Release>:/O1>)
endif()


# Release optimizations for small standalone EXE
if(MSVC)
    # Enable LTCG (Link-Time Code Generation) for dead code elimination
    set_property(TARGET third_party_effects_data_gen PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

    # Optimize for minimal size in Release
    target_compile_options(third_party_effects_data_gen PRIVATE
        $<$<CONFIG:Release>:/O1>        # minimize size
        $<$<CONFIG:Release>:/GL>        # whole program optimization
        $<$<CONFIG:Release>:/Gy>        # function-level linking
    )

    # Linker optimizations for size
    target_link_options(third_party_effects_data_gen PRIVATE
        $<$<CONFIG:Release>:/LTCG>      # link-time code generation
        $<$<CONFIG:Release>:/OPT:REF>   # remove unreferenced data
        $<$<CONFIG:Release>:/OPT:ICF>   # fold identical functions
    )
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

