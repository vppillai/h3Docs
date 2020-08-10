# coding: utf-8
"""*****************************************************************************
* Copyright (C) 2018 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*****************************************************************************"""

################################################################################
#### Global Variables ####
################################################################################

################################################################################
#### Business Logic ####
################################################################################

################################################################################
#### Component ####
################################################################################
def instantiateComponent(syswifiprovComponent):

    res = Database.activateComponents(["HarmonyCore"])

    # Enable dependent Harmony core components
    if (Database.getSymbolValue("HarmonyCore", "ENABLE_SYS_COMMON") == False):
        Database.clearSymbolValue("HarmonyCore", "ENABLE_SYS_COMMON")
        Database.setSymbolValue("HarmonyCore", "ENABLE_SYS_COMMON", True)

    if (Database.getSymbolValue("HarmonyCore", "ENABLE_DRV_COMMON") == False):
        Database.clearSymbolValue("HarmonyCore", "ENABLE_DRV_COMMON")
        Database.setSymbolValue("HarmonyCore", "ENABLE_DRV_COMMON", True)


    syswifiprovNvmAdd = syswifiprovComponent.createStringSymbol("SYS_WIFIPROV_NVMADDR", None)
    syswifiprovNvmAdd.setLabel("WiFi Configuration Stored At NVM Address")
    syswifiprovNvmAdd.setVisible(True)
    syswifiprovNvmAdd.setDescription("Enter 4KB Aligned NVM Address for storing WiFi Configuration")
    syswifiprovNvmAdd.setDefaultValue("0x900F0000")

    syswifiprovstaEnable = syswifiprovComponent.createBooleanSymbol("SYS_WIFIPROV_STA_ENABLE", None)
    syswifiprovstaEnable.setVisible(False)
    syswifiprovstaEnable.setDefaultValue((Database.getSymbolValue("sysWifiPic32mzw1", "SYS_WIFI_STA_ENABLE") == True))
    syswifiprovstaEnable.setDependencies(syswifiprovCustomSet, ["sysWifiPic32mzw1.SYS_WIFI_STA_ENABLE"])

    syswifiprovapEnable = syswifiprovComponent.createBooleanSymbol("SYS_WIFIPROV_AP_ENABLE", None)
    syswifiprovapEnable.setVisible(False)
    syswifiprovapEnable.setDefaultValue((Database.getSymbolValue("sysWifiPic32mzw1", "SYS_WIFI_AP_ENABLE") == True))
    syswifiprovapEnable.setDependencies(syswifiprovCustomSet, ["sysWifiPic32mzw1.SYS_WIFI_AP_ENABLE"])

    syswifiprovdebugEnable = syswifiprovComponent.createBooleanSymbol("SYS_WIFIPROV_APPDEBUG_ENABLE", None)
    syswifiprovdebugEnable.setVisible(False)
    syswifiprovdebugEnable.setDefaultValue((Database.getSymbolValue("sysWifiPic32mzw1", "SYS_WIFI_APPDEBUG_ENABLE") == True))
    syswifiprovdebugEnable.setDependencies(syswifiprovCustomSet, ["sysWifiPic32mzw1.SYS_WIFI_APPDEBUG_ENABLE"])
    syswifiprovdebugEnable.setDependencies(syswifiprovCustomSet, ["sysWifiPic32mzw1.SYS_WIFI_PROVISION_ENABLE"])

    syswifiprovSave = syswifiprovComponent.createBooleanSymbol("SYS_WIFIPROV_SAVECONFIG", None)
    syswifiprovSave.setLabel("Save Configuration in NVM")
    syswifiprovSave.setDefaultValue(True)
    syswifiprovSave.setDescription("Enable Option to store Wi-Fi Configuration to NVM")
    syswifiprovSave.setDependencies(syswifiprovMenuVisible, ["SYS_WIFIPROV_ENABLE"])

    syswifiprovMethod = syswifiprovComponent.createMenuSymbol("SYS_WIFIPROV_METHOD", None)
    syswifiprovMethod.setLabel("WiFi Provisioning Methods")
    syswifiprovMethod.setVisible(True)

    syswifiprovcmd = syswifiprovComponent.createBooleanSymbol("SYS_WIFIPROV_CMD", syswifiprovMethod)
    syswifiprovcmd.setLabel("Command Line (CLI) ")
    syswifiprovcmd.setDefaultValue(True)
    syswifiprovcmd.setDescription("Enable WiFi Provisioning via CLI")
    syswifiprovcmd.setDependencies(syswifiprovMenuVisible, ["SYS_WIFIPROV_ENABLE"])

    syswifiprovhttp = syswifiprovComponent.createBooleanSymbol("SYS_WIFIPROV_HTTP", syswifiprovMethod)
    syswifiprovhttp.setLabel("HTTP Pages ")
    syswifiprovhttp.setVisible(True)
    syswifiprovhttp.setDescription("Enable WiFi Provisioning via HTTP")
    syswifiprovhttp.setDefaultValue(False)
    syswifiprovhttp.setDependencies(syswifiprovMenuVisible, ["SYS_WIFIPROV_ENABLE"])
    syswifiprovhttp.setDependencies(syswifiprovHTTPMenuVisible, ["SYS_WIFIPROV_HTTP"])
    
    #syswifiprovhttploc = syswifiprovComponent.createBooleanSymbol("SYS_WIFIPROV_HTTP_LOC", None)
    #syswifiprovhttploc.setVisible(False)
    #syswifiprovhttploc.setDefaultValue(True)
    
    syswifiprovhttpPort = syswifiprovComponent.createIntegerSymbol("SYS_WIFIPROV_HTTPPORT", syswifiprovhttp)
    syswifiprovhttpPort.setLabel("HTTP Server Port")
    syswifiprovhttpPort.setMin(1)
    syswifiprovhttpPort.setMax(65535)
    syswifiprovhttpPort.setDefaultValue(80)
    syswifiprovhttpPort.setDependencies(syswifiprovMenuVisible, ["SYS_WIFIPROV_HTTP"])


    syswifiprovsocket = syswifiprovComponent.createBooleanSymbol("SYS_WIFIPROV_SOCKET", syswifiprovMethod)
    syswifiprovsocket.setLabel("TCP Socket ")
    syswifiprovsocket.setDefaultValue(True)
    syswifiprovsocket.setDescription("Enable WiFi Provisioning via SOcket")
    syswifiprovsocket.setDependencies(syswifiprovMenuVisible, ["SYS_WIFIPROV_ENABLE"])

    syswifiprovsocketPort = syswifiprovComponent.createIntegerSymbol("SYS_WIFIPROV_SOCKETPORT", syswifiprovsocket)
    syswifiprovsocketPort.setLabel("Socket Server Port")
    syswifiprovsocketPort.setMin(1)
    syswifiprovsocketPort.setMax(65535)
    syswifiprovsocketPort.setDefaultValue(6666)
    syswifiprovsocketPort.setDescription("Enter TCP Server SOcket Port Number ")
    syswifiprovsocketPort.setDependencies(syswifiprovMenuVisible, ["SYS_WIFIPROV_SOCKET"])

    if(Database.getSymbolValue("tcpipHttp", "TCPIP_HTTP_CUSTOM_TEMPLATE_SL") == True):
       Database.setSymbolValue("tcpipHttp", "TCPIP_HTTP_CUSTOM_TEMPLATE_SL", False)
