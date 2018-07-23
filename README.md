# meta-openbmc-nuvoton-addon

This README file contains information on the contents of the meta-openbmc-nuvoton-addon layer.

Please see the corresponding sections below for details.

## Table of Contents

<!--ts-->
   * [meta-openbmc-nuvoton-addon](#meta-openbmc-nuvoton-addon)
   * [Table of contents](#table-of-contents)
   * [Dependencies](#dependencies)
   * [Patches](#patches)
   * [Usage](#usage)
      * [Adding the meta-openbmc-nuvoton-addon layer to your build](#adding-the-meta-openbmc-nuvoton-addon-layer-to-your-build)
   * [Features](#features)
      * [Remote KVM](#remote-kvm)
        * [Source URL](#source-url)
        * [How to use](#how-to-use)
        * [Maintainer](#maintainer)
   * [Modifications](#modifications)
<!--te-->

## Dependencies

This layer depends on:
```
  URI: git@github.com:Nuvoton-Israel/openbmc
  branch: npcm7xx_v2.1
```

## Patches

Please submit any patches against the meta-openbmc-nuvoton-addon layer to the maintainer of nuvoton:

```
Joseph Liu <KWLIU@nuvoton.com>
Medad CChien <CTCCHIEN@nuvoton.com>
Tyrone Ting <KFTING@nuvoton.com>
Stanley Chu <YSCHU@nuvoton.com>
Ray Lin <CHLIN59@nuvoton.com>
Tim Li <CHLI30@nuvoton.com>
```
## Usage

### Adding the meta-openbmc-nuvoton-addon layer to your build

In order to use this layer, you need to make the build system aware of
it.

Assuming the meta-openbmc-nuvoton-addon layer exists at the top-level of your
yocto build tree, you can add it to the build system by adding the
location of the meta-openbmc-nuvoton-addon layer to bblayers.conf, along with any
other layers needed. e.g.:
```
BBLAYERS ?= " \
    /path/to/yocto/meta \
    /path/to/yocto/meta-poky \
    /path/to/yocto/meta-yocto-bsp \
    /path/to/yocto/meta-meta-openbmc-nuvoton-addon \
    "
```

## Features

### Remote KVM

This is a Virtual Network Computing (VNC) server programm using [LibVNCServer](https://libvnc.github.io/).

In order to support some hardware features of Nuvoton's NPCM750, we have made some modifications to [LibVNCServer](https://libvnc.github.io/).
1) Add Video Capture and Differentiation(VCD), compares frame by hardware.
2) Add Encoding Compression Engine(ECE), 16-bit hextile compression hardware encoding.
3) Add USB HID, support Keyboard and Mouse.

#### Source URL:

https://github.com/NTC-CCBG/soc-remote-media

#### How to use:

1) launch remote-kvm
```
./remote-kvm &

* Please skip this step if remote-kvm daemon is managed by systemd
```
2) open VNC viewer
```
IP xxx.xxx.xxx.xxx
port 5900

* Please choose hextile as preferred encoding.
```
#### Maintainer
Joseph Liu

### Modifications

* 2018.07.23 First release Remote-KVM
