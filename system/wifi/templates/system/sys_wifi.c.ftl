/*******************************************************************************
  Wi-Fi System Service Implementation

  File Name:
    sys_wifi.c

  Summary:
    Source code for the Wi-Fi system service implementation.

  Description:
    This file contains the source code for the Wi-Fi system service
    implementation.
 *******************************************************************************/

//DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (C) 2020 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED AS IS WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *******************************************************************************/
//DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stdlib.h>
#include "definitions.h"
#include "driver/wifi/pic32mzw1/include/wdrv_pic32mzw_client_api.h"
#include "tcpip/src/tcpip_manager_control.h"
#include "system/wifi/sys_wifi.h"
#include "configuration.h"
<#if SYS_WIFI_PROVISION_ENABLE == true>
#include "system/wifiprov/sys_wifiprov.h"
</#if>
// *****************************************************************************
// *****************************************************************************
// Section: Type Definitions
// *****************************************************************************
// *****************************************************************************

typedef struct 
{
    /* The WiFi service current status */
    SYS_WIFI_STATUS status;
    DRV_HANDLE wdrvHandle;
    WDRV_PIC32MZW_AUTH_CONTEXT authCtx;
    WDRV_PIC32MZW_BSS_CONTEXT bssCtx;

} SYS_WIFI_OBJ; //Wi-Fi System Service Object


#define MAX_AUTO_CONNECT_RETRY                5                                                                    // Wi-Fi STA Mode, maximum auto connect retry

// *********************************************************************************************************************************************************************
// *********************************************************************************************************************************************************************
// Section: Global Data
// *********************************************************************************************************************************************************************
// *********************************************************************************************************************************************************************

static    SYS_WIFI_CALLBACK                    g_wifiSrvcCallBack[SYS_WIFI_MAX_CBS];               	                // Storing Wi-Fi Service Callbacks 
static    SYS_WIFI_OBJ                         g_wifiSrvcObj = {SYS_WIFI_STATUS_NONE,NULL};                         // Wi-Fi Service Object
static    WDRV_PIC32MZW_ASSOC_HANDLE           g_wifiSrvcDrvAssocHdl = WDRV_PIC32MZW_ASSOC_HANDLE_INVALID;          // Wi-Fi Driver ASSOC Handle
<#if SYS_WIFI_AP_ENABLE == true>
static    WDRV_PIC32MZW_MAC_ADDR               g_wifiSrvcApBssId;                                                   // In Ap mode, Connected STA MAC Address 
static    bool                                 g_wifiSrvcApConnectStatus;                                           // In Ap mode, Connection status
</#if>
<#if  SYS_WIFI_STA_ENABLE == true>
static    TCPIP_DHCP_HANDLE                    g_wifiSrvcDhcpHdl = NULL;                                             // Wi-Fi DHCP handler
static    uint32_t                             g_wifiSrvcAutoConnectRetry = 0;                                       // Wi-Fi STA Mode, Auto connect retry count
</#if>
static    SYS_WIFI_CONFIG                      g_wifiSrvcConfig;                                                     // Wi-Fi  Service Configuration Structure
static    void *                               g_wifiSrvcCookie ;                                                    // Wi-Fi Service cookie
static    bool                                 g_wifiSrvcInit = false;                                               // Wi-Fi Service init enable wait status

<#if SYS_WIFI_PROVISION_ENABLE == true>
static    char                                 g_wifiSrvcProvCookieVal = 100;                                        // Cookie for Wi-Fi Provision 
static    SYS_MODULE_OBJ                       g_wifiSrvcProvObj ;                                                   // Wi-Fi Provision Service Object
</#if>
static    OSAL_SEM_HANDLE_TYPE                 g_wifiSrvcSemaphore;	                                                 // Semaphore for Critical Section

<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
          SYS_MODULE_OBJ                       g_wifiSrvcAppDebugHdl = SYS_MODULE_OBJ_INVALID;                       // Wi-Fi service APP debug handle
          SYS_APPDEBUG_CONFIG                  g_wifiSrvcAppDbgCfg;                                                  // APP debug configuration handle

/* App Debug Print Flows */
#define    WIFI_CFG                            0x1
#define    WIFI_CONNECT                        0x2
#define    WIFI_PROVISIONING                   0x3
#define    WIFI_PROVISIONINGCMD                0x4
#define    WIFI_PROVISIONINGSOCKET             0x5

</#if>

static     uint8_t                             SYS_WIFI_DisConnect(void);
static     SYS_WIFI_RESULT                     SYS_WIFI_ConnectReq(void);
static     SYS_WIFI_RESULT                     SYS_WIFI_SetScan(uint8_t channel,bool active);


<#if SYS_WIFI_PROVISION_ENABLE == true>
static     void                                SYS_WIFI_WIFIPROVCallBack(uint32_t event, void * data,void *cookie );
</#if>


// *********************************************************************************************************************************************************************
// *********************************************************************************************************************************************************************
// Section: Local Functions

// *********************************************************************************************************************************************************************
// *********************************************************************************************************************************************************************<#if (SYS_WIFI_PROVISION_ENABLE == true) && (SYS_WIFI_AP_ENABLE == true) >
static void Soft_Reset(void) 
{

    bool int_flag = false;
    /*disable interrupts since we are going to do a sysKey unlock*/
    int_flag = (bool) __builtin_disable_interrupts();

    /* unlock system for clock configuration */
    SYSKEY = 0x00000000;
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;

    if (int_flag) {
        __builtin_mtc0(12, 0, (__builtin_mfc0(12, 0) | 0x0001)); /* enable interrupts */
    }

    RSWRSTbits.SWRST = 1;
    /*This read is what actually causes the reset*/
    RSWRST = RSWRSTbits.SWRST;

    /*Reference code. We will not hit this due to reset. This is here for reference.*/
    int_flag = (bool) __builtin_disable_interrupts();

    SYSKEY = 0x33333333;

    if (int_flag) /* if interrupts originally were enabled, re-enable them */ {
        __builtin_mtc0(12, 0, (__builtin_mfc0(12, 0) | 0x0001));
    }

}
</#if>

static inline void SYS_WIFI_CallBackFun(uint32_t event, void * data, void *cookie) 
{
    uint8_t Idx;
    
    for (Idx = 0; Idx < SYS_WIFI_MAX_CBS; Idx++) 
    {
        if (g_wifiSrvcCallBack[Idx]) 
        {
            (g_wifiSrvcCallBack[Idx])(event, data, cookie);
        }
    }
}

static inline SYS_WIFI_RESULT SYS_WIFI_REGCB(SYS_WIFI_CALLBACK callback) 
{
    SYS_WIFI_RESULT ret = SYS_WIFI_FAILURE;
    uint8_t Idx;
    
    for (Idx = 0; Idx < SYS_WIFI_MAX_CBS; Idx++) 
    {
        if (!g_wifiSrvcCallBack[Idx]) {
            g_wifiSrvcCallBack[Idx] = callback;
            ret = SYS_WIFI_SUCCESS;
            break;
        }
    }
    return ret;
}

<#if SYS_WIFI_PROVISION_ENABLE == false>
static inline void SYS_WIFI_InitConfig(SYS_WIFI_CONFIG *config) 
{   
    if(!config) 
    {   
        g_wifiSrvcConfig.mode = SYS_WIFI_DEVMODE;
        g_wifiSrvcConfig.save_config = 0;
        memcpy(g_wifiSrvcConfig.countrycode,SYS_WIFI_COUNTRYCODE,strlen(SYS_WIFI_COUNTRYCODE));
<#if SYS_WIFI_STA_ENABLE == true>
        g_wifiSrvcConfig.staconfig.channel = 0;        
        g_wifiSrvcConfig.staconfig.auto_connect = SYS_WIFI_STA_AUTOCONNECT;
        g_wifiSrvcConfig.staconfig.auth_type = SYS_WIFI_STA_AUTHTYPE;
        memcpy(g_wifiSrvcConfig.staconfig.ssid,SYS_WIFI_STA_SSID,sizeof(SYS_WIFI_STA_SSID));        
        memcpy(g_wifiSrvcConfig.staconfig.psk,SYS_WIFI_STA_PWD,sizeof(SYS_WIFI_STA_PWD));
</#if>
<#if SYS_WIFI_AP_ENABLE == true>

        g_wifiSrvcConfig.apconfig.channel = SYS_WIFI_AP_CHANNEL;
        g_wifiSrvcConfig.apconfig.ssid_visibility = SYS_WIFI_AP_SSIDVISIBILE;
        g_wifiSrvcConfig.apconfig.auth_type = SYS_WIFI_AP_AUTHTYPE;
        memcpy(g_wifiSrvcConfig.apconfig.ssid,SYS_WIFI_AP_SSID,sizeof(SYS_WIFI_AP_SSID));
        memcpy(g_wifiSrvcConfig.apconfig.psk,SYS_WIFI_AP_PWD,sizeof(SYS_WIFI_AP_PWD));     
</#if>
    } 
    else 
    {
        memcpy(&g_wifiSrvcConfig,config,sizeof(SYS_WIFI_CONFIG));
    }
}
</#if>