#    if(Database.getSymbolValue("sysWifiProvPic32mzw1", "SYS_WIFIPROV_HTTP") == True):
#        res = Database.activateComponents(["sys_fs"],"System Component", True)
#        if(Database.getSymbolValue("tcpip_apps_config", "TCPIP_AUTOCONFIG_ENABLE_HTTP_SERVER") != True):
#            Database.setSymbolValue("tcpip_apps_config", "TCPIP_AUTOCONFIG_ENABLE_HTTP_SERVER", True)
#        res = Database.activateComponents(["drv_memory"],"System Component", True)
#        autoConnectTableconmem = [["drv_memory_0", "drv_memory_MEMORY_dependency", "nvm","NVM_MEMORY"]]
#        autoConnectTableconfs = [["drv_memory_0", "drv_media", "sys_fs","sys_fs_DRV_MEDIA_dependency"]]
#        res = Database.connectDependencies(autoConnectTableconmem)
#        res = Database.connectDependencies(autoConnectTableconfs)
#        Database.setSymbolValue("sys_fs", "SYS_FS_FAT", False)
#        Database.setSymbolValue("sys_fs", "SYS_FS_MPFS", True)
#        Database.setSymbolValue("sys_fs", "SYS_FS_MAX_FILES", 10)
#        Database.setSymbolValue("tcpipHttp", "TCPIP_HTTP_CUSTOM_TEMPLATE", False)
#        Database.setSymbolValue("drv_memory_0", "DRV_MEMORY_DEVICE_TYPE", "SYS_FS_MEDIA_TYPE_NVM")
#        Database.setSymbolValue("nvm", "MEMORY_MEDIA_SIZE", 64)
#        Database.setSymbolValue("nvm", "START_ADDRESS", "90010000")
    ############################################################################
    #### Code Generation ####
    ############################################################################
    configName = Variables.get("__CONFIGURATION_NAME")


    syswifiprovSystemConfFile = syswifiprovComponent.createFileSymbol("SYS_WIFIPROV_CONFIGURATION_H", None)
    syswifiprovSystemConfFile.setType("STRING")
    syswifiprovSystemConfFile.setOutputName("core.LIST_SYSTEM_CONFIG_H_MIDDLEWARE_CONFIGURATION")
    syswifiprovSystemConfFile.setSourcePath("system/wifiprov/templates/system/system_config.h.ftl")
    syswifiprovSystemConfFile.setMarkup(True)
    syswifiprovSystemConfFile.setDependencies(syswifiprovGenSourceFile, ["SYS_WIFIPROV_ENABLE"])

    syswifiprovHeaderFile = syswifiprovComponent.createFileSymbol("SYS_WIFIPROV_SOURCE", None)
    syswifiprovHeaderFile.setSourcePath("system/wifiprov/templates/system/sys_wifiprov.c.ftl")
    syswifiprovHeaderFile.setOutputName("sys_wifiprov.c")
    syswifiprovHeaderFile.setDestPath("system/wifiprov/src")
    syswifiprovHeaderFile.setProjectPath("config/" + configName + "/system/wifiprov/")
    syswifiprovHeaderFile.setType("SOURCE")
    syswifiprovHeaderFile.setMarkup(True)
    syswifiprovHeaderFile.setEnabled(True)
    syswifiprovHeaderFile.setDependencies(syswifiprovGenSourceFile, ["SYS_WIFIPROV_ENABLE"])

    syswifiprovSourceFile = syswifiprovComponent.createFileSymbol("SYS_WIFIPROV_HEADER", None)
    syswifiprovSourceFile.setSourcePath("system/wifiprov/templates/system/sys_wifiprov.h.ftl")
    syswifiprovSourceFile.setOutputName("sys_wifiprov.h")
    syswifiprovSourceFile.setDestPath("system/wifiprov/")
    syswifiprovSourceFile.setProjectPath("config/" + configName + "/system/wifiprov/")
    syswifiprovSourceFile.setType("HEADER")
    syswifiprovSourceFile.setMarkup(True)
    syswifiprovSourceFile.setEnabled(True)
    syswifiprovSourceFile.setDependencies(syswifiprovGenSourceFile, ["SYS_WIFIPROV_ENABLE"])

    syswifiprovcjsonHeaderFile = syswifiprovComponent.createFileSymbol("SYS_WIFIPROV_JSON_SOURCE", None)
    syswifiprovcjsonHeaderFile.setSourcePath("system/wifiprov/templates/system/sys_wifiprov_json.c")
    syswifiprovcjsonHeaderFile.setOutputName("sys_wifiprov_json.c")
    syswifiprovcjsonHeaderFile.setDestPath("system/wifiprov/src")
    syswifiprovcjsonHeaderFile.setProjectPath("config/" + configName + "/system/wifiprov/")
    syswifiprovcjsonHeaderFile.setType("SOURCE")
    syswifiprovcjsonHeaderFile.setMarkup(True)
    syswifiprovcjsonHeaderFile.setEnabled(True)
    syswifiprovcjsonHeaderFile.setDependencies(syswifiprovGenSourceFile, ["SYS_WIFIPROV_SOCKET"])

    syswifiprovcjsonSourceFile = syswifiprovComponent.createFileSymbol("SYS_WIFIPROV_JSON_HEADER", None)
    syswifiprovcjsonSourceFile.setSourcePath("system/wifiprov/templates/system/sys_wifiprov_json.h")
    syswifiprovcjsonSourceFile.setOutputName("sys_wifiprov_json.h")
    syswifiprovcjsonSourceFile.setDestPath("system/wifiprov/")
    syswifiprovcjsonSourceFile.setProjectPath("config/" + configName + "/system/wifiprov/")
    syswifiprovcjsonSourceFile.setType("HEADER")
    syswifiprovcjsonSourceFile.setMarkup(True)
    syswifiprovcjsonSourceFile.setEnabled(True)
    syswifiprovcjsonSourceFile.setDependencies(syswifiprovGenSourceFile, ["SYS_WIFIPROV_SOCKET"])
	
    syswifiprovcustomhttpSourceFile = syswifiprovComponent.createFileSymbol("SYS_WIFIPROV_CUSTOM_HTTP_SOURCE", None)
    syswifiprovcustomhttpSourceFile.setSourcePath("system/wifiprov/templates/system/custom_http_app.c")
    syswifiprovcustomhttpSourceFile.setOutputName("custom_http_app.c")
    syswifiprovcustomhttpSourceFile.setDestPath("system/wifiprov/")
    syswifiprovcustomhttpSourceFile.setProjectPath("config/" + configName + "/system/wifiprov/")
    syswifiprovcustomhttpSourceFile.setType("SOURCE")
    syswifiprovcustomhttpSourceFile.setMarkup(True)
    syswifiprovcustomhttpSourceFile.setEnabled(False)
    syswifiprovcustomhttpSourceFile.setDependencies(syswifiprovGenSourceFile, ["SYS_WIFIPROV_HTTP"])


    syswifiprovhttppSourceFile = syswifiprovComponent.createFileSymbol("SYS_WIFIPROV_HTTP_SOURCE", None)
    syswifiprovhttppSourceFile.setSourcePath("system/wifiprov/templates/system/http_print.c")
    syswifiprovhttppSourceFile.setOutputName("http_print.c")
    syswifiprovhttppSourceFile.setDestPath("system/wifiprov/")
    syswifiprovhttppSourceFile.setProjectPath("config/" + configName + "/system/wifiprov/")
    syswifiprovhttppSourceFile.setType("SOURCE")
    syswifiprovhttppSourceFile.setMarkup(True)
    syswifiprovhttppSourceFile.setEnabled(False)
    syswifiprovhttppSourceFile.setDependencies(syswifiprovGenSourceFile, ["SYS_WIFIPROV_HTTP"])
	
    syswifiprovmpfsSourceFile = syswifiprovComponent.createFileSymbol("SYS_WIFIPROV_MPFS_SOURCE", None)
    syswifiprovmpfsSourceFile.setSourcePath("system/wifiprov/templates/system/mpfs_img2.c")
    syswifiprovmpfsSourceFile.setOutputName("mpfs_img2.c")
    syswifiprovmpfsSourceFile.setDestPath("system/wifiprov/")
    syswifiprovmpfsSourceFile.setProjectPath("config/" + configName + "/system/wifiprov/")
    syswifiprovmpfsSourceFile.setType("SOURCE")
    syswifiprovmpfsSourceFile.setMarkup(True)
    syswifiprovmpfsSourceFile.setEnabled(False)
    syswifiprovmpfsSourceFile.setDependencies(syswifiprovGenSourceFile, ["SYS_WIFIPROV_HTTP"])
