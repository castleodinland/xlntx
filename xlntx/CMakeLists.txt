# xlntx — 自研 C++17 只读 Excel .xlsx 解析库，xlnt 的平替
# 仅依赖 miniz (内置)，无其他第三方依赖
cmake_minimum_required(VERSION 3.15.0)

project(xlntx VERSION 0.1.0)

add_library(xlntx STATIC
    # implementation sources
    source/cell_reference.cpp
    source/index_types.cpp
    source/exceptions.cpp
    source/rich_text.cpp
    source/path.cpp
    source/cell.cpp
    source/worksheet.cpp
    source/workbook.cpp
    source/range.cpp
    source/range_reference.cpp
    source/detail/xml_parser.cpp
    source/detail/xlsx_reader.cpp
    source/detail/zip_reader.cpp
    third-party/miniz/miniz.c
)

target_include_directories(xlntx
    PUBLIC
        include
    PRIVATE
        source
        third-party/miniz
)

target_compile_features(xlntx PUBLIC cxx_std_17)
target_compile_definitions(xlntx PUBLIC XLNTX_STATIC=1)

# miniz.c needs C compilation
set_source_files_properties(
    third-party/miniz/miniz.c
    PROPERTIES LANGUAGE C
)

if(MSVC)
    target_compile_definitions(xlntx PRIVATE _CRT_SECURE_NO_WARNINGS=1)
    target_compile_options(xlntx PRIVATE
        $<$<CONFIG:Release>:/O1>        # minimize size
        $<$<CONFIG:Release>:/Gy>        # function-level linking
    )
endif()