static inline void SYS_WIFI_SetCookie(void *cookie) 
{
    g_wifiSrvcCookie = cookie;
}

static inline SYS_WIFI_MODE SYS_WIFI_GetMode(void) 
{
    return g_wifiSrvcConfig.mode;
}

static inline bool SYS_WIFI_GetSaveConfig(void) 
{
<#if (SYS_WIFI_STA_ENABLE == true) && (SYS_WIFI_AP_ENABLE == true)>
    return (SYS_WIFI_STA == SYS_WIFI_GetMode())? (g_wifiSrvcConfig.save_config ): (g_wifiSrvcConfig.save_config);
<#elseif SYS_WIFI_STA_ENABLE == true>
    return (g_wifiSrvcConfig.save_config );
<#elseif SYS_WIFI_AP_ENABLE == true>
    return (g_wifiSrvcConfig.save_config);
</#if>
}

static inline uint8_t * SYS_WIFI_GetSSID(void) 
{
<#if (SYS_WIFI_STA_ENABLE == true) && (SYS_WIFI_AP_ENABLE == true)>
    return (SYS_WIFI_STA == SYS_WIFI_GetMode())? (g_wifiSrvcConfig.staconfig.ssid ): (g_wifiSrvcConfig.apconfig.ssid);
<#elseif SYS_WIFI_STA_ENABLE == true>
    return (g_wifiSrvcConfig.staconfig.ssid );
<#elseif SYS_WIFI_AP_ENABLE == true>
    return (g_wifiSrvcConfig.apconfig.ssid);
</#if>
}

static inline uint8_t SYS_WIFI_GetSSIDLen(void) 
{
<#if (SYS_WIFI_STA_ENABLE == true) && (SYS_WIFI_AP_ENABLE == true)>
    return (SYS_WIFI_STA == SYS_WIFI_GetMode())? (strlen((const char *)g_wifiSrvcConfig.staconfig.ssid)): (strlen((const char *)g_wifiSrvcConfig.apconfig.ssid));
<#elseif SYS_WIFI_STA_ENABLE == true>
    return strlen((const char *)g_wifiSrvcConfig.staconfig.ssid);
<#elseif SYS_WIFI_AP_ENABLE == true>
    return strlen((const char *)g_wifiSrvcConfig.apconfig.ssid);
</#if>
}

static inline uint8_t SYS_WIFI_GetChannel(void) 
{
<#if (SYS_WIFI_STA_ENABLE == true) && (SYS_WIFI_AP_ENABLE == true)>
    return (SYS_WIFI_STA == SYS_WIFI_GetMode())? (g_wifiSrvcConfig.staconfig.channel ): (g_wifiSrvcConfig.apconfig.channel);
<#elseif SYS_WIFI_STA_ENABLE == true>
    return (g_wifiSrvcConfig.staconfig.channel );
<#elseif SYS_WIFI_AP_ENABLE == true>
    return (g_wifiSrvcConfig.apconfig.channel);
</#if>
}

static inline uint8_t SYS_WIFI_GetAuthType(void) 
{
<#if (SYS_WIFI_STA_ENABLE == true) && (SYS_WIFI_AP_ENABLE == true)>
    return (SYS_WIFI_STA == SYS_WIFI_GetMode())? (g_wifiSrvcConfig.staconfig.auth_type ): (g_wifiSrvcConfig.apconfig.auth_type);
<#elseif SYS_WIFI_STA_ENABLE == true>
    return (g_wifiSrvcConfig.staconfig.auth_type );
<#elseif SYS_WIFI_AP_ENABLE == true>
    return (g_wifiSrvcConfig.apconfig.auth_type);
</#if>    
}

static inline uint8_t *SYS_WIFI_GetPsk(void) 
{
<#if (SYS_WIFI_STA_ENABLE == true) && (SYS_WIFI_AP_ENABLE == true)>
    return (SYS_WIFI_STA == SYS_WIFI_GetMode())? (g_wifiSrvcConfig.staconfig.psk ): (g_wifiSrvcConfig.apconfig.psk);
<#elseif SYS_WIFI_STA_ENABLE == true>
    return (g_wifiSrvcConfig.staconfig.psk );
<#elseif SYS_WIFI_AP_ENABLE == true>
    return (g_wifiSrvcConfig.apconfig.psk);
</#if>      
}

static inline uint8_t SYS_WIFI_GetPskLen(void) 
{
<#if (SYS_WIFI_STA_ENABLE == true) && (SYS_WIFI_AP_ENABLE == true)>
    return (SYS_WIFI_STA == SYS_WIFI_GetMode())? (strlen((const char *)g_wifiSrvcConfig.staconfig.psk) ): (strlen((const char *)g_wifiSrvcConfig.apconfig.psk));
<#elseif SYS_WIFI_STA_ENABLE == true>
    return strlen((const char *)g_wifiSrvcConfig.staconfig.psk);
<#elseif SYS_WIFI_AP_ENABLE == true>
    return strlen((const char *)g_wifiSrvcConfig.apconfig.psk);
</#if>    
}

static inline bool SYS_WIFI_GetSSIDVisibility(void) 
{
<#if (SYS_WIFI_STA_ENABLE == true) && (SYS_WIFI_AP_ENABLE == true)>
    return (SYS_WIFI_AP == SYS_WIFI_GetMode())? (g_wifiSrvcConfig.apconfig.ssid_visibility): 0;
<#elseif SYS_WIFI_STA_ENABLE == true>
    return 0;
<#elseif SYS_WIFI_AP_ENABLE == true>
    return (g_wifiSrvcConfig.apconfig.ssid_visibility);
</#if>     
}

static inline const char *SYS_WIFI_GetCountryCode(void) 
{
    return (const char *) g_wifiSrvcConfig.countrycode;
}

static inline bool SYS_WIFI_GetAutoConnect(void) 
{
<#if (SYS_WIFI_STA_ENABLE == true) && (SYS_WIFI_AP_ENABLE == true)>
    //In AP mode, auto_connect is always enabled
    return ((SYS_WIFI_STA == SYS_WIFI_GetMode() && (true == g_wifiSrvcConfig.staconfig.auto_connect)) ||(SYS_WIFI_AP == SYS_WIFI_GetMode()));
<#elseif SYS_WIFI_STA_ENABLE == true>
    return g_wifiSrvcConfig.staconfig.auto_connect;        
<#elseif SYS_WIFI_AP_ENABLE == true>
    //In AP mode, auto_connect is always enabled
    return true;
</#if>
}

static inline void SYS_WIFI_SetTaskstatus(SYS_WIFI_STATUS status) 
{
    g_wifiSrvcObj.status = status;
}

static inline SYS_WIFI_STATUS SYS_WIFI_GetTaskstatus(void) 
{
    return g_wifiSrvcObj.status;
}

static inline void SYS_WIFI_PrintConfig(void) 
{
    SYS_CONSOLE_PRINT("\r\n mode=%d (0-STA,1-AP) save_config=%d \r\n ", g_wifiSrvcConfig.mode, g_wifiSrvcConfig.save_config);
<#if SYS_WIFI_STA_ENABLE == true>
    if(g_wifiSrvcConfig.mode == SYS_WIFI_STA) 
    {
        SYS_CONSOLE_PRINT("\r\n STA Configuration :\r\n channel=%d \r\n auto_connect=%d \r\n ssid=%s \r\n passphase=%s \r\n authentication type=%d (1-Open,2-WEP,3-Mixed mode(WPA/WPA2),4-WPA2)\r\n",g_wifiSrvcConfig.staconfig.channel,g_wifiSrvcConfig.staconfig.auto_connect,g_wifiSrvcConfig.staconfig.ssid,g_wifiSrvcConfig.staconfig.psk,g_wifiSrvcConfig.staconfig.auth_type);
    }
</#if>
<#if SYS_WIFI_AP_ENABLE == true>
    if(g_wifiSrvcConfig.mode == SYS_WIFI_AP)
    {
        SYS_CONSOLE_PRINT("\r\n AP Configuration :\r\n channel=%d \r\n ssid_visibility=%d \r\n ssid=%s \r\n passphase=%s \r\n authentication type=%d (1-Open,2-WEP,3-Mixed mode(WPA/WPA2),4-WPA2) \r\n",g_wifiSrvcConfig.apconfig.channel,g_wifiSrvcConfig.apconfig.ssid_visibility,g_wifiSrvcConfig.apconfig.ssid,g_wifiSrvcConfig.apconfig.psk,g_wifiSrvcConfig.apconfig.auth_type);
    }
</#if>

}

