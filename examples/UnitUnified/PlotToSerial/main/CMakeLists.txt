# Dependencies (M5Unit-GESTURE, M5UnitUnified, M5Unified, ...) are pulled in via
# main/idf_component.yml and auto-added as requirements by the component manager.
#
# UnitGesture has a single target chip, so this example has no Kconfig USING_* variant
# and therefore does not include common/variant.cmake.
idf_component_register(
    SRCS "PlotToSerial.cpp"
    INCLUDE_DIRS ".")
