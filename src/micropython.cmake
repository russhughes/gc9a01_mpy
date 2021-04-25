# Create an INTERFACE library for our C module.
add_library(usermod_gc9a01 INTERFACE)

# Add our source files to the lib
target_sources(usermod_gc9a01 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/gc9a01.c
    ${CMAKE_CURRENT_LIST_DIR}/mpfile.c
    ${CMAKE_CURRENT_LIST_DIR}/tjpgd565.c
)

# Add the current directory as an include directory.
target_include_directories(usermod_gc9a01 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}

)
target_compile_definitions(usermod_gc9a01 INTERFACE
    MODULE_GC9A01_ENABLED=1
)

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_gc9a01)
