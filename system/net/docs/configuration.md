---
grand_parent: Services
parent: Net Service
title: Net System Service Configuration
has_toc: true
nav_order: 1
---

# Net System Service Configuration
The NET System Service library should be configured through the MHC. When user selects the NET System Service library, all the required dependencies components are added automatically in the MHC configuration. In the NET System Service library, user can select the mode as Client or Server and make a required changes for the selected mode.

The following figure shows the MHC configuration window for configuring the NET System Service and a brief description of various configuration options.
1. Open the MHC 3
2. Drag the Net Service Module into the Active components from the Available components
![](./images/Net_service_MHC.png)

3. Configure the various parameters
![](./images/Net_service_config.png)

4. Configure the Supported Interface - 'Wifi Only', or 'Wifi and Ethernet Both'
![](./images/Net_service_SuppIntf.png)

5. User can configure 2 intsances of a Net Socket. By default, only the first one is enabled.

6. Instance 0:

    1. Configure the Network Interface as Wifi or Ethernet. Note that Ethernet as an interface can only be chosen if the 'Supported Interaces' param is WIFI_ETHERNET.
![](./images/Net_service_Inst0_Intf.png)

    2. Configure the Ip Protocol as either TCP or UDP
![](./images/Net_service_config_ipprot.png)

    3. Configure the Mode as either Client or Server
![](./images/Net_service_config_mode.png)

    4. Enable/ Disable "Auto Connect" as per your requirement. This parameter when enabled ensures that if the NET Connection disconnects, the service internally tries to reconnect. By Default, the parameter value is 'True'.

    5. Enable/ Disable "Enable TLS" in case the connection needs to be secured. Please note that in case this parameter is Enabled, the User needs to configure the WolfSSL related configuration on his own. Also, this parameter is valid only for TCP Connections. By Default, the parameter value is 'False'.<br>Note: In case the TLS is enabled, the User needs to update the component 'Presentation Layer' with the CA Certificate format, location, name, and size. Other parameters can be updated as per the User's requirements.

        - Configure the various parameters of Presentation Layer if TLS enabled
![](./images/presentation_layer.png)


    6. Server Port - 1-65535. This is a mandatory parameter.

    7. Host Name/ IP Address: Can be a Host Name or an IP Address. By Default, the parameter value is '192.168.1.1'.
    
7. Enable CLI Commands - This is enabled by default. This can be used by the user to give commands on the CLI to open/ close/ send message on a socket.
8. Enable Debug Logs in case more prints are required for debugging. By Default, the parameter value is 'False'.<br>Note: In case the user enables debug logs, user needs to manually add the 'App Debug Service' component from Wireless-> System Service-> App Debug Service.


All of the required files are automatically added into the MPLAB X IDE project by the MHC when the Net Service is selected for use.
