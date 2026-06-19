add_library(CesiumHoloveser "")

configure_cesium_library(CesiumHoloveser)

cesium_glob_files(CESIUM_HOLO_VESER_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_HOLO_VESER_HEADERS src/*.h)
cesium_glob_files(CESIUM_HOLO_VESER_PUBLIC_HEADERS include/CesiumHoloveser/*.h)
cesium_glob_files(CESIUM_HOLO_VESER_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_HOLO_VESER_TEST_HEADERS test/*.h)

set_target_properties(CesiumHoloveser
    PROPERTIES
        TEST_SOURCES "${CESIUM_HOLO_VESER_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_HOLO_VESER_TEST_HEADERS}"
)

set_target_properties(CesiumHoloveser
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_HOLO_VESER_PUBLIC_HEADERS}"
)

target_sources(
    CesiumHoloveser
    PRIVATE
        ${CESIUM_HOLO_VESER_SOURCES}
        ${CESIUM_HOLO_VESER_HEADERS}
        ${CESIUM_HOLO_VESER_PUBLIC_HEADERS}
)

cesium_target_include_directories(
    TARGET
        CesiumHoloveser
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(CesiumHoloveser
    PUBLIC
        CesiumAsync
        CesiumGeospatial
        CesiumGeometry
        CesiumGltf
        CesiumGltfContent
        CesiumGltfReader
        CesiumUtility
        CesiumRasterOverlays
        tinyxml2::tinyxml2
)
