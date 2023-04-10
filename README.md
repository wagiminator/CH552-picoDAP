# picoDAP
The CH552E-based picoDAP is a CMSIS-DAP compliant debugging probe with SWD protocol support. It can be used to program Microchip SAM and other ARM-based microcontrollers. The Firmware is based on [Ralph Doncaster's](https://github.com/nerdralph/ch554_sdcc/tree/master/examples/CMSIS_DAP) DAPLink-implementation for CH55x microcontrollers and Deqing Sun's [CH55xduino](https://github.com/DeqingSun/ch55xduino) port.

![picoDAP_pic1.jpg](https://raw.githubusercontent.com/wagiminator/CH552-picoDAP/main/documentation/picoDAP_pic1.jpg)

# CMSIS-DAP
CMSIS-DAP provides a standardized way to access the Coresight Debug Access Port (DAP) of an ARM Cortex microcontroller via USB. CMSIS-DAP is generally implemented as an on-board interface chip, providing direct USB connection from a development board to a debugger running on a host computer on one side, and over JTAG (Joint Test Action Group) or SWD (Serial Wire Debug) to the target device to access the Coresight DAP on the other. As a USB HID compliant device, it typically does not require any drivers for the operating system. For more information refer to the [CMSIS-DAP Handbook](https://os.mbed.com/handbook/CMSIS-DAP).

![CMSIS-DAP.png](https://raw.githubusercontent.com/wagiminator/CH552-picoDAP/main/documentation/picoDAP_CMSIS-DAP.png)

# Compiling and Installing Firmware
## Preparing the CH55x Bootloader
### Installing Drivers for the CH55x Bootloader
On Linux you do not need to install a driver. However, by default Linux will not expose enough permission to upload your code with the USB bootloader. In order to fix this, open a terminal and run the following commands:

```
echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="4348", ATTR{idProduct}=="55e0", MODE="666"' | sudo tee /etc/udev/rules.d/99-ch55x.rules
sudo service udev restart
```

For Windows, you need the [CH372 driver](http://www.wch-ic.com/downloads/CH372DRV_EXE.html). Alternatively, you can also use the [Zadig Tool](https://zadig.akeo.ie/) to install the correct driver. Here, click "Options" and "List All Devices" to select the USB module, and then install the libusb-win32 driver. To do this, the board must be connected and the CH55x must be in bootloader mode.

### Entering CH55x Bootloader Mode
A brand new chip starts automatically in bootloader mode as soon as it is connected to the PC via USB. Once firmware has been uploaded, the bootloader must be started manually for new uploads. To do this, the board must first be disconnected from the USB port and all voltage sources. Now press the BOOT button and keep it pressed while reconnecting the board to the USB port of your PC. The chip now starts again in bootloader mode, the BOOT button can be released and new firmware can be uploaded within the next couple of seconds.

## Compiling and Uploading using the makefile
### Installing SDCC Toolchain for CH55x
Install the [SDCC Compiler](https://sdcc.sourceforge.net/). In order for the programming tool to work, Python3 must be installed on your system. To do this, follow these [instructions](https://www.pythontutorial.net/getting-started/install-python/). In addition [pyusb](https://github.com/pyusb/pyusb) must be installed. On Linux (Debian-based), all of this can be done with the following commands:

```
sudo apt install build-essential sdcc python3 python3-pip
sudo pip install pyusb
```

### Compiling and Uploading Firmware
- Open a terminal.
- Navigate to the folder with the makefile. 
- Connect the board and make sure the CH55x is in bootloader mode. 
- Run ```make flash``` to compile and upload the firmware. 
- If you don't want to compile the firmware yourself, you can also upload the precompiled binary. To do this, just run ```python3 ./tools/chprog.py picodap.bin```.

## Compiling and Uploading using the Arduino IDE
### Installing the Arduino IDE and CH55xduino
Install the [Arduino IDE](https://www.arduino.cc/en/software) if you haven't already. Install the [CH55xduino](https://github.com/DeqingSun/ch55xduino) package by following the instructions on the website.

### Compiling and Uploading Firmware
- Copy the .ino and .c files as well as the /src folder together into one folder and name it like the .ino file. 
- Open the .ino file in the Arduino IDE.
- Go to **Tools -> Board -> CH55x Boards** and select **CH552 Board**.
- Go to **Tools** and choose the following board options:
  - **Clock Source:**   16 MHz (internal)
  - **Upload Method:**  USB
  - **USB Settings:**   USER CODE /w 266B USB RAM
- Connect the board and make sure the CH55x is in bootloader mode. 
- Click **Upload**.

# Operating Instructions
Connect the picoDAP to the target board via the 10-pin connector or the pin header (RST / DIO / CLK / GND). Make sure the target board is powered. You can supply power via the 3V3 pin (max 150 mA) or the 5V pin (max 400 mA). Plug the picoDAP into a USB port on your PC. Since it is recognized as a Human Interface Device (HID), no driver installation is required. The picoDAP should work with any debugging software that supports CMSIS-DAP (e.g. [OpenOCD](http://openocd.org/)). Of course, it also works with the [SAMD DevBoards](https://github.com/wagiminator/SAMD-Development-Boards) in the Arduino IDE (Tools -> Programmer -> Generic CMSIS-DAP).

# References, Links and Notes
1. [EasyEDA Design Files](https://oshwlab.com/wagiminator/ch552-swd-programmer)
2. [DAPLink](https://github.com/ARMmbed/DAPLink)
3. [CH55xduino](https://github.com/DeqingSun/ch55xduino)
4. [Ralph Doncaster's Implementation](https://github.com/nerdralph/ch554_sdcc/tree/master/examples/CMSIS_DAP)
5. [CMSIS-DAP Handbook](https://os.mbed.com/handbook/CMSIS-DAP)
6. [SDCC Compiler](https://sdcc.sourceforge.net/)
7. [CH55x SDK for SDCC](https://github.com/Blinkinlabs/ch554_sdcc)
8. [OpenOCD](http://openocd.org/)
9. [SAMD DevBoards](https://github.com/wagiminator/SAMD-Development-Boards)

![picoDAP_pic2.jpg](https://raw.githubusercontent.com/wagiminator/CH552-picoDAP/main/documentation/picoDAP_pic2.jpg)

# License
![license.png](https://i.creativecommons.org/l/by-sa/3.0/88x31.png)

This work is licensed under Creative Commons Attribution-ShareAlike 3.0 Unported License. 
(http://creativecommons.org/licenses/by-sa/3.0/)
