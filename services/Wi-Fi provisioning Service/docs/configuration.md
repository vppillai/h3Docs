---
grand_parent: Services
parent: Wi-Fi provisioning Service
title: Wi-Fi provisioning System Service Configuration
has_toc: true
nav_order: 1
---

# Wi-Fi provisioning System Service Configuration

The Wi-Fi Provisioning System Service library should be configured through MHC(MPLAB Harmony Configurator). The following figure shows the MHC configuration window for configuring the Wi-Fi Provisioning System Service and a brief description of various configuration options. 

 

The Wi-Fi Provisioning System Service library MHC menu provide option to enable required Wi-Fi Provisioning methods base on user application requirements. User can select Command line and Socket mode as shown in below diagram. 


![](./images/SYS_WiFi_Provision_MHC_diagram.png)

## Configuration Options:

- WiFi Configuration Stored at NVM Address(Program Flash memory): 
    - NVM Address for storing Wi-Fi Configuration. 
    - User can change this configuration value with program flash memory page aligned address. 
    - User has to make sure this NVM address(Program Flash memory) page is not overwritten by application code. 
- WiFi Provisioning Enable in STA mode 
    - Configuration for Wi-Fi Provisioning in STA mode. 
    - Enables this configuration only when Wi-Fi system service STA mode is enabled. 
- WiFi Provisioning Enable in AP mode 
    - Configuration for Wi-Fi Provisioning in AP mode. 
    - Enables this configuration only when Wi-Fi system service AP mode is enabled. 
- WiFi Provisioning with Command Line: 
    - Enable/Disable Wi-Fi Provision using command line. 
- WiFi Provisioning with TCP socket: 
    -  Enable/Disable Wi-Fi Provision using TCP Socket. 
    -  TCP Socket Number: 
        - User configuration for TCP Server Socket. 
 

When user want to enable Wi-Fi Provisioning service along with Wi-Fi System Service, then MHC configuration options "Enable Wi-Fi Provisioning service" and "Save Configuration in NVM" should be enabled. 

![](./images/Wi-Fi_system_MHC_block_diagram.png)
 