<#if SYS_WIFI_AP_ENABLE == true>
static void SYS_WIFI_APConnCallBack(DRV_HANDLE handle, WDRV_PIC32MZW_ASSOC_HANDLE assocHandle, WDRV_PIC32MZW_CONN_STATE currentState) 
{
    switch (currentState) 
    {
        case WDRV_PIC32MZW_CONN_STATE_CONNECTED:
        {
            if (WDRV_PIC32MZW_STATUS_OK == WDRV_PIC32MZW_AssocPeerAddressGet(assocHandle, &g_wifiSrvcApBssId)) 
            {
                g_wifiSrvcApConnectStatus = true;
                g_wifiSrvcDrvAssocHdl = assocHandle;
                SYS_CONSOLE_PRINT("Connected STA MAC Address=%x:%x:%x:%x:%x:%x\r\n", g_wifiSrvcApBssId.addr[0], g_wifiSrvcApBssId.addr[1], g_wifiSrvcApBssId.addr[2], g_wifiSrvcApBssId.addr[3], g_wifiSrvcApBssId.addr[4], g_wifiSrvcApBssId.addr[5]);
                SYS_WIFI_SetTaskstatus(SYS_WIFI_STATUS_WAIT_FOR_STA_IP);
            }
            break;
        }
        case WDRV_PIC32MZW_CONN_STATE_DISCONNECTED:
        {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
            SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CONNECT, " Connected STA is disconnected\r\n");
</#if>
            if (true == g_wifiSrvcApConnectStatus) 
            {
                g_wifiSrvcDrvAssocHdl = WDRV_PIC32MZW_ASSOC_HANDLE_INVALID;
                g_wifiSrvcApConnectStatus = false;
                SYS_WIFI_CallBackFun(SYS_WIFI_DISCONNECT, NULL, g_wifiSrvcCookie);
            }
            break;
        }
        default:
        {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
            SYS_APPDEBUG_ERR_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CONNECT, " AP Callback received with invalid state\r\n");
</#if>
            break;
        }

    }

}

</#if>
<#if SYS_WIFI_STA_ENABLE == true>
static void SYS_WIFI_STAConnCallBack(DRV_HANDLE handle, WDRV_PIC32MZW_ASSOC_HANDLE assocHandle, WDRV_PIC32MZW_CONN_STATE currentState) 
{
    switch (currentState) 
    {
        case WDRV_PIC32MZW_CONN_STATE_CONNECTED:
        {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
            SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CONNECT, "STA Connected\r\n");
</#if>
            g_wifiSrvcDrvAssocHdl = assocHandle;
            g_wifiSrvcAutoConnectRetry = 0;
            break;
        }
        case WDRV_PIC32MZW_CONN_STATE_FAILED:
        {
            SYS_CONSOLE_PRINT(" Trying to connect to SSID : %s \r\n STA Connection failed. \r\n \r\n", SYS_WIFI_GetSSID());

            if ((true == SYS_WIFI_GetAutoConnect()) && (g_wifiSrvcAutoConnectRetry < MAX_AUTO_CONNECT_RETRY)) 
            {
                SYS_WIFI_SetTaskstatus(SYS_WIFI_STATUS_CONNECT_REQ);
                g_wifiSrvcAutoConnectRetry++;
            } 
            else if (g_wifiSrvcAutoConnectRetry == MAX_AUTO_CONNECT_RETRY) 
            {
                SYS_WIFI_SetTaskstatus(SYS_WIFI_STATUS_CONNECT_ERROR);
            }
            g_wifiSrvcDrvAssocHdl = WDRV_PIC32MZW_ASSOC_HANDLE_INVALID;
            break;
        }
        case WDRV_PIC32MZW_CONN_STATE_DISCONNECTED:
        {
            SYS_CONSOLE_PRINT("STA DisConnected\r\n");
            if(true == SYS_WIFI_GetAutoConnect()) 
            {
                SYS_WIFI_SetTaskstatus(SYS_WIFI_STATUS_CONNECT_REQ);
            }
            g_wifiSrvcDrvAssocHdl = WDRV_PIC32MZW_ASSOC_HANDLE_INVALID;
            g_wifiSrvcAutoConnectRetry = 0;
            break;
        }
        /*case WDRV_PIC32MZW_CONN_STATE_CONNECTING:
            break;*/
        default:
        {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
            SYS_APPDEBUG_ERR_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CONNECT, "STA Callback received with invalid state\r\n");
</#if>
            break;
        }
    }       
}
</#if>

static void SYS_WIFI_SetRegDomainCallback(DRV_HANDLE handle, uint8_t index, uint8_t ofTotal, bool isCurrent, const WDRV_PIC32MZW_REGDOMAIN_INFO * const pRegDomInfo) 
{
    if ((1 != index) || (1 != ofTotal) || (false == isCurrent) || (NULL == pRegDomInfo)) 
    {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
        SYS_APPDEBUG_ERR_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, "Regulatory domain set unsuccessful\r\n");
</#if>
    } 
    else 
    {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
        SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, "Regulatory domain set successful\r\n");
</#if>
    }
}

static void SYS_WIFI_ScanHandler(DRV_HANDLE handle, uint8_t index, uint8_t ofTotal, WDRV_PIC32MZW_BSS_INFO *pBSSInfo) 
{
    if (0 == ofTotal) 
    {
        SYS_CONSOLE_MESSAGE("No AP Found Rescan\r\n");
    } 
    else 
    {
        if (1 == index) 
        {
            char cmdTxt[8];
            sprintf(cmdTxt, "SCAN#%02d", ofTotal);
            SYS_CONSOLE_PRINT("#%02d\r\n", ofTotal);
            SYS_CONSOLE_MESSAGE(">>#  RI  Sec  Recommend CH BSSID             SSID\r\n");
            SYS_CONSOLE_MESSAGE(">>#      Cap  Auth Type\r\n>>#\r\n");
        }
        SYS_CONSOLE_PRINT(">>%02d %d 0x%02x ", index, pBSSInfo->rssi, pBSSInfo->secCapabilities);
        switch (pBSSInfo->authTypeRecommended) 
        {
            case WDRV_PIC32MZW_AUTH_TYPE_OPEN:
            {
                SYS_CONSOLE_MESSAGE("OPEN     ");
                break;
            }
            case WDRV_PIC32MZW_AUTH_TYPE_WEP:
            {
                SYS_CONSOLE_MESSAGE("WEP      ");
                break;
            }
            case WDRV_PIC32MZW_AUTH_TYPE_WPAWPA2_PERSONAL:
            {
                SYS_CONSOLE_MESSAGE("WPA/2 PSK");
                break;
            }
            case WDRV_PIC32MZW_AUTH_TYPE_WPA2_PERSONAL:
            {
                SYS_CONSOLE_MESSAGE("WPA2 PSK ");
                break;
            }
            default:
            {
                SYS_CONSOLE_MESSAGE("Not Avail");
                break;
            }
        }
        SYS_CONSOLE_PRINT(" %02d %02X:%02X:%02X:%02X:%02X:%02X %s\r\n", pBSSInfo->ctx.channel,
                pBSSInfo->ctx.bssid.addr[0], pBSSInfo->ctx.bssid.addr[1], pBSSInfo->ctx.bssid.addr[2],
                pBSSInfo->ctx.bssid.addr[3], pBSSInfo->ctx.bssid.addr[4], pBSSInfo->ctx.bssid.addr[5],
                pBSSInfo->ctx.ssid.name);
    }
}

