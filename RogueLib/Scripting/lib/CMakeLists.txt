get_cmake_property(CACHE_VARS CACHE_VARIABLES)
foreach (CACHE_VAR ${CACHE_VARS})
    get_property(CACHE_VAR_HELPSTRING CACHE ${CACHE_VAR} PROPERTY HELPSTRING)
    if (CACHE_VAR_HELPSTRING STREQUAL "No help, variable specified on the command line.")
        get_property(CACHE_VAR_TYPE CACHE ${CACHE_VAR} PROPERTY TYPE)
        if (CACHE_VAR_TYPE STREQUAL "UNINITIALIZED")
            set(CACHE_VAR_TYPE)
        else ()
            set(CACHE_VAR_TYPE :${CACHE_VAR_TYPE})
        endif ()
        set(CMAKE_ARGS "${CMAKE_ARGS} -D${CACHE_VAR}${CACHE_VAR_TYPE}=\"${${CACHE_VAR}}\"")
    endif ()
endforeach ()

include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

execute_process(COMMAND nproc OUTPUT_VARIABLE NPROC)

ExternalProject_Add(ClingBuild
        DOWNLOAD_COMMAND ${CMAKE_CURRENT_LIST_DIR}/downloadCling.sh
        SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/cling"
        BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/cling/build"
        CMAKE_ARGS ${CMAKE_ARGS} -DLLVM_TARGETS_TO_BUILD=X86;PowerPC;ARM
        # i only need it to build clingInterpreter, as im not using anything else
        BUILD_COMMAND ${CMAKE_COMMAND} -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS}" -DCMAKE_C_FLAGS="${CMAKE_C_FLAGS}" --build ${CMAKE_CURRENT_BINARY_DIR}/cling/build --target libcling -- -j ${NPROC}
        UPDATE_COMMAND ""
        INSTALL_COMMAND ""
        )

add_library(Cling INTERFACE)
add_dependencies(Cling ClingBuild)
target_link_directories(Cling INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/cling/build/lib/)
target_link_libraries(Cling INTERFACE cling)
target_include_directories(Cling INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/cling/build/include)
target_include_directories(Cling INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/cling/include)
target_include_directories(Cling INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/cling/tools/cling/include)
target_include_directories(Cling INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/cling/tools/clang/include)
target_compile_options(Cling INTERFACE -DLLVMDIR="${CMAKE_CURRENT_BINARY_DIR}/cling/build/")