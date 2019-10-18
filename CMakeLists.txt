cmake_minimum_required (VERSION 3.4)
project (scmp_rescale)

# add the required Qt source includes to the cmake path
include_directories(
    ${CMAKE_SOURCE_DIR}
    )

add_subdirectory (scmp)
add_subdirectory (nfa_gl)

# ----- apps ------------
add_subdirectory (scmp_rescale)
