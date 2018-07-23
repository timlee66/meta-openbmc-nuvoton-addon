DESCRIPTION = "Remote-KVM"
PRIORITY = "optional"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=361b6b837cad26c6900a926b62aada5f"

DEPENDS += "zlib jpeg libpng openssl systemd"

inherit autotools binconfig pkgconfig
inherit obmc-phosphor-systemd

S = "${WORKDIR}/git"

SRC_URI = "git://github.com/NTC-CCBG/soc-remote-kvm;protocol=ssh;branch=master; \
          "
SRCREV = "${AUTOREV}"

EXTRA_OECONF += "--with-poleg=yes --without-gcrypt --without-gnutls"
SYSTEMD_SERVICE_${PN} = "remote-kvm.service"