############################################################################
#### Dependency ####
############################################################################

def onAttachmentConnected(source, target):
    localComponent = source["component"]
    remoteComponent = target["component"]
    remoteID = remoteComponent.getID()
    connectID = source["id"]
    targetID = target["id"]

    if (connectID == "sys_wifiprov_WIFI_dependency"):
        plibUsed = localComponent.getSymbolByID("SYS_WIFI_SYS")
        plibUsed.clearValue()
        plibUsed.setValue(remoteID.upper())

def syswifiprovCustomSet(symbol, event):
    symbol.clearValue()
    if (event["value"] == True):
        symbol.setValue(True,2)
    else:
        symbol.setValue(False,2)
        

def onAttachmentDisconnected(source, target):
    localComponent = source["component"]
    remoteComponent = target["component"]
    remoteID = remoteComponent.getID()
    connectID = source["id"]
    targetID = target["id"]

def syswifiprovMenuVisible(symbol, event):
    if (event["value"] == True):
        print("WiFi Provisioning Menu Visible.")
        symbol.setVisible(True)
    else:
        print("WiFi Provisioning Menu Invisible.")
        symbol.setVisible(False)

    #if (connectID == "sys_wifiprov_WIFI_dependency"):
    #    plibUsed = localComponent.getSymbolByID("SYS_WIFI_SYS")
    #    plibUsed.clearValue()

