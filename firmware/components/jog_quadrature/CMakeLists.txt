# IDF component manifest for the jog_quadrature external component.
#
# `pulse_cnt` lives inside IDF's `driver` component, so we only need to
# declare that dependency. No managed components, no third-party libs.

idf_component_register(
    SRCS
        "jog_quadrature.cpp"
    INCLUDE_DIRS
        "."
    REQUIRES
        driver
)