<#if SYS_WIFI_STA_ENABLE == true>
<#if (tcpipDhcp.TCPIP_STACK_USE_DHCP_CLIENT)?has_content && (tcpipDhcp.TCPIP_STACK_USE_DHCP_CLIENT) == true>   
static void SYS_WIFI_TCPIP_DHCP_EventHandler(TCPIP_NET_HANDLE hNet, TCPIP_DHCP_EVENT_TYPE evType, const void* param) 
{
    IPV4_ADDR IPAddr;
    IPV4_ADDR GateWayAddr;
<#if SYS_WIFI_PROVISION_ENABLE == true>
    bool prov_connectstatus = false;
</#if>            
    switch (evType) 
    {
        case DHCP_EVENT_BOUND:
        {
            IPAddr.Val = TCPIP_STACK_NetAddress(hNet);
            if (IPAddr.Val) {
                GateWayAddr.Val = TCPIP_STACK_NetAddressGateway(hNet);
                SYS_CONSOLE_PRINT("IP address obtained = %d.%d.%d.%d \r\n",
                        IPAddr.v[0], IPAddr.v[1], IPAddr.v[2], IPAddr.v[3]);
                SYS_CONSOLE_PRINT("Gateway IP address = %d.%d.%d.%d \r\n",
                        GateWayAddr.v[0], GateWayAddr.v[1], GateWayAddr.v[2], GateWayAddr.v[3]);
                SYS_WIFI_CallBackFun(SYS_WIFI_CONNECT, &IPAddr, g_wifiSrvcCookie);
<#if SYS_WIFI_PROVISION_ENABLE == true>
            prov_connectstatus = true;
            SYS_WIFIPROV_CtrlMsg(g_wifiSrvcProvObj,SYS_WIFIPROV_CONNECT,&prov_connectstatus,sizeof(bool));
</#if>
            }
            break;
        }
        case DHCP_EVENT_CONN_ESTABLISHED:
        {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
            SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CONNECT, "connection to the DHCP server established \r\n");
</#if>
            break;
        }
        case DHCP_EVENT_CONN_LOST:
        {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
            SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CONNECT, "connection to the DHCP server lost \r\n");
</#if>
            SYS_WIFI_CallBackFun(SYS_WIFI_DISCONNECT,NULL,g_wifiSrvcCookie);
<#if SYS_WIFI_PROVISION_ENABLE == true>
            prov_connectstatus = false;
            SYS_WIFIPROV_CtrlMsg(g_wifiSrvcProvObj,SYS_WIFIPROV_CONNECT,&prov_connectstatus,sizeof(bool));
</#if>
            break;
        }
        default:
        {
            break;
        }
    }
}
</#if>
</#if>

static SYS_WIFI_RESULT SYS_WIFI_SetScan(uint8_t channel, bool active) 
{
    uint8_t ret = SYS_WIFI_FAILURE;
    if (WDRV_PIC32MZW_STATUS_OK == WDRV_PIC32MZW_BSSFindFirst(g_wifiSrvcObj.wdrvHandle, channel, active, (WDRV_PIC32MZW_BSSFIND_NOTIFY_CALLBACK) SYS_WIFI_ScanHandler)) 
    {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
        SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, " Wi-Fi Scan request is successful with channel number:%d and scan type(active=true,passive=false):%d\r\n",channel,active);
</#if>
        ret = SYS_WIFI_SUCCESS ;
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
    } 
    else
    {
        SYS_APPDEBUG_ERR_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, " Wi-Fi Scan request is unsuccessful with channel number:%d and scan type(active=true,passive=false):%d\r\n",channel,active);
</#if>
    }
    return ret;
}

static SYS_WIFI_RESULT SYS_WIFI_SetChannel() 
{
    uint8_t ret = SYS_WIFI_FAILURE;
    uint8_t channel = SYS_WIFI_GetChannel();

    if (WDRV_PIC32MZW_STATUS_OK == WDRV_PIC32MZW_BSSCtxSetChannel(&g_wifiSrvcObj.bssCtx, channel)) 
    {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
        SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, " Wi-Fi channel request is successful with channel number:%d \r\n",channel);
</#if>
        ret = SYS_WIFI_SUCCESS;
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
    } 
    else
    {
        SYS_APPDEBUG_ERR_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, " Wi-Fi channel request is unsuccessful with channel number:%d \r\n",channel);
</#if>
    }
    return ret;
}

static uint8_t SYS_WIFI_DisConnect() 
{
    uint8_t ret = SYS_WIFI_FAILURE;

    if (WDRV_PIC32MZW_STATUS_OK == WDRV_PIC32MZW_BSSDisconnect(g_wifiSrvcObj.wdrvHandle)) 
    {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
        SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CONNECT, " Wi-Fi disconnect request is successful \r\n");
</#if>
        ret = SYS_WIFI_SUCCESS;
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
    }
    else
    {
        SYS_APPDEBUG_ERR_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CONNECT, " Wi-Fi channel request is unsuccessful \r\n");
</#if>
    }
    return ret;
}

static SYS_WIFI_RESULT SYS_WIFI_ConnectReq() 
{
    SYS_WIFI_RESULT ret = SYS_WIFI_CONNECT_FAILURE;
    
<#if (SYS_WIFI_STA_ENABLE == true) && (SYS_WIFI_AP_ENABLE == true)>
    SYS_WIFI_MODE devicemode = SYS_WIFI_GetMode();
    if (SYS_WIFI_STA == devicemode) 
    {
        if (WDRV_PIC32MZW_STATUS_OK == WDRV_PIC32MZW_BSSConnect(g_wifiSrvcObj.wdrvHandle, &g_wifiSrvcObj.bssCtx, &g_wifiSrvcObj.authCtx, SYS_WIFI_STAConnCallBack)) 
        {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
            SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CONNECT, "Wi-Fi Driver STA connect request is successful\r\n");
</#if>
            ret = SYS_WIFI_SUCCESS;
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
        }
        else
        {
            SYS_APPDEBUG_ERR_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CONNECT, " Wi-Fi Driver STA connect request is unsuccessful \r\n");
</#if>
        }
    } 
    else if (SYS_WIFI_AP == devicemode) 
    {
        if (WDRV_PIC32MZW_STATUS_OK == WDRV_PIC32MZW_APStart(g_wifiSrvcObj.wdrvHandle, &g_wifiSrvcObj.bssCtx, &g_wifiSrvcObj.authCtx, SYS_WIFI_APConnCallBack)) 
        {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
            SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CONNECT, "Wi-Fi Driver AP start request is successful\r\n");
</#if>
            ret = SYS_WIFI_SUCCESS;        
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
        } 
        else 
        {
            SYS_APPDEBUG_ERR_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CONNECT, "Wi-Fi Driver AP start request is unsuccessful \r\n");
</#if>
        }
    }
<#elseif SYS_WIFI_STA_ENABLE == true>
    if (WDRV_PIC32MZW_STATUS_OK == WDRV_PIC32MZW_BSSConnect(g_wifiSrvcObj.wdrvHandle, &g_wifiSrvcObj.bssCtx, &g_wifiSrvcObj.authCtx, SYS_WIFI_STAConnCallBack)) 
    {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
        SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CONNECT, "Wi-Fi Driver STA connect request is successful\r\n");
</#if>
        ret = SYS_WIFI_SUCCESS ;
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
    } 
    else 
    {
        SYS_APPDEBUG_ERR_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CONNECT, " Wi-Fi Driver STA connect request is unsuccessful \r\n");
</#if>
    }
<#elseif SYS_WIFI_AP_ENABLE == true>
    if (WDRV_PIC32MZW_STATUS_OK == WDRV_PIC32MZW_APStart(g_wifiSrvcObj.wdrvHandle, &g_wifiSrvcObj.bssCtx, &g_wifiSrvcObj.authCtx, SYS_WIFI_APConnCallBack)) 
    {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
        SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CONNECT, "Wi-Fi Driver AP start request is successful\r\n");
</#if>
        ret = SYS_WIFI_SUCCESS ;
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
    } 
    else 
    {
        SYS_APPDEBUG_ERR_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CONNECT, "Wi-Fi Driver AP start request is unsuccessful \r\n");
</#if>
    }
</#if>
    
    if(SYS_WIFI_CONNECT_FAILURE == ret) 
    {
       SYS_WIFI_SetTaskstatus(SYS_WIFI_STATUS_CONNECT_ERROR);	
    }
    return ret;
}

