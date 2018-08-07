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
      * [SOL](#sol)
        * [SOL URL](#sol-url)
        * [SOL How to use](#sol-how-to-use)
        * [SOL Maintainer](#sol-maintainer)
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

### SOL

The Serial over LAN (SoL) console redirects the output of the server’s serial port to a browser window on your workstation.

This is a patch for enabling SOL in [phosphor-webui](https://github.com/openbmc/phosphor-webui) on Nuvoton's NPCM750.

The patch provides the [obmc-console](https://github.com/openbmc/obmc-console) configuration and an update to [phosphor-rest-server](https://github.com/openbmc/phosphor-rest-server).

It's verified with Nuvoton's NPCM750 solution and Supermicro MBD-X9SCL-F-0.

#### SOL URL

https://github.com/NTC-CCBG/meta-openbmc-nuvoton-addon/tree/master/recipes-phosphor/console

https://github.com/NTC-CCBG/meta-openbmc-nuvoton-addon/tree/master/recipes-phosphor/interfaces

#### SOL How to use

1) Prepare a Poleg EVB with up-to-date boot block, Uboot and OpenBMC versions with this SOL patch applied.  
&ensp;&ensp;Check with Nuvoton support for the most recent versions.

2) Prepare a Supermicro MBD-X9SCL-F-0 motherboard and a LPC cable.

    2-1) The UEFI firmware version in Supermicro MBD-X9SCL-F-0 for verification is 2.15.1234.

3) Steps to connect the JTPM header on Supermicro MBD-X9SCL-F-0 to the J10 header on Poleg EVB with the LPC cable:

    3-1) Connect Pin 1 of Supermicro JTPM header to Pin 1 of Poleg EVB J10 header.  
    3-2) Connect Pin 2 of Supermicro JTPM header to Pin 2 of Poleg EVB J10 header.  
    3-3) Connect Pin 3 of Supermicro JTPM header to Pin 3 of Poleg EVB J10 header.  
    3-4) Connect Pin 5 of Supermicro JTPM header to Pin 5 of Poleg EVB J10 header.  
    3-5) Connect Pin 7 of Supermicro JTPM header to Pin 7 of Poleg EVB J10 header.  
    3-6) Connect Pin 8 of Supermicro JTPM header to Pin 8 of Poleg EVB J10 header.  
    3-7) Connect Pin 10 of Supermicro JTPM header to Pin 10 of Poleg EVB J10 header.  
    3-8) Connect Pin 11 of Supermicro JTPM header to Pin 11 of Poleg EVB J10 header.  
    3-9) Connect Pin 12 of Supermicro JTPM header to Pin 12 of Poleg EVB J10 header.  
    3-10) Connect Pin 15 of Supermicro JTPM header to Pin 15 of Poleg EVB J10 header.  
    3-11) Connect Pin 16 of Supermicro JTPM header to Pin 16 of Poleg EVB J10 header.  
    3-12) Connect Pin 17 of Supermicro JTPM header to Pin 17 of Poleg EVB J10 header.  

4) Steps to copy UEFI SOL related drivers to a USB drive.  

    4-1) Format the USB drive in FAT or FAT32.  
    4-2) Download PolegSerialDxe.efi and TerminalDxe.efi from  
         https://github.com/Nuvoton-Israel/openbmc-uefi-util/tree/npcm7xx_v2.1/sol_binary  
		 and copy them to the USB drive.

5) Power up the Poleg EVB and steps to prepare a working terminal for Poleg:

    5-1) Download and install the USB-to-UART driver from: http://www.ftdichip.com/Drivers/VCP.htm  
         according to the host OS in your workstation.  
    5-2) Connect a micro usb cable from your workstation to J2 header of Poleg EVB.  
    5-3) Wait for the FTDI driver to be installed automatically.  
         The COM port number is assigned automatically.  
    5-4) Open a terminal (e.g., Tera Term version 4.87) and set the correct COM port number  
         assigned by the FTDI driver (in Step 5-3).  
    5-5) The COM port should be configured as follows: 115200, 8 bit, 1 stop-bit,  
         no parity no flow control.  
    5-6) Press and release the PWR-ON-RST (SW3) button to issue a Power-On-reset.  
         It's expected to see messages output by Poleg on the terminal.  
         Use the following login name/password to login into Poleg.
         Login name: root  
         Login password: 0penBmc  

6) Steps to configure Supermicro MBD-X9SCL-F-0 UEFI setting for SOL:

    6-1) Do not plug any bootable device into Supermicro MBD-X9SCL-F-0.  
    6-2) Power up Supermicro MBD-X9SCL-F-0 and boot into UEFI setting.  
    6-3) Navigate to "Super IO Concifugration" in "Advanced" menu option and   
		enter into "Super IO Concifugration".  
    6-4) Configure serial port 1 to "IO=3E8h; IRQ=5;" and then disable it.  
    6-5) Go back to the main UEFI setting.  
    6-6) Navigate to "Boot" menu option and select "UEFI: Built-in EFI Shell" as Boot Option #1.  
&ensp;&ensp;&ensp;&ensp;6-6-1) Make sure that the rest boot options are set to "Disabled".  
    6-7) Navigate to "Exit" menu option and select "Save changes and Reset".  
    6-8) Press "Yes" in the prompt window and it will reboot then.  
    6-9) Wait for Supermicro MBD-X9SCL-F-0 to boot into UEFI shell.  
    6-10) Plug the USB drive prepared in 4) into Supermicro MBD-X9SCL-F-0's usb slot.  
    6-11) Input the following command at UEFI shell prompt, press enter key and  
		it will route to UEFI shell again.  
