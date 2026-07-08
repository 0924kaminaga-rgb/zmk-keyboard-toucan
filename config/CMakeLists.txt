# Included by ZMK's build system when ZMK_CONFIG points to this directory.
# Telemetry module compiles for the left half only — right half has no USB host connection.
if(CONFIG_SHIELD_TOUCAN_LEFT)
    target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/toucan_telemetry.c)
endif()