static SYS_WIFI_RESULT SYS_WIFI_SetSSID() 
{
    SYS_WIFI_RESULT ret = SYS_WIFI_CONFIG_FAILURE;
    uint8_t * SSID = SYS_WIFI_GetSSID();
    uint8_t ssid_lenth = SYS_WIFI_GetSSIDLen();

    if (WDRV_PIC32MZW_STATUS_OK == WDRV_PIC32MZW_BSSCtxSetSSID(&g_wifiSrvcObj.bssCtx, SSID, ssid_lenth)) 
    {
<#if SYS_WIFI_AP_ENABLE == true>
        if (SYS_WIFI_AP == SYS_WIFI_GetMode()) 
        {
            WDRV_PIC32MZW_BSSCtxSetSSIDVisibility(&g_wifiSrvcObj.bssCtx, SYS_WIFI_GetSSIDVisibility());
        }
</#if>
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
        SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, "Wi-Fi Driver SSID request is successful\r\n");
</#if>        
        ret = SYS_WIFI_SUCCESS;
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
    } 
    else 
    {
        SYS_APPDEBUG_ERR_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, "Wi-Fi Driver SSID request is unsuccessful \r\n");
</#if>
    }
    return ret;
}

static SYS_WIFI_RESULT SYS_WIFI_ConfigReq() 
{
    SYS_WIFI_RESULT ret = SYS_WIFI_SUCCESS;
    uint8_t authtype = SYS_WIFI_GetAuthType();
    uint8_t * const Password = SYS_WIFI_GetPsk();
    uint8_t pwd_len = SYS_WIFI_GetPskLen();

<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
        SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, "authtype=%d,password=%s,password len=%d\r\n",authtype,Password,pwd_len);
</#if>
    if (SYS_WIFI_SUCCESS == SYS_WIFI_SetSSID()) 
    {
        switch (authtype) 
        {
            case SYS_WIFI_OPEN:
            {
                if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_AuthCtxSetOpen(&g_wifiSrvcObj.authCtx)) 
                {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
                    SYS_APPDEBUG_ERR_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, " Unable to set Authentication to Open \r\n");
</#if>
                    ret = SYS_WIFI_CONFIG_FAILURE;
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
                } 
                else 
                {
                    SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, "set Authentication to Open \r\n");
</#if>
                }
                break;
            }
            case SYS_WIFI_WPA2:
            {                
                if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_AuthCtxSetPersonal(&g_wifiSrvcObj.authCtx, Password, pwd_len, WDRV_PIC32MZW_AUTH_TYPE_WPA2_PERSONAL)) 
                {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
                    SYS_APPDEBUG_ERR_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, "Unable to set authentication to WPA2 PSK\r\n");
</#if>
                    ret = SYS_WIFI_CONFIG_FAILURE;
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
                } 
                else 
                {
                    SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, "set authentication to WPA2 PSK\r\n");
</#if>
                }
                break;
            }
            case SYS_WIFI_WPAWPA2MIXED:
            {
                if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_AuthCtxSetPersonal(&g_wifiSrvcObj.authCtx, Password, pwd_len, WDRV_PIC32MZW_AUTH_TYPE_WPAWPA2_PERSONAL)) 
                {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
                    SYS_APPDEBUG_ERR_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, "Unable to set authentication to WPAWPA2 MIXED PSK\r\n");
</#if>
                    ret = SYS_WIFI_CONFIG_FAILURE;
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
                } 
                else
                {
                    SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, "set authentication to WPAWPA2 MIXED PSK\r\n");
</#if>
                }
                break;
            }
<#if (drvWifiPic32mzw1.DRV_WIFI_PIC32MZW1_SUPPORT_WPA3)?has_content && (drvWifiPic32mzw1.DRV_WIFI_PIC32MZW1_SUPPORT_WPA3) == true>
            case SYS_WIFI_WPA2WPA3MIXED:
            {
                if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_AuthCtxSetPersonal(&g_wifiSrvcObj.authCtx, Password, pwd_len, WDRV_PIC32MZW_AUTH_TYPE_WPA2WPA3_PERSONAL)) 
                {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
                    SYS_APPDEBUG_ERR_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, "Unable to set authentication to WPA2WPA3 MIXED \r\n");
</#if>
                    ret = SYS_WIFI_CONFIG_FAILURE;
                }
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
                else
                {
                    SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, "set authentication to WPA2WPA3 MIXED \r\n");
                }
</#if>
                break;
            }
            case SYS_WIFI_WPA3:
            {
                if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_AuthCtxSetPersonal(&g_wifiSrvcObj.authCtx, Password, pwd_len, WDRV_PIC32MZW_AUTH_TYPE_WPA3_PERSONAL)) 
                {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
                    SYS_APPDEBUG_ERR_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, "Unable to set authentication to WPA3 PSK \r\n");
</#if>
                    ret = SYS_WIFI_CONFIG_FAILURE;
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
                } 
                else 
                {
                    SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, "set authentication to WPA3 PSK \r\n");
</#if>
                }
                break;
            }
</#if>
            case SYS_WIFI_WEP:
            {
               ret = SYS_WIFI_CONFIG_FAILURE; 
              //Wi-Fi service doesn't support WEP
                break;
            }
            default:
            {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
                SYS_APPDEBUG_ERR_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, "set valid authentication type\r\n");
</#if>
                ret = SYS_WIFI_CONFIG_FAILURE;
            }
        }
    }

    if (SYS_WIFI_CONFIG_FAILURE == ret) {
        SYS_WIFI_SetTaskstatus(SYS_WIFI_STATUS_CONFIG_ERROR);
    }

    return ret;
}

static SYS_WIFI_RESULT SYS_WIFI_SetConfig(SYS_WIFI_CONFIG *wifi_config, SYS_WIFI_STATUS status) 
{
<#if SYS_WIFI_PROVISION_ENABLE == true>
    if(true == SYS_WIFI_GetSaveConfig()) 
    {
        return SYS_WIFIPROV_CtrlMsg(g_wifiSrvcProvObj,SYS_WIFIPROV_SETCONFIG,wifi_config,sizeof(SYS_WIFI_CONFIG));
    } 
    else 
    {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
        SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, "Save configuration is disabled\r\n");
</#if>
        SYS_WIFI_RESULT ret = SYS_WIFI_SUCCESS;
        memcpy(&g_wifiSrvcConfig,wifi_config,sizeof(SYS_WIFI_CONFIG));
        SYS_WIFI_SetTaskstatus(status);
        return ret;
    }
<#else>
    SYS_WIFI_RESULT ret = SYS_WIFI_SUCCESS;
    memcpy(&g_wifiSrvcConfig,wifi_config,sizeof(SYS_WIFI_CONFIG));
    SYS_WIFI_SetTaskstatus(status);
    return ret;	
</#if>
}

