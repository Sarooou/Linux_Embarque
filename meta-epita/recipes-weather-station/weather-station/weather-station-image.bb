SUMMARY = "Weather Station (Banana Pi M2 Zero)"
LICENSE = "MIT"

# Image
inherit core-image

# Debug
IMAGE_INSTALL:append = " weather-station-app i2c-tools"

# Root login
EXTRA_IMAGE_FEATURES:append = " empty-root-password"

ROOTFS_POSTPROCESS_COMMAND += "weather_station_inittab;"

weather_station_inittab() {
    if [ -f ${IMAGE_ROOTFS}${sysconfdir}/inittab ]; then
        echo '::once:/etc/init.d/weather-station start' >> ${IMAGE_ROOTFS}${sysconfdir}/inittab
        echo '::shutdown:/etc/init.d/weather-station stop' >> ${IMAGE_ROOTFS}${sysconfdir}/inittab
    fi
}