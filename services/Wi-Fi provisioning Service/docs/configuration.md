---
grand_parent: Services
parent: Wi-Fi provisioning Service
title: Wi-Fi provisioning System Service Configuration
has_toc: true
nav_order: 1
---

# Wi-Fi System Service Configuration
The Wi-Fi System Service library should be configured through MHC(MPLAB Harmony Configurator). The following figure shows the MHC configuration window for configuring the Wi-Fi System Service and a brief description of various configuration options. 

When user select the Wi-Fi System Service library, all the required dependencies are added automatically into the MHC configuration. In the Wi-Fi System Service library, user can select the operating device mode as station(STA) or access point(AP) and make a required changes in the selected mode. 

![](./images/image4.png)

## Configuration Options:

 Using MHC menu,user can select required device mode as a station(STA)
or access point(AP)

  - Device Mode:
      - Indicates the device operation mode(STA\\AP).
  - Save Configuration in the NVM(Program flash memory):
      - Indicates the Wi-Fi configuration storing in the NVM.
      - This configuration is only valid when "Enable Wi-Fi Provisioning
        service" is enabled.
  - Number of User Register Callback:
      - Indicates the maximum number of user register callback.
  - Enable Wi-Fi Provisioning service:
      - Enables/Disables Wi-Fi Provisioning System Service functionality
        along with Wi-Fi System Service.
      - When this configuration is disabled, removed the Wi-Fi
        Provisioning System Service from MHC project graph.
  - STA Mode Configuration:
      - SSID:
          - Access Point (AP/Router) SSID to connect.
      - Security:
          - Indicates the security being used by the AP with which
            device should connect – OPEN / WPA2 / WPAWPA2 (Mixed)/ WPA3. 
      - Password:
          - Password to be used while connecting to the AP. This is
            mandatory is security mode is set to anything other than
            OPEN. It will be ignored if security mode is set to OPEN.
      - Auto Connect:
          - Indicate whether to auto connect to AP (enable) or wait for
            user input (disable).
      - Channel
          - Channel configuration details:
              - value : 0 - AP search in all the channels.
              - value : 1-13 : AP only search in specified channel.
  - AP Mode Configuration:
      - SSID:
          - Indicate AP mode SSID.
      - Security:
          - Indicate AP mode security: 
            - OPEN
            - WPA2
            - WPAWPA2(Mixed)
            - WPA3
            
      - Password:
          - Indicate AP mode password(passphrase).
      - SSID Visibility:
          - Indicate AP mode SSID visibility.
      - Channel:
          - Indicate operating channel of AP mode.

### Building The Library

All of the required files are automatically added into the MPLAB X IDE
project by the MHC when the library is selected for use.
