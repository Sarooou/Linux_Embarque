SUMMARY = "Weather Station (Banana Pi M2 Zero)"
LICENSE = "MIT"

# Image
inherit core-image

# Debug
IMAGE_INSTALL:append = " weather-station-app i2c-tools"

# Root login
EXTRA_IMAGE_FEATURES:append = " empty-root-password"