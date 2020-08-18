---
parent: Example Applications
title: Firmware Update
has_toc: true
has_children: false
has_toc: false
nav_order: 1

family: PIC32MZW1
function: Firmware update
---

# Firmware Update 

This example demonstrates the use of commands to create and program firmware in to WILC1000 device.

## Description

This documents explains how to use the tools to prepare a firmware binary image and to program the prepared image to the WINC1500 device. Two seperate commands are available for preparing and downloading the image. The image_tool is used for preparing a consolidated image taking input from defauls flash_image.xml file and the other tool winc_programmer_UART tool is used to read the image from WINC1500, write the image to WINC1500 and verify the written image. 

## Downloading and building the application

To download or clone this application from Github, go to the [top level of the repository](https://github.com/Microchip-MPLAB-Harmony/wireless)


Path of the application within the repository is **apps/tcp_client/firmware** .

To build the application, refer to the following table and open the project using its IDE.

| Project Name      | Description                                    |
| ----------------- | ---------------------------------------------- |
| pic32mz_w1_curiosity_freertos.X | MPLABX project for PIC32MZ W1 Curiosity Board |
|||

## Setting up PIC32MZ W1 Curiosity Board

- Connect the Debug USB port on the board to the computer using a micro USB cable
- On the GPIO Header (J207), connect U1RX (PIN 13) and U1TX (PIN 23) to TX and RX pin of any USB to UART converter
- Home AP (Wi-Fi Access Point with internet connection)

## Building the binary firmware image

WINC1500 memory is divided in to following sections:
- Boot firmware
- Control sector
- PLL table
- Gain table
- TLS root certificates
- TLS local certificates
- Downloader firmware
- WiFi application firmware
- Ate firmware

The image_tool collects all the firmware for each section and combine  it in to one firmware called m2m_image_3a0.bin
The Image_tool gathers the above-mentioned section and its address information from flash_image XML file. Please refer flash_image XML file for more information on how memory is divided.

image_tool and the configuration XML file both can be found under "firmware" directory inside firmware_update_project.

### Command to create a compound programmable binary image

image_tool.exe -c flash_image.config -o firmware\m2m_image_3a0.bin -of prog

### Writing to a specified region.

image_tool.exe -c flash_image.config -o firmware\m2m_image_3a0.bin -of prog -r "root certificates"

### Command to create a compound OTA binary image

image_tool.exe -c flash_image.config -c c Tools\gain_builder\gain_sheets\new_gain.config -o ota_firmware\m2m_ota_3A0.bin -of winc_ota -s ota

### For more information enter image_tool help command:

image_tool -h

## Programming the binary firmware image

There are two tools  available to program the WINC1500 device.
1.	winc_programmer_UART – Device to program it via UART interface
2.	winc_programmer_I2C - Device to program it via I2C interface

For winc_programmer_UART tool to work as expected, an application called serial_bridge is used in the host SAM device which acts as bridge between the programmer winc_programmer_UART tool and the WINC device.
This serial_bridge applicaiton as the name implies acts a bridge between winc_programmer_UART and WINC1500 device.

Serial bridge applications for the corresponding Host device can be found in the firmware\Tools\serial_bridge path under firmware_update_project

But the user can directly use MPLab serial bridge application to download it into the Host memory.

winc_programmer_UART <----> samd21_xplained_pro_serial_bridge.elf <----> WINC device

Similarly winc_programmer_I2C uses Aardvark tool to communicaite with WINC1500 device. 
Please note this document covers only the winc_programmer_UART tool.


### command to program WINC device using winc_programmer_UART tool:

Step 1: Erase WINC1500 memory

winc_programmer_UART.exe  -p \\.\COM16 -d winc1500 -e -pfw programmer_firmware\release3A0\programmer_release_text.bin

Step 2: Write the binary image m2m_image_3A0.bin to WINC1500 memory

winc_programmer_UART.exe  -p \\.\COM16 -d winc1500 -i m2m_image_3A0.bin -if prog -w -pfw programmer_firmware\release3A0\programmer_release_text.bin

Step 3: Read back the written image from WINC1500 memory

winc_programmer_UART.exe  -p \\.\COM16 -d winc1500 -r -pfw programmer_firmware\release3A0\programmer_release_text.bin

Step 4: Verify the written data with binary image m2m_image_3A0.bin

winc_programmer_UART.exe  -p \\.\COM16 -d winc1500 -i m2m_image_3A0.bin -r -o outputfile.bin -of prog  -pfw programmer_firmware\release3A0\programmer_release_text.bin

All the above step can be consolidated in to single command:

winc_programmer_UART.exe  -p \\.\COM16 -d winc1500 -e -i m2m_image_3A0.bin -if prog -w -r -pfw programmer_firmware\release3A0\programmer_release_text.bin


programmer_release_text.bin is the programming firmware which used to program the divice.
COM Port will change based on the port number to which the Host device is connected in user system

### For more information enter image_tool help command:

winc_programmer_UART.exe -h
























