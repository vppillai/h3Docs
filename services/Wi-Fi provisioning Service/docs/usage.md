---
grand_parent: Services
parent: Wi-Fi provisioning Service
title: Wi-Fi provisioning System Service Usage
has_toc: true
nav_order: 1
---

# Wi-Fi provisioning System Service Usage

## Command line
User can follow below commands for Wi-Fi Provisioning, 

Information on command parameters, 

| Parameter       | Sub Parameter                                          |
| ----------------| -----------------------------------------------------  |
| bootmode        | 0 - Station(STA) mode.1- Access point(AP) mode.        |
| save config     | 0 - Do not save configuration in NVM(Program Flash Memory). 1- Save configuration in NVM .                                    |
| Channel         | In Station mode value range from 0-13, 0 - select all the channels.1-13 - select specified channel. In Access point mode value range from 1-13.                                                           |
|auto connect(only applicable in STA mode)| 0 - Don't connect to AP, wait for client request.1 - Connect to AP.                                      |
|ssid visibility (only applicable in AP mode)| 0 - Hidden SSID.1 - Broadcast SSID .                                                           |
|authtype(Security type) | 1 - OPEN Mode. 3 - WPAWPA2 (Mixed) mode. 4 - WPA2 mode.                                                                 |
|ssid(ssid name)   | SSID name |
|psk name(password)| Password/passphrase  |
|||


Note: Wi-Fi Provisioning using command line method is not recommended in production release due to security concerns. 

## Socket mode

User can follow below steps to enable Wi-Fi Provisioning,

If Wi-Fi provisioning service is configured to use TCP socket, a socket server is activated when the device boots. 
Use a laptop or mobile phone as a TCP client to connect to the device's socket server. 
Send the below JSON format data from TCP Client . 
 

Example: 

{ 
"mode": 0, "save_config": 1, 
"STA": { "ch": 0, "auto": 1, "auth": 2, "SSID": "DEMO_AP", "PWD": password"}, 
"AP": {"ch": 2, "ssidv": 1, "auth": 4, "SSID": "DEMO_AP_SOFTAPAS", "PWD": "password" } } 

 

Details of JSON Parameters, 
| Parameter       | Sub Parameter  |               Value Details     |
| ----------------| ---------------|-------------------------------  |
| mode            |                | 0 - Station(STA) mode. 1- Access point(AP) mode.|
| save_config     |                | 0 - Do not save configuration in NVM. 1- Save configuration in NVM . |
| STA             | ch (Channel)   |    In Station mode value range from 0-13,0 - select all the channels.1-13 - select specified channel. |
|                 |auto(auto connect)| 0 - Don't connect to AP, wait for client request. 1 - Connect to AP.  |
|                 |Auth(Security type) | 1 - OPEN Mode.3 - WPAWPA2 (Mixed) mode.  4 - WPA2 mode. |
|                 |SSID(ssid name)   | SSID name |
|                 |PWD(password)     | Password/passphrase  |
| AP              |ch (Channel)      | In Access point mode value range from 1-13   |
|                 |ssidv(ssid visibility)        | 0 - Hidden SSID. 1 - Broadcast SSID .  |
|                 |Auth(Security type) | 1 - OPEN Mode. 3 - WPAWPA2 (Mixed) mode. 4 - WPA2 mode. |
|                 |SSID(ssid name)   | SSID name |
|                 |PWD(password)     | Password/passphrase  |
|||

## Mobile Application
##  Webpage
## How The Library Works

The Wi-Fi Provisioning System Service implemented Command line and Socket mode Wi-Fi Provisioning method.Wi-Fi Provisioning System Service by default enabled along Wi-Fi System Service.User can make configuration changes as per their application requirement

### Execution Flow
The following diagram shows how the Command line and Socket mode Wi-Fi Provisioning methods are enabled. 

![](./images/SYS_WiFi_Provision_Seq.png)



