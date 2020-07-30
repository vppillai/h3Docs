---
grand_parent: Services
parent: Net Service
title: Net System Service Usage
has_toc: true
nav_order: 1
---

# Net System Service Usage

|Command   |Details   |Example   |
|---|---|---|
|sysnethelp   |NET System Service help command which displays the supported CLI commands   |>sysnethelp
SysNet commands:

1) sysnet open ?                -- To open the sysnet service

2) sysnet close ?               -- To close the sysnet service

3) sysnet send ?                -- To send message via the sysnet service

4) sysnet get info ?            -- To get list of sysnet service instances
>   |
|sysnet get info   |Command for knowing the Current Information for all the Instances of Net System Service   |>
>sysnet get info


*****************************************
NET Service Instance: 0
Status: SYS_NET_STATUS_IDLE
Mode: SYS_NET_MODE_CLIENT
Socket ID: 0
Host:
Remote IP: 0.0.0.0
Remote Port: 0
Local Port: 0
hNet: 0

*****************************************
NET Service Instance: 1
Status: SYS_NET_STATUS_IDLE
Mode: SYS_NET_MODE_CLIENT
Socket ID: 0
Host:
Remote IP: 0.0.0.0
Remote Port: 0
Local Port: 0
hNet: 0
>
>   |
|sysnet open   |Command for Reconfiguring an already open instance of Net System Service   |sysnet 0 1 google.com 443 tls_enable 1 auto_reconnect 1   |
|sysnet send   |Command to send a message on an already open Instance of Net System Service   |sysnet send 0 hello   |
|sysnet close   |Command for closing the socket   |sysnet close 0   |
## Abstraction Model

The NET System Service library provides an abstraction to the NetPres/ TCPIP APIs to provide following functionalities.

- Connectivity for TCP Client 
- Connectivity for TCP Server 
- Connectivity for UDP Client 
- Connectivity for UDP Server 
- Self Healing 
- Reduce code user has to write 
- Reduce time to develop and maintain 
 
The following diagram depicts the Net System Service abstraction model. 

![](./images/NetService_abstract.png)

## How The Library Works

By default MHC generated code provides all the functionalities to enable Client or Server mode applicatation, with TCP or UDP as the IP Protocol. User needs to configure the required Client or Server mode configuration using MHC. User needs to call the SYS_NET_Open() API with a valid callback to open an instance of the Client/ Server configured in the MHC. 

![](./images/NetOpen.png)

The User Application is expected to call SYS_NET_Task() API periodically as this API ensures that the Net System service is able to execute its state machine to process any messages and invoke the user callback for any events. 

![](./images/NetTask.png)

The User Application can call SYS_NET_CtrlMsg() API in case it wants to disconnect the opened connection or to reconnect using different configuration.

![](./images/NetCtrlMsg.png)


