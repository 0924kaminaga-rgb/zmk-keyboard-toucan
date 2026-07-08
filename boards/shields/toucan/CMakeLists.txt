# Telemetry module — compiled for the left half only (USB host connection).
# Zephyr automatically includes CMakeLists.txt from active shield directories.
if(CONFIG_SHIELD_TOUCAN_LEFT)
    target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/toucan_telemetry.c)
endif()