static uint32_t SYS_WIFI_ExecuteBlock(SYS_MODULE_OBJ object) 
{
    SYS_STATUS tcpipStat;
    static TCPIP_NET_HANDLE netH;
    SYS_WIFI_OBJ *Obj = (SYS_WIFI_OBJ *) object;
<#if SYS_WIFI_AP_ENABLE == true>
    IPV4_ADDR    dwLastIP = {-1};
    IPV4_ADDR           ipAddr;
</#if>
 
    switch (Obj->status) 
    {
        case SYS_WIFI_STATUS_INIT:
        {
            if (OSAL_RESULT_TRUE == OSAL_SEM_Pend(&g_wifiSrvcSemaphore, OSAL_WAIT_FOREVER)) 
            {
                if (true == g_wifiSrvcInit) 
                {
                    if (SYS_STATUS_READY == WDRV_PIC32MZW_Status(sysObj.drvWifiPIC32MZW1)) 
                    {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
                        SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, "Wi-Fi Driver is ready\r\n");
</#if>
                        Obj->status = SYS_WIFI_STATUS_WDRV_OPEN_REQ;                
                    }
                }
                OSAL_SEM_Post(&g_wifiSrvcSemaphore);
            }
            break;
        }
        case SYS_WIFI_STATUS_WDRV_OPEN_REQ:
        {
            if (OSAL_RESULT_TRUE == OSAL_SEM_Pend(&g_wifiSrvcSemaphore, OSAL_WAIT_FOREVER)) 
            {
                Obj->wdrvHandle = WDRV_PIC32MZW_Open(0, 0);
                if (DRV_HANDLE_INVALID != Obj->wdrvHandle) 
                {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
                    SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, "Wi-Fi Driver open successfully\r\n");
</#if>
                    Obj->status = SYS_WIFI_STATUS_AUTOCONNECT_WAIT;
                }
                OSAL_SEM_Post(&g_wifiSrvcSemaphore);
            }
            break;
        }
        case SYS_WIFI_STATUS_AUTOCONNECT_WAIT:
        {
            if (OSAL_RESULT_TRUE == OSAL_SEM_Pend(&g_wifiSrvcSemaphore, OSAL_WAIT_FOREVER)) 
            {
                if (true == SYS_WIFI_GetAutoConnect()) 
                {
                    if (WDRV_PIC32MZW_STATUS_OK == WDRV_PIC32MZW_RegDomainSet(Obj->wdrvHandle, SYS_WIFI_GetCountryCode(), SYS_WIFI_SetRegDomainCallback)) 
                    {
                        SYS_WIFI_PrintConfig();
                        Obj->status = SYS_WIFI_STATUS_TCPIP_WAIT_FOR_TCPIP_INIT;
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
                    } 
                    else
                    {
                        SYS_APPDEBUG_ERR_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, "Wi-Fi driver Regulatory domain set failed\r\n");
</#if>
                    }
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
                } 
                else
                {
                    SYS_APPDEBUG_DBG_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, "Auto connect is disabled\r\n");
</#if>
                }
                OSAL_SEM_Post(&g_wifiSrvcSemaphore);
            }
            break;
        }
        case SYS_WIFI_STATUS_TCPIP_WAIT_FOR_TCPIP_INIT:
        {
            if (OSAL_RESULT_TRUE == OSAL_SEM_Pend(&g_wifiSrvcSemaphore, OSAL_WAIT_FOREVER)) 
            {
                tcpipStat = TCPIP_STACK_Status(sysObj.tcpip);
                if (tcpipStat < 0) 
                {
                    SYS_CONSOLE_MESSAGE("  TCP/IP stack initialization failed!\r\n");
                    Obj->status = SYS_WIFI_STATUS_TCPIP_ERROR;
                } 
                else if (tcpipStat == SYS_STATUS_READY) 
                {
                    netH = TCPIP_STACK_NetHandleGet("PIC32MZW1");
<#if (SYS_WIFI_STA_ENABLE == true) && (SYS_WIFI_AP_ENABLE == true)>                  
<#if (tcpipDhcps.TCPIP_STACK_USE_DHCP_SERVER)?has_content && (tcpipDhcps.TCPIP_STACK_USE_DHCP_SERVER) == true>
<#if (tcpipDhcp.TCPIP_STACK_USE_DHCP_CLIENT)?has_content && (tcpipDhcp.TCPIP_STACK_USE_DHCP_CLIENT) == true>                
                    if (SYS_WIFI_STA == SYS_WIFI_GetMode()) 
                    {
                        if (true == TCPIP_DHCPS_IsEnabled(netH)) 
                        {
                            TCPIP_DHCPS_Disable(netH);
                        }
                        if ((true == TCPIP_DHCP_Enable(netH)) &&
                            (TCPIP_STACK_ADDRESS_SERVICE_DHCPC == TCPIP_STACK_AddressServiceSelect(_TCPIPStackHandleToNet(netH), TCPIP_NETWORK_CONFIG_DHCP_CLIENT_ON))) 
                        {
                                g_wifiSrvcDhcpHdl = TCPIP_DHCP_HandlerRegister(netH, SYS_WIFI_TCPIP_DHCP_EventHandler, NULL);
                        }
                    } 
                    else if (SYS_WIFI_AP == SYS_WIFI_GetMode()) 
                    {
                        if (true == TCPIP_DHCP_IsEnabled(netH)) 
                        {
                            TCPIP_DHCP_Disable(netH);
                        }
                        TCPIP_DHCPS_Enable(netH); //Enable DHCP Server in AP mode
                    }
</#if>
</#if>
<#elseif SYS_WIFI_STA_ENABLE == true>
<#if (tcpipDhcps.TCPIP_STACK_USE_DHCP_SERVER)?has_content && (tcpipDhcps.TCPIP_STACK_USE_DHCP_SERVER) == true>
                    if (true == TCPIP_DHCPS_IsEnabled(netH)) 
                    {
                        TCPIP_DHCPS_Disable(netH);
                    }
</#if>
                    if ((true == TCPIP_DHCP_Enable(netH)) && 
                        (TCPIP_STACK_ADDRESS_SERVICE_DHCPC == TCPIP_STACK_AddressServiceSelect(_TCPIPStackHandleToNet(netH), TCPIP_NETWORK_CONFIG_DHCP_CLIENT_ON))) 
                    {
                        g_wifiSrvcDhcpHdl= TCPIP_DHCP_HandlerRegister (netH, SYS_WIFI_TCPIP_DHCP_EventHandler, NULL);
                    }
<#elseif SYS_WIFI_AP_ENABLE == true>
<#if (tcpipDhcp.TCPIP_STACK_USE_DHCP_CLIENT)?has_content && (tcpipDhcp.TCPIP_STACK_USE_DHCP_CLIENT) == true>
                    if (true == TCPIP_DHCP_IsEnabled(netH)) 
                    {
                        TCPIP_DHCP_Disable(netH);
                    }
</#if>
                    TCPIP_DHCPS_Enable(netH);                                         //Enable DHCP Server in AP mode
</#if>
                }                
                Obj->status = SYS_WIFI_STATUS_CONNECT_REQ;
                OSAL_SEM_Post(&g_wifiSrvcSemaphore);
            }
            break;
        }
        case SYS_WIFI_STATUS_CONNECT_REQ:
        {
            if (OSAL_RESULT_TRUE == OSAL_SEM_Pend(&g_wifiSrvcSemaphore, OSAL_WAIT_FOREVER)) 
            {
                if (SYS_WIFI_SUCCESS == SYS_WIFI_SetChannel()) 
                {
                    if (SYS_WIFI_SUCCESS == SYS_WIFI_ConfigReq()) 
                    {
                        if (SYS_WIFI_SUCCESS == SYS_WIFI_ConnectReq()) 
                        {
<#if (SYS_WIFI_STA_ENABLE == true) && (SYS_WIFI_AP_ENABLE == true)>	
                            Obj->status = (SYS_WIFI_STA == SYS_WIFI_GetMode()) ? SYS_WIFI_STATUS_TCPIP_READY : SYS_WIFI_STATUS_WAIT_FOR_AP_IP;
<#elseif SYS_WIFI_STA_ENABLE == true>
                            Obj->status = SYS_WIFI_STATUS_TCPIP_READY;
<#elseif SYS_WIFI_AP_ENABLE == true>
                            Obj->status = SYS_WIFI_STATUS_WAIT_FOR_AP_IP;
</#if>
                        }
                    }
                }
                OSAL_SEM_Post(&g_wifiSrvcSemaphore);
			}
            break;
        }
<#if SYS_WIFI_AP_ENABLE == true>
        case SYS_WIFI_STATUS_WAIT_FOR_AP_IP:
        {
            if (OSAL_RESULT_TRUE == OSAL_SEM_Pend(&g_wifiSrvcSemaphore, OSAL_WAIT_FOREVER)) 
            {
                ipAddr.Val = TCPIP_STACK_NetAddress(netH);
                if (dwLastIP.Val != ipAddr.Val) 
                {
                    dwLastIP.Val = ipAddr.Val;
                    SYS_CONSOLE_MESSAGE(TCPIP_STACK_NetNameGet(netH));
                    SYS_CONSOLE_MESSAGE(" AP Mode IP Address: ");
                    SYS_CONSOLE_PRINT("%d.%d.%d.%d \r\n", ipAddr.v[0], ipAddr.v[1], ipAddr.v[2], ipAddr.v[3]);
                    Obj->status = SYS_WIFI_STATUS_TCPIP_READY;
                    OSAL_SEM_Post(&g_wifiSrvcSemaphore);
<#if SYS_WIFI_PROVISION_ENABLE == true>
                    bool prov_connectstatus = true;
                    SYS_WIFIPROV_CtrlMsg(g_wifiSrvcProvObj, SYS_WIFIPROV_CONNECT, &prov_connectstatus, sizeof (bool));
</#if>
                }
            }
            break;
        }
        case SYS_WIFI_STATUS_WAIT_FOR_STA_IP:
        {
            TCPIP_DHCPS_LEASE_HANDLE dhcpsLease = 0;
            TCPIP_DHCPS_LEASE_ENTRY dhcpsLeaseEntry;
            
            OSAL_SEM_Pend(&g_wifiSrvcSemaphore, OSAL_WAIT_FOREVER); 
            if ((true == g_wifiSrvcApConnectStatus) && (true == g_wifiSrvcApBssId.valid)) 
            {
                dhcpsLease = TCPIP_DHCPS_LeaseEntryGet(netH, &dhcpsLeaseEntry, dhcpsLease);
                if ((0 != dhcpsLease) && (0 == memcmp(&dhcpsLeaseEntry.hwAdd, g_wifiSrvcApBssId.addr, WDRV_PIC32MZW_MAC_ADDR_LEN))) 
                {
                    SYS_CONSOLE_PRINT("\r\n Connected STA IP:%d.%d.%d.%d \r\n", dhcpsLeaseEntry.ipAddress.v[0], dhcpsLeaseEntry.ipAddress.v[1], dhcpsLeaseEntry.ipAddress.v[2], dhcpsLeaseEntry.ipAddress.v[3]);
                    ipAddr.Val = dhcpsLeaseEntry.ipAddress.Val;
                    Obj->status = SYS_WIFI_STATUS_TCPIP_READY;
                    OSAL_SEM_Post(&g_wifiSrvcSemaphore);
                    SYS_WIFI_CallBackFun(SYS_WIFI_CONNECT, &ipAddr, g_wifiSrvcCookie);
                }
            }            
            break;
        }
