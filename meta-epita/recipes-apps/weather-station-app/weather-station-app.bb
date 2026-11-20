SUMMARY = "Weather Station : BME280 + écran OLED SSD1306"
DESCRIPTION = "C program to write sensor BME280's value on the OLED screen SSD1306."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "libssd1306 libunistring freetype"

SRC_URI = "file://weather-station.c \
           file://weather-station.init"

S = "${UNPACKDIR}"

inherit update-rc.d

INITSCRIPT_NAME = "weather-station"
INITSCRIPT_PARAMS = "defaults 90"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} ${S}/weather-station.c -o ${B}/weather-station -lssd1306_i2c
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/weather-station ${D}${bindir}/weather-station
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 ${UNPACKDIR}/weather-station.init \
        ${D}${sysconfdir}/init.d/weather-station
}