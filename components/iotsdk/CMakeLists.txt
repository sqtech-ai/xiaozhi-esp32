idf_component_register(
    INCLUDE_DIRS "."
    PRIV_REQUIRES mbedtls esp_http_client
)

add_prebuilt_library(iotsdk_prebuilt "esp32s3-idf-v5.5.2/migumusic.a")
target_link_libraries(${COMPONENT_LIB} INTERFACE iotsdk_prebuilt)
