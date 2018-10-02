SUMMARY = "Jtag master driver module"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=12f884d2ae1ff87c09e5b7ccc2c4ca7e"

inherit module

SRC_URI = "file://Makefile \
           file://COPYING \
	   file://jtag_drv.c \
	   file://jtag_drv.h \
          "

S = "${WORKDIR}"

RPROVIDES_${PN} += "kernel-module-jtag"

