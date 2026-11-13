SUMMARY = "C library for OLED SSD1306"
DESCRIPTION = "C graphics and device driver library to write to OLED SSD1306 128x64 or 128x32 using I2C"

HOMEPAGE = "https://github.com/stealthylabs/libssd1306"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=61d71fea493144d3daf5fedc17299739"
inherit autotools-brokensep pkgconfig

SRC_URI = "git://github.com/stealthylabs/libssd1306.git;branch=master;protocol=https"
SRCREV = "9d62b38ce2ad0ae6dbb4cc7b4f4c39b902fa7cce"

DEPENDS = "freetype libunistring"