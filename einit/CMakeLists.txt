project (einit)

# Tools for using NK parser.
include (platform/image)

# Set compile flags.
project_header_default ("STANDARD_GNU_11:YES" "STRICT_WARNINGS:NO")

# Define the ENTITIES variable with a list of entities.
set (ENTITIES
        ${precompiled_vfsVfsNet}
        Client
        ControlSystem
        LightsGPIO
        Diagnostics)

set_target_properties (${precompiled_vfsVfsNet} PROPERTIES
        EXTRA_ARGS "\
    - -l
    - devfs /dev devfs 0
    - -l
    - romfs /etc romfs 0")

# KasperskyOS image for target hardware platform.
build_kos_hw_image (kos-image
        EINIT_ENTITY EinitHw
        CONNECTIONS_CFG "src/init.yaml.in"
        SECURITY_PSL "src/security.psl"
        IMAGE_FILES ${ENTITIES}
                    ${CMAKE_SOURCE_DIR}/resources/romfs/etc/hosts)

# KasperskyOS image for QEMU with simulation targets (sim, gdbsim, gdb).
build_kos_qemu_image (kos-qemu-image
        EINIT_ENTITY EinitQemu
        CONNECTIONS_CFG "src/init.yaml.in"
        SECURITY_PSL "src/security.psl"
        IMAGE_FILES ${ENTITIES}
                    ${CMAKE_SOURCE_DIR}/resources/romfs/etc/hosts)