def enableTcpipAutoConfigApps(enable):

    if(enable == True):
        tcpipAutoConfigAppsGroup = Database.findGroup("APPLICATION LAYER")
        if (tcpipAutoConfigAppsGroup == None):
            tcpipAutoConfigAppsGroup = Database.createGroup("TCP/IP STACK", "APPLICATION LAYER")
            
        tcpipAutoConfigTransportGroup = Database.findGroup("TRANSPORT LAYER")
        if (tcpipAutoConfigTransportGroup == None):
            tcpipAutoConfigTransportGroup = Database.createGroup("TCP/IP STACK", "TRANSPORT LAYER")

        tcpipAutoConfigNetworkGroup = Database.findGroup("NETWORK LAYER")
        if (tcpipAutoConfigNetworkGroup == None):
            tcpipAutoConfigNetworkGroup = Database.createGroup("TCP/IP STACK", "NETWORK LAYER")
            
        tcpipAutoConfigDriverGroup = Database.findGroup("DRIVER LAYER")
        if (tcpipAutoConfigDriverGroup == None):
            tcpipAutoConfigDriverGroup = Database.createGroup("TCP/IP STACK", "DRIVER LAYER")

        tcpipAutoConfigBasicGroup = Database.findGroup("BASIC CONFIGURATION")
        if (tcpipAutoConfigBasicGroup == None):
            tcpipAutoConfigBasicGroup = Database.createGroup("TCP/IP STACK", "BASIC CONFIGURATION")
            
        if(Database.getComponentByID("tcpip_transport_config") == None):
            res = tcpipAutoConfigTransportGroup.addComponent("tcpip_transport_config")
            res = Database.activateComponents(["tcpip_transport_config"], "TRANSPORT LAYER", False) 
        
        if(Database.getComponentByID("tcpip_network_config") == None):
            res = tcpipAutoConfigNetworkGroup.addComponent("tcpip_network_config")
            res = Database.activateComponents(["tcpip_network_config"], "NETWORK LAYER", False) 
            
        if(Database.getComponentByID("tcpip_driver_config") == None):
            res = tcpipAutoConfigDriverGroup.addComponent("tcpip_driver_config")
            res = Database.activateComponents(["tcpip_driver_config"], "DRIVER LAYER", False)       
            
        if(Database.getComponentByID("tcpip_basic_config") == None):
            res = tcpipAutoConfigBasicGroup.addComponent("tcpip_basic_config")
            res = Database.activateComponents(["tcpip_basic_config"], "BASIC CONFIGURATION", False)         
            
        if(Database.getSymbolValue("tcpip_basic_config", "TCPIP_AUTOCONFIG_ENABLE_STACK") != True):
            Database.setSymbolValue("tcpip_basic_config", "TCPIP_AUTOCONFIG_ENABLE_STACK", True, 2)
            
        if(Database.getSymbolValue("tcpip_basic_config", "TCPIP_AUTOCONFIG_ENABLE_NETCONFIG") != True):
            Database.setSymbolValue("tcpip_basic_config", "TCPIP_AUTOCONFIG_ENABLE_NETCONFIG", True, 2)

        if(Database.getComponentByID("drv_memory") != None):
           Database.setSymbolValue("drv_memory_0", "DRV_MEMORY_DEVICE_TYPE", "SYS_FS_MEDIA_TYPE_NVM")

