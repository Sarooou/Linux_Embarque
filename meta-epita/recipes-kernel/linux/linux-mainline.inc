SECTION = "kernel"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=bbea815ee2795b2f4230826c0c6b8814"
COMPATIBLE_MACHINE = "(sun4i|sun5i|sun7i|sun8i|sun9i|sun50i|sun55i)"

inherit kernel

require linux.inc

LINUX_VERSION = "${PV}"

KBRANCH ?= "master"

DEPENDS += "rsync-native"

RDEPENDS_${KERNEL_PACKAGE_NAME}-base += "kernel-devicetree"

KERNEL_EXTRA_ARGS += "LOADADDR=${UBOOT_ENTRYPOINT}"

S = "${UNPACKDIR}/linux-${PV}"

KRELEASE = "${@d.getVar('PV', True).split('.')[0]}"

SRC_URI = "https://www.kernel.org/pub/linux/kernel/v${KRELEASE}.x/linux-${PV}.tar.xz \
       file://defconfig \
"

SRC_URI:append:use-mailine-graphics = " file://drm.cfg"
SRC_URI:append:bananapi-m2-zero = " file://axp20x.cfg"

FILES_${KERNEL_PACKAGE_NAME}-base:append = " ${nonarch_base_libdir}/modules/${KERNEL_VERSION}/modules.builtin.modinfo"

# Bananapi-m2-zero
SRC_URI:append:bananapi-m2-zero = " file://patches/0003-dts-allwinner-bananapi-m2-zero-Enforce-consistent-MM.patch"

# Capteur BPM280
SRC_URI:append:bananapi-m2-zero = " file://bme280.cfg"
SRC_URI:append:bananapi-m2-zero = " file://patches/0004-dts-enable-i2c0-bme280-and-i2c1.patch"
