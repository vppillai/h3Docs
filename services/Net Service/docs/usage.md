---
grand_parent: Services
parent: Net Service
title: Net System Service Usage
has_toc: true
nav_order: 1
---

# Net System Service Usage
## Description
The NET System Service provides simple APIs to enable Server or Client Connectivity for either TCP or UDP. The User need not take care of intermediate states of a TCPIP Connection, as the Service internally takes care of that. User is not required to have Security domain knowledge to establish a secured connection via the application using NET System Service library.

### Command Line:
User can follow below commands for NET System Service: 

| Command         | Details                                                                                     | Example                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
|-----------------|---------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| sysnethelp      |    <br>NET System Service help command which displays   the supported CLI commands          |    <br>   <br>   <br>>sysnethelp<br><br>          SysNet commands:<br><br>          1) sysnet open ?                -- To   open the sysnet service<br><br>          2) sysnet close ?               -- To   close the sysnet service<br><br>          3) sysnet send ?                -- To   send message via the sysnet   service<br><br>          4) sysnet get info ?            -- To   get list of sysnet service instances                                                                                                                                                                                                                                                                                                                          |
| sysnet open     | Command   for Reconfiguring an already open instance of Net System Service                  | sysnet 0 1   google.com 443 tls_enable 1 auto_reconnect 1                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| sysnet close    | Command   to sclose the socket                                                              | sysnet close 0                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| sysnet send     | Command   to send a message on an already open Instance of Net System Service               | sysnet send 0   hello                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| sysnet get info | Command   for knowing the Current Information for all the Instances of Net System   Service | ><br>     <br>          >sysnet get info<br>            *****************************************<br>     <br>          NET Service Instance: 0<br>          Status:   SYS_NET_STATUS_IDLE<br>          Mode: SYS_NET_MODE_CLIENT<br>          Socket ID: 0<br>          Host:<br>          Remote IP: 0.0.0.0<br>          Remote Port: 0<br>          Local Port: 0<br>          hNet: 0<br>            *****************************************<br>     <br>          NET Service Instance: 1<br>          Status:   SYS_NET_STATUS_IDLE<br>          Mode: SYS_NET_MODE_CLIENT<br>          Socket ID: 0<br>          Host:<br>          Remote IP: 0.0.0.0<br>          Remote Port: 0<br>          Local Port: 0<br>          hNet: 0<br>          > |
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