</#if>
        case SYS_WIFI_STATUS_TCPIP_READY:
        {
            break;
        }
        case SYS_WIFI_STATUS_TCPIP_ERROR:
        {
            SYS_STATUS tcpipStat;
            tcpipStat = TCPIP_STACK_Status(sysObj.tcpip);
            
            if (tcpipStat < 2) 
            {
                Obj->status = SYS_WIFI_STATUS_TCPIP_ERROR;
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
                SYS_APPDEBUG_ERR_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, "TCP/IP Stack is not in ready state\r\n");
</#if>
            } 
            else 
            {
                Obj->status = SYS_WIFI_STATUS_TCPIP_WAIT_FOR_TCPIP_INIT;
            }
            break;
        }
        default:
        {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
            SYS_APPDEBUG_ERR_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG, " Wi-Fi service is not in invalid state\r\n");
</#if>
            break;
        }
    }
<#if SYS_WIFI_PROVISION_ENABLE == true>
    SYS_WIFIPROV_Tasks (g_wifiSrvcProvObj);
</#if>
    return Obj->status;
}

<#if SYS_WIFI_PROVISION_ENABLE == true>
static void SYS_WIFI_WIFIPROVCallBack(uint32_t event, void * data, void *cookie) 
{
    char *cookie_l = (void *) cookie;
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
    SYS_APPDEBUG_FN_ENTER_PRINT(g_wifiSrvcAppDebugHdl, WIFI_PROVISIONING);
</#if>       

    if (g_wifiSrvcProvCookieVal == *cookie_l) 
    {
        switch (event) 
        {
            case SYS_WIFIPROV_SETCONFIG:
            {
                SYS_WIFIPROV_CONFIG *wificonfig = (SYS_WIFIPROV_CONFIG*) data;
                if ( (wificonfig) && (false == g_wifiSrvcInit)) 
                {
                    g_wifiSrvcInit = true;
                    memcpy(&g_wifiSrvcConfig, wificonfig, sizeof (SYS_WIFIPROV_CONFIG));
                }
                else 
                {
<#if (SYS_WIFI_STA_ENABLE == true) && (SYS_WIFI_AP_ENABLE == true)>	
                    if ((SYS_WIFIPROV_STA == (SYS_WIFIPROV_MODE) SYS_WIFI_GetMode()) && (SYS_WIFIPROV_STA == wificonfig->mode)) 
                    {
                        memcpy(&g_wifiSrvcConfig, wificonfig, sizeof (SYS_WIFIPROV_CONFIG));
                        if (g_wifiSrvcDrvAssocHdl == WDRV_PIC32MZW_ASSOC_HANDLE_INVALID) 
                        {
                            if ((true == SYS_WIFI_GetAutoConnect()) && (g_wifiSrvcAutoConnectRetry == MAX_AUTO_CONNECT_RETRY)) 
                            {
                                SYS_WIFI_SetTaskstatus(SYS_WIFI_STATUS_CONNECT_REQ);
                                g_wifiSrvcAutoConnectRetry = 0;
                            }
                        } 
                        else 
                        {
                            SYS_WIFI_DisConnect();
                        }
                    } 
                    else 
                    {
                        SYS_CONSOLE_MESSAGE("######################################Rebooting the Device ###############################\r\n");
                        Soft_Reset();
                    }
                    if(data) 
                    {
                        SYS_WIFI_CallBackFun(SYS_WIFI_PROVCONFIG, data, g_wifiSrvcCookie);
                    }
                }                
<#elseif SYS_WIFI_STA_ENABLE == true>
                    if((SYS_WIFIPROV_STA == (SYS_WIFIPROV_MODE)SYS_WIFI_GetMode()) && (SYS_WIFIPROV_STA == wificonfig->mode))
                    {
                        memcpy(&g_wifiSrvcConfig,wificonfig,sizeof(SYS_WIFIPROV_CONFIG));
                        if(g_wifiSrvcDrvAssocHdl == WDRV_PIC32MZW_ASSOC_HANDLE_INVALID)
                        {
                            if((true == SYS_WIFI_GetAutoConnect()) && (g_wifiSrvcAutoConnectRetry == MAX_AUTO_CONNECT_RETRY))
                            {
                                SYS_WIFI_SetTaskstatus(SYS_WIFI_STATUS_CONNECT_REQ);
                                g_wifiSrvcAutoConnectRetry = 0;
                            }
                        } 
                        else 
                        {
                            SYS_WIFI_DisConnect();
                        }
                    }
                    if(data) 
                    {
                        SYS_WIFI_CallBackFun(SYS_WIFI_PROVCONFIG,data,g_wifiSrvcCookie);
                    }
                }
<#elseif SYS_WIFI_AP_ENABLE == true>
                        SYS_CONSOLE_MESSAGE("######################################Rebooting the Device ###############################\r\n"); 
                        Soft_Reset();
            }
                if(data) 
                {
                    SYS_WIFI_CallBackFun(SYS_WIFI_PROVCONFIG,data,g_wifiSrvcCookie);		
                }
</#if>
                break;
            }
            case SYS_WIFIPROV_GETCONFIG:
            {
                if(data) 
                {
                    SYS_WIFI_CallBackFun(SYS_WIFI_GETCONFIG, data, g_wifiSrvcCookie);
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
    SYS_APPDEBUG_FN_EXIT_PRINT(g_wifiSrvcAppDebugHdl, WIFI_PROVISIONING);
</#if>
}
</#if>

// *****************************************************************************
// *****************************************************************************
// Section:  SYS WIFI Interface Functions
// *****************************************************************************
// *****************************************************************************
SYS_MODULE_OBJ SYS_WIFI_Initialize(SYS_WIFI_CONFIG *config, SYS_WIFI_CALLBACK callback, void *cookie) 
{

<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
    g_wifiSrvcAppDebugHdl.logLevel = (APP_LOG_ERROR_LVL & SYS_WIFI_APPDEBUG_ERR_LEVEL_ENABLE) | (APP_LOG_DBG_LVL & SYS_WIFI_APPDEBUG_DBG_LEVEL_ENABLE) | 
                                (APP_LOG_INFO_LVL & SYS_WIFI_APPDEBUG_INFO_LEVEL_ENABLE) | (APP_LOG_FN_EE_LVL & SYS_WIFI_APPDEBUG_FUNC_LEVEL_ENABLE);
        g_wifiSrvcAppDebugHdl.logFlow =  (WIFI_CFG & SYS_WIFI_APPDEBUG_CFG_FLOW) | 
                                (WIFI_CONNECT & SYS_WIFI_APPDEBUG_CONNECT_FLOW)| 
                                (WIFI_PROVISIONING & SYS_WIFI_APPDEBUG_PROVISIONING_FLOW) |
                                (WIFI_PROVISIONINGCMD & SYS_WIFI_APPDEBUG_PROVISIONINGCMD_FLOW) | 
                                (WIFI_PROVISIONINGSOCKET & SYS_WIFI_APPDEBUG_PROVISIONINGSOCK_FLOW);
    g_wifiSrvcAppDebugHdl.prefixString = SYS_WIFI_DEBUG_PRESTR;
    g_wifiSrvcAppDebugHdl = SYS_APPDEBUG_Open(&g_wifiSrvcAppDebugHdl);
    SYS_APPDEBUG_FN_ENTER_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG);
</#if>
    if (SYS_WIFI_STATUS_NONE == SYS_WIFI_GetTaskstatus()) 
    {
        if (OSAL_SEM_Create(&g_wifiSrvcSemaphore, OSAL_SEM_TYPE_BINARY, 1, 1) != OSAL_RESULT_TRUE) 
        {
            SYS_CONSOLE_MESSAGE("Failed to Initialize Wi-Fi Service as Semaphore NOT created\r\n");
            return SYS_MODULE_OBJ_INVALID;
        }
        if (callback != NULL) 
        {
            SYS_WIFI_REGCB(callback);
        }
        WDRV_PIC32MZW_BSSCtxSetDefaults(&g_wifiSrvcObj.bssCtx);
        WDRV_PIC32MZW_AuthCtxSetDefaults(&g_wifiSrvcObj.authCtx);
        SYS_WIFI_SetTaskstatus(SYS_WIFI_STATUS_INIT);
        SYS_WIFI_SetCookie(cookie);
<#if SYS_WIFI_PROVISION_ENABLE == false>
        SYS_WIFI_InitConfig(config);
        g_wifiSrvcInit = true;
<#else>
        g_wifiSrvcProvObj= SYS_WIFIPROV_Initialize ((SYS_WIFIPROV_CONFIG *)config,SYS_WIFI_WIFIPROVCallBack,&g_wifiSrvcProvCookieVal);
</#if>
        return (SYS_MODULE_OBJ) &g_wifiSrvcObj;
    }
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
    SYS_APPDEBUG_FN_EXIT_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG);
</#if>
    return SYS_MODULE_OBJ_INVALID;
}