&ensp;&ensp;&ensp;exit  
    6-12) Check the device mapping table of the USB drive in UEFI shell. It is "fs0:"  
		here for example.  
    6-13) Input the following command at UEFI shell prompt, press enter key and the prompt will  
		show "fs0:\>" from now.  
&ensp;&ensp;&ensp;fs0:  
    6-14) Input the following command at UEFI shell prompt and press the enter key.  
&ensp;&ensp;&ensp;load PolegSerialDxe.efi  
    6-15) Input the following command at UEFI shell prompt and press the enter key.  
&ensp;&ensp;&ensp;load TerminalDxe.efi  
    6-16) Unplug the usb drive.  
    6-17) Input the following command at UEFI shell prompt and it will route to the UEFI setting.  
&ensp;&ensp;&ensp;exit

7) Prepare a PC or use a virtual pc software to install Ubuntu 14.04, 64 bit on your workstation.  
    7-1) Boot Ubuntu and log in as a normal user.

8) Open a terminal in Ubuntu 14.04. Steps to install and execute software for SOL:

    8-1) Install git:  
		Input the following command in the opened terminal and press enter key:  
&ensp;&ensp;&ensp;sudo apt-get install git  
    8-2) Install Nodejs:  
&ensp;&ensp;&ensp;&ensp;8-2-1) Input the following command in the opened terminal and press enter key:  
&ensp;&ensp;&ensp;&ensp;&ensp;&ensp;curl -sL https://deb.nodesource.com/setup_10.x | sudo -E bash -  
&ensp;&ensp;&ensp;&ensp;8-2-2) Input the following command in the opened terminal and press enter key:  
&ensp;&ensp;&ensp;&ensp;&ensp;&ensp;sudo apt-get install -y nodejs  
&ensp;&ensp;&ensp;&ensp;8-2-3) install build tools:  
&ensp;&ensp;&ensp;&ensp;Input the following command in the opened terminal and press enter key:  
&ensp;&ensp;&ensp;&ensp;&ensp;&ensp;sudo apt-get install -y build-essential  
    8-2) Install phosphor-webui:  
&ensp;&ensp;&ensp;&ensp;8-2-1) Download phosphor-webui.  
&ensp;&ensp;&ensp;&ensp;Input the following command in the opened terminal and press enter key:  
&ensp;&ensp;&ensp;&ensp;&ensp;&ensp;git clone https://github.com/openbmc/phosphor-webui.git  
&ensp;&ensp;&ensp;&ensp;8-2-2) Navigate your working path to the phosphor-webui folder in the opened terminal.  
&ensp;&ensp;&ensp;&ensp;Then input the following command in the opened terminal and press enter key:  
&ensp;&ensp;&ensp;&ensp;&ensp;&ensp;npm install  
&ensp;&ensp;&ensp;&ensp;8-2-3) Run phosphor-webui.  
&ensp;&ensp;&ensp;&ensp;Input the following command in the opened terminal and press enter key:  
&ensp;&ensp;&ensp;&ensp;&ensp;&ensp;npm run-script server  

9) Steps to configure the ethernet communication between Ubuntu 14.04 and Poleg EVB:

    9-1) Connect an ethernet cable between the PC running Ubuntu 14.04 and J7 header of Poleg EVB.  
    9-2) Configure Ubuntu 14.04 ip address to 192.168.0.1 and the netmask to 255.255.255.0  
         as an example here.  
    9-3) Configure Poleg EVB ip address to 192.168.0.2 and the netmask to 255.255.255.0.  
         For example, input the following command in the terminal connected to Poleg EVB  
         on your workstation and press enter key:  
&ensp;&ensp;&ensp;&ensp;ifconfig eth0 192.168.0.2 netmask 255.255.255.0
         

10) Steps to run SOL:

    10-1) Launch a browser in Ubuntu 14.04, open a tab window and navigate to https://192.168.0.2.  
    10-2) By pass the secure warning. You will see a JSON response with Login required message.  
    10-3) In the same tab window, navigate to http://localhost:8080. Enter the BMC IP  
          (which is 192.168.0.2 as an example here, Username and Password (defaults: root/0penBmc)).  
    10-4) You will see the OpenBMC management screen.  
    10-5) Click "Server control" at the left side of the OpenBMC management screen.  
    10-6) A "Serial over LAN console" menu item prompts then and click it.  
    10-7) A specific area will display the UEFI setting of Supermicro MBD-X9SCL-F-0.  
    10-8) (Optional) If the area doesn't display the UEFI setting clearly, use the mouse pointer  
		  to click in the area and press the "Esc" key.  
&ensp;&ensp;&ensp;&ensp;10-8-1) It shows a prompt window named "Exit Without Saving", choose "No" and press enter key  
&ensp;&ensp;&ensp;&ensp;to refresh the area for showing UEFI setting entirely.

#### SOL Maintainer
Tyrone Ting

Stanley Chu

### Modifications

* 2018.07.23 First release Remote-KVM
* 2018.08.02 First release SOL
* 2018.08.07 Modify Readme.md for adding description about SOL How to use
