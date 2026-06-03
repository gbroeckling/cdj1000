# IDF component manifest for the usb_midi external component.
#
# ESPHome's codegen will already register the .cpp/.c source files for
# build, so this file mostly exists to declare the TinyUSB dependency
# at the IDF-component-system level. The `__init__.py` adds the
# `espressif/esp_tinyusb` managed component; this CMakeLists.txt makes
# sure our local sources are compiled together with it.

idf_component_register(
    SRCS
        "usb_midi.cpp"
        "usb_descriptors.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        esp_tinyusb
)
