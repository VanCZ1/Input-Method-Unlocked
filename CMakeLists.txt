cmake_minimum_required(VERSION 4.2)
set(NAME "InputMethodUnlocked")
set(VERSION 1.2.0)
set(LICENSE "GPL-3.0 License")

# --------------------------------------------------------------------------- #
#                                   Options                                   #
# --------------------------------------------------------------------------- #
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)
option(COPY_BUILD "Copy the build output to the target directory." ON)

# --------------------------------------------------------------------------- #
#                                   Project                                   #
# --------------------------------------------------------------------------- #
project(
    ${NAME}
    VERSION ${VERSION}
    LANGUAGES CXX
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.rc
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
    @ONLY
)

include(cmake/lib/autoAddSources.cmake)

# --------------------------------------------------------------------------- #
#                                 Dependencies                                #
# --------------------------------------------------------------------------- #
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/CommonLibSSE-NG)

find_path(CLIB_UTIL_INCLUDE_DIRS "ClibUtil/utils.hpp")

# --------------------------------------------------------------------------- #
#                                    Source                                   #
# --------------------------------------------------------------------------- #
add_include_folder("${CMAKE_CURRENT_SOURCE_DIR}/include")
add_src_folder("${CMAKE_CURRENT_SOURCE_DIR}/include")
add_src_folder("${CMAKE_CURRENT_SOURCE_DIR}/src")

source_group(
    TREE
        ${CMAKE_CURRENT_SOURCE_DIR}
    FILES
        ${headers}
        ${sources}
)

source_group(
    TREE
        ${CMAKE_CURRENT_BINARY_DIR}
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
)

# --------------------------------------------------------------------------- #
#                                    Build                                    #
# --------------------------------------------------------------------------- #
add_library(
    ${PROJECT_NAME} 
    SHARED 
        ${sources}
        ${headers}
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc
        ${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
)

target_compile_features(
    ${PROJECT_NAME}
    PRIVATE
        cxx_std_23
)

target_precompile_headers(
    ${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/PCH.h
)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CLIB_UTIL_INCLUDE_DIRS}
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        CommonLibSSE::CommonLibSSE
)

# --------------------------------------------------------------------------- #
#                                  Post build                                 #
# --------------------------------------------------------------------------- #
if(COPY_BUILD AND DEFINED ENV{SKYRIM_MODS_PATH})
    set(DLL_FOLDER "$ENV{SKYRIM_MODS_PATH}/${PROJECT_NAME}/SKSE/Plugins")

    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DLL_FOLDER}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> $<TARGET_PDB_FILE:${PROJECT_NAME}> ${DLL_FOLDER}
        VERBATIM
    )
endif()
