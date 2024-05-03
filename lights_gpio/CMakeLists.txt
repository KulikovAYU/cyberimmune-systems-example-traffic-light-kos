project (LightsGPIO)

# Tools for using NK parser.
include (platform/nk)

# Set compile flags.
project_header_default ("STANDARD_GNU_11:YES" "STRICT_WARNINGS:NO")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable (LightsGPIO "src/lights_gpio.c" "src/server.c")
add_dependencies (LightsGPIO traffic_light_lights_gpio_edl_files)
target_link_libraries(LightsGPIO ${vfs_CLIENT_LIB})
target_link_libraries(LightsGPIO Threads::Threads)

target_include_directories(LightsGPIO PRIVATE "headers")

# We do not need the default VFS entity which comes from ${vfs_CLIENT_LIB}.
set_target_properties (LightsGPIO PROPERTIES ${vfs_ENTITY}_REPLACEMENT "")
set_target_properties (LightsGPIO PROPERTIES LINK_FLAGS ${VULN_LDFLAGS})
