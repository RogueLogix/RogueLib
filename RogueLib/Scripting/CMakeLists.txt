add_lib(Scripting)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib)

target_link_libraries(RogueLib_Scripting PUBLIC Cling)