SYS_WIFI_RESULT SYS_WIFI_Deinitialize(SYS_MODULE_OBJ object) 
{
    uint32_t ret = SYS_WIFI_OBJ_INVALID;
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
    SYS_APPDEBUG_FN_ENTER_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG);
</#if>
    
    if (&g_wifiSrvcObj == (SYS_WIFI_OBJ *) object) 
    {
<#if (SYS_WIFI_STA_ENABLE == true) && (SYS_WIFI_AP_ENABLE == true)>
        if (SYS_WIFI_STA == SYS_WIFI_GetMode()) 
        {
            SYS_WIFI_DisConnect();
            TCPIP_DHCP_HandlerDeRegister(g_wifiSrvcDhcpHdl);
        } 
        else if (SYS_WIFI_AP == SYS_WIFI_GetMode()) 
        {
            if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_APStop(g_wifiSrvcObj.wdrvHandle)) 
            {
                SYS_CONSOLE_MESSAGE(" AP mode Stop Failed \n");
            }
        }
<#elseif SYS_WIFI_STA_ENABLE == true>
        SYS_WIFI_DisConnect();
        TCPIP_DHCP_HandlerDeRegister(g_wifiSrvcDhcpHdl);
<#elseif SYS_WIFI_AP_ENABLE == true>
        if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_APStop(g_wifiSrvcObj.wdrvHandle)) 
        {
            SYS_CONSOLE_MESSAGE(" AP mode Stop Failed \n");
        }
</#if>
        WDRV_PIC32MZW_Close(g_wifiSrvcObj.wdrvHandle);
        g_wifiSrvcInit = false;
        memset(&g_wifiSrvcObj,0,sizeof(SYS_WIFI_OBJ));
        memset(g_wifiSrvcCallBack,0,sizeof(g_wifiSrvcCallBack));
        SYS_WIFI_SetTaskstatus(SYS_WIFI_STATUS_NONE);
<#if SYS_WIFI_PROVISION_ENABLE == true>		
        SYS_WIFIPROV_Deinitialize(g_wifiSrvcProvObj);
</#if>
        if (OSAL_SEM_Delete(&g_wifiSrvcSemaphore) != OSAL_RESULT_TRUE) 
        {
            SYS_CONSOLE_MESSAGE("Failed to Delete Wi-Fi Service Semaphore \r\n");
        }
        ret = SYS_WIFI_SUCCESS;
    }
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
    SYS_APPDEBUG_FN_EXIT_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG);
</#if>
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
//SYS_APPDEBUG_Close(g_wifiSrvcAppDebugHdl);
</#if>
    return ret;
}

uint8_t SYS_WIFI_GetStatus(SYS_MODULE_OBJ object) 
{
    uint8_t ret = SYS_WIFI_OBJ_INVALID;
    
    if (OSAL_RESULT_TRUE == OSAL_SEM_Pend(&g_wifiSrvcSemaphore, OSAL_WAIT_FOREVER)) 
    {
        if (&g_wifiSrvcObj == (SYS_WIFI_OBJ *) object) 
        {
            ret = ((SYS_WIFI_OBJ *) object)->status;
        }
        OSAL_SEM_Post(&g_wifiSrvcSemaphore);
    }
    return ret;
}

uint8_t SYS_WIFI_Tasks(SYS_MODULE_OBJ object) 
{
    uint8_t ret = SYS_WIFI_OBJ_INVALID;

    if (&g_wifiSrvcObj == (SYS_WIFI_OBJ *) object) 
    {
        ret = SYS_WIFI_ExecuteBlock(object);
    }
    return ret;
}

SYS_WIFI_RESULT SYS_WIFI_CtrlMsg(SYS_MODULE_OBJ object, uint32_t event, void *buffer, uint32_t length) 
{
    uint8_t ret = SYS_WIFI_OBJ_INVALID;
    uint8_t *channel;
    bool *scan_type;

    if (OSAL_RESULT_TRUE == OSAL_SEM_Pend(&g_wifiSrvcSemaphore, OSAL_WAIT_FOREVER)) 
    {
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
        SYS_APPDEBUG_FN_ENTER_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG);
</#if>    
        if (&g_wifiSrvcObj == (SYS_WIFI_OBJ *)object) 
        {
            switch (event) 
            {
                case SYS_WIFI_CONNECT:
                {
                    //if service is already processing pending request from client then ignore new request
                    if (SYS_WIFI_STATUS_CONNECT_REQ != g_wifiSrvcObj.status) 
                    {
                        if ((buffer) && (length == sizeof (SYS_WIFI_CONFIG)))
                        {
                            ret = SYS_WIFI_SetConfig((SYS_WIFI_CONFIG *) buffer, SYS_WIFI_STATUS_CONNECT_REQ);
                        }
                    } 
                    else
                    {
                        ret = SYS_WIFI_CONNECT_FAILURE;
                    }
                    break;
                }
                case SYS_WIFI_REGCALLBACK:
                {
                    if ((buffer) && (length == 4)) 
                    {
                        ret = SYS_WIFI_REGCB(buffer);
                    }
                    break;
                }
                case SYS_WIFI_DISCONNECT:
                {
                    ret = SYS_WIFI_DisConnect();
                    break;
                }
                case SYS_WIFI_GETCONFIG:
                {
                    if (true == g_wifiSrvcInit) 
                    {
                        if ((buffer) && (length == sizeof (SYS_WIFI_CONFIG))) 
                        {
                            memcpy(buffer, &g_wifiSrvcConfig, sizeof (g_wifiSrvcConfig));
                            ret = SYS_WIFI_SUCCESS;
                        }
                    } 
                    else
                    {
                        ret = SYS_WIFI_SERVICE_UNINITIALIZE;
                    }
                    break;
                }
                case SYS_WIFI_SCANREQ:
                {
                    //if service is already processing pending connection request from client then ignore new request
                    if ((SYS_WIFI_STATUS_CONNECT_REQ != g_wifiSrvcObj.status) && (buffer) && (length == 2)) 
                    {
                        channel = (uint8_t *) buffer;
                        scan_type = (bool *) buffer + 1;
                        ret = SYS_WIFI_SetScan(*channel, *scan_type);
                    }
                    break;
                }
            }
        }
<#if SYS_WIFI_APPDEBUG_ENABLE  == true>
        SYS_APPDEBUG_FN_EXIT_PRINT(g_wifiSrvcAppDebugHdl, WIFI_CFG);
</#if>
        OSAL_SEM_Post(&g_wifiSrvcSemaphore);
    }
    return ret;
}
/* *****************************************************************************
 End of File
 */
