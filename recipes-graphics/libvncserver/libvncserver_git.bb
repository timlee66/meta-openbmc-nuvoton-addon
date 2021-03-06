DESCRIPTION = "libvncserver - A library implementing the VCN protocol"
HOMEPAGE = "https://github.com/LibVNC/libvncserver"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=361b6b837cad26c6900a926b62aada5f"
DEPENDS += "jpeg libpng openssl"
BRANCH = "npcm7xx_v2.1"
SRC_URI = "git://github.com/Nuvoton-Israel/libvncserver;branch=${BRANCH};"
SRCREV = "${AUTOREV}"
S = "${WORKDIR}/git"

EXTRA_OECMAKE += "-DWITH_HWDIFF=on"

inherit cmake
