cmake_minimum_required (VERSION 3.12)

project (traffic_light)

# Initialize CMake library for KasperskyOS SDK.
include (platform)
initialize_platform (FORCE_STATIC)

# Tools for using NK parser.
include (platform/nk)

# Add a package for working with the virtual file system.
find_package (vfs REQUIRED COMPONENTS FS_LIB CLIENT_LIB SERVER_LIB)
include_directories (${vfs_INCLUDE})

# Add a package with the dhcpcd program implementation.
find_package (rump REQUIRED COMPONENTS DHCPCD_ENTITY)
include_directories (${rump_INCLUDE})

# Add a package with the VFS program implementations.
find_package (precompiled_vfs REQUIRED)
include_directories (${precompiled_vfs_INCLUDE})

# Add package importing components for building audit entity and connecting to it.
find_package (klog REQUIRED)
include_directories (${klog_INCLUDE})

# Add package importing `klog_storage_entity`.
find_package (klog_storage REQUIRED)
include_directories (${klog_storage_INCLUDE})

# Choose configuration files and the network configuration mode.
# If none of the options are selected, the ramfs device and the dynamic
# network configuration with fallback to static mode are used.
option (DHCPCD_FALLBACK "Configuration by /etc/dhcpcd.conf"           OFF)
option (DHCPCD_DYNAMIC  "Configuration by /etc/dhcpcd_dynamic.config" OFF)
option (DHCPCD_STATIC   "Configuration by /etc/dhcpcd_static.config"  OFF)

# Attach the directory with the generated config header files.
include_directories (${CMAKE_CURRENT_BINARY_DIR})
set (RESOURCES ${CMAKE_SOURCE_DIR}/resources)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Recommended compiler flags against vulnerabilities.
set(VULN_LDFLAGS "\
    -Wl,-z,noexecstack \
    -Wl,-z,separate-code \
    -Wl,-z,now")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
    -Wall -Wextra -Wconversion \
    -fPIE -pie -D_FORTIFY_SOURCE=2 -O2 \
    -fstack-protector-strong -fstack-clash-protection \
    -mbranch-protection=standard \
    -Wsign-conversion -Wformat=2 -Wformat-security -Werror=format-security \
    -fsanitize=undefined -fsanitize-undefined-trap-on-error")

nk_build_idl_files (traffic_light_idl_files NK_MODULE "traffic_light" IDL "resources/IMode.idl")
nk_build_idl_files (traffic_light_idl_files NK_MODULE "traffic_light" IDL "resources/IDiagMessage.idl")
nk_build_idl_files (traffic_light_control_system_idl_files NK_MODULE "traffic_light" IDL "resources/ControlSystem.idl")

nk_build_cdl_files (traffic_light_cdl_files IDL_TARGET traffic_light_idl_files NK_MODULE "traffic_light" CDL "resources/CMode.cdl")
nk_build_cdl_files (traffic_light_cdl_files IDL_TARGET traffic_light_idl_files NK_MODULE "traffic_light" CDL "resources/CDiagMessage.cdl")
nk_build_cdl_files (traffic_light_control_system_cdl_files IDL_TARGET traffic_light_control_system_idl_files NK_MODULE "traffic_light" CDL "resources/CControlSystem.cdl")

nk_build_edl_files (traffic_light_lights_gpio_edl_files CDL_TARGET traffic_light_cdl_files NK_MODULE "traffic_light" EDL "resources/LightsGPIO.edl")
nk_build_edl_files (traffic_light_control_system_edl_files NK_MODULE "traffic_light" EDL "resources/ControlSystem.edl")
nk_build_edl_files (traffic_light_diagnostics_edl_files CDL_TARGET traffic_light_cdl_files NK_MODULE "traffic_light" EDL "resources/Diagnostics.edl")
nk_build_edl_files (traffic_light_client_edl_files NK_MODULE "client" EDL "${CMAKE_SOURCE_DIR}/resources/Client.edl")
nk_build_edl_files (traffic_light_klog_edl_files NK_MODULE "traffic_light" EDL "resources/KlogEntity.edl")
nk_build_edl_files (traffic_light_klog_storage_edl_files NK_MODULE "traffic_light" EDL "resources/KlogStorageEntity.edl")
nk_build_edl_files (traffic_light_control_system_edl_files CDL_TARGET traffic_light_control_system_cdl_files NK_MODULE "traffic_light" EDL "resources/ControlSystem.edl")
nk_build_edl_files (traffic_light_connector_edl_files NK_MODULE "traffic_light" EDL "resources/Connector.edl")

add_library (json_parser STATIC ${CMAKE_SOURCE_DIR}/third-party/json-parser/json.c)
target_include_directories(json_parser PUBLIC  ${CMAKE_SOURCE_DIR}/third-party/json-parser)

add_subdirectory (control_system)
add_subdirectory (lights_gpio)
add_subdirectory (diagnostics)
add_subdirectory (client)
add_subdirectory (connector)
add_subdirectory (klog_storage)
add_subdirectory (einit)