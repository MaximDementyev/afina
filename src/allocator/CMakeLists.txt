# build service
set(SOURCE_FILES
    Simple.cpp
    Pointer.cpp
	Simple_private.cpp
)

add_library(Allocator ${SOURCE_FILES})
target_link_libraries(Allocator ${CMAKE_THREAD_LIBS_INIT})