def syswifiprovGenSourceFile(sourceFile, event):
    sourceFile.setEnabled(event["value"])
			
def syswifiprovHTTPMenuVisible(symbol, event):
    syswifiprovHTTPAutoConfigGroup = Database.findGroup("System Component")
    syswifiprovtcpipAutoConfigAppsGroup = Database.findGroup("APPLICATION LAYER")
    syswifiprovtcpipAutoConfigStackGroup = Database.findGroup("TCP/IP STACK")
    enableTcpipAutoConfigApps(True)
    
    if (event["value"] == True):
        if(Database.getSymbolValue("tcpip_apps_config", "TCPIP_AUTOCONFIG_SYS_FS_CONNECT") != True):  
            if(Database.getComponentByID("sys_fs") == None):    
                res = Database.activateComponents(["sys_fs"],"System Component", True)
                syswifiprovHTTPAutoConfigGroup.setAttachmentVisible("sys_fs", "SYS_FS")
                Database.setSymbolValue("sys_fs", "SYS_FS_FAT", False)
                Database.setSymbolValue("sys_fs", "SYS_FS_MPFS", True)
                Database.setSymbolValue("sys_fs", "SYS_FS_MAX_FILES", 10)
            
            if(Database.getComponentByID("tcpipHttp") == None):
                res = Database.activateComponents(["tcpipHttp"],"APPLICATION LAYER", False) 
                syswifiprovtcpipAutoConfigAppsGroup.setAttachmentVisible("tcpipHttp", "libtcpipHttp")
                #syswifiprovtcpipAutoConfigAppsGroup.setAttachmentVisible("tcpipHttp", "Http_TcpipFs_Dependency")
                #syswifiprovtcpipAutoConfigStackGroup.setAttachmentVisible("APPLICATION LAYER", "tcpipHttp:Http_TcpipFs_Dependency")
                #autoConnectTableHTTPFS = [["TCP/IP STACK", "APPLICATION LAYER:tcpipHttp:Http_TcpipFs_Dependency", "sys_fs", "sys_fs"]]
                #res = Database.connectDependencies(autoConnectTableHTTPFS)
                if(Database.getSymbolValue("tcpipHttp", "TCPIP_HTTP_CUSTOM_TEMPLATE_SL") == True):
                   Database.setSymbolValue("tcpipHttp", "TCPIP_HTTP_CUSTOM_TEMPLATE", False)
                if(Database.getSymbolValue("tcpipHttp", "TCPIP_HTTP_CUSTOM_TEMPLATE_SL") == True):
                   Database.setSymbolValue("tcpipHttp", "TCPIP_HTTP_CUSTOM_TEMPLATE_SL", False)
                if(Database.getSymbolValue("tcpip_transport_config", "TCPIP_AUTOCONFIG_ENABLE_TCP") != True):
                   Database.setSymbolValue("tcpip_transport_config", "TCPIP_AUTOCONFIG_ENABLE_TCP", True, 2)            
            
            if(Database.getComponentByID("drv_memory") == None):
               res = Database.activateComponents(["drv_memory"],"System Component", True)
               #syswifiprovHTTPAutoConfigGroup.setAttachmentVisible("drv_memory", "DRV_MEDIA")
               autoConnectTableconmem = [["drv_memory_0", "drv_memory_MEMORY_dependency", "nvm","NVM_MEMORY"]]
               autoConnectTableconfs = [["drv_memory_0", "drv_media", "sys_fs","sys_fs_DRV_MEDIA_dependency"]]
               res = Database.connectDependencies(autoConnectTableconmem)
               res = Database.connectDependencies(autoConnectTableconfs)
               Database.setSymbolValue("drv_memory_0", "DRV_MEMORY_DEVICE_TYPE", "SYS_FS_MEDIA_TYPE_NVM")
               Database.setSymbolValue("nvm", "MEMORY_MEDIA_SIZE", 64)
               Database.setSymbolValue("nvm", "START_ADDRESS", "90010000")

    else:
        #autoConnectTableconmem = [["drv_memory_0", "drv_memory_MEMORY_dependency", "nvm","NVM_MEMORY"]]
        #autoConnectTableconfs = [["drv_memory_0", "drv_media", "sys_fs","sys_fs_DRV_MEDIA_dependency"]]
        #res = Database.disconnectDependencies(autoConnectTableconmem)
        #res = Database.disconnectDependencies(autoConnectTableconfs)
        res = Database.deactivateComponents(["drv_memory"])
        res = Database.deactivateComponents(["sys_fs"])
        res = Database.deactivateComponents(["tcpipHttp"])

def destroyComponent(component):
    if(Database.getSymbolValue("sysWifiProvPic32mzw1", "SYS_WIFIPROV_HTTP") == True):
       res = Database.deactivateComponents(["drv_memory"])
       res = Database.deactivateComponents(["sys_fs"])
       res = Database.deactivateComponents(["tcpipHttp"])