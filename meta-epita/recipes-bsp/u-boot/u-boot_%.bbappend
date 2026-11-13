FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

DEPENDS:append:sunxi = " bc-native dtc-native swig-native python3-native flex-native bison-native "

COMPATIBLE_MACHINE:sunxi = "(sun4i|sun5i|sun7i|sun8i|sun9i|sun50i|sun55i)"

DEFAULT_PREFERENCE:sun8i = "1"

SRC_URI:append:sunxi = " \
    file://boot.cmd \
"

UBOOT_ENV_SUFFIX:sunxi = "scr"
UBOOT_ENV:sunxi = "boot"

EXTRA_OEMAKE:append:sunxi = ' HOSTLDSHARED="${BUILD_CC} -shared ${BUILD_LDFLAGS} ${BUILD_CFLAGS}" '

do_compile:append:sunxi() {
    ${UBOOT_MKIMAGE} -C none -A arm -T script -d ${UNPACKDIR}/boot.cmd ${UNPACKDIR}/${UBOOT_ENV_BINARY}
}