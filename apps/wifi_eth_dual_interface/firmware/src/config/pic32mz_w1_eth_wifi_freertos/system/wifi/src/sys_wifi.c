/*******************************************************************************
  Wi-Fi System Service Implementation.

  Company:
    Microchip Technology Inc.

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
#include "system/wifiprov/sys_wifiprov.h"

// *****************************************************************************
// *****************************************************************************
// Section: Type Definitions
// *****************************************************************************
// *****************************************************************************
typedef struct {
    /* The WiFi service current status */
    SYS_WIFI_STATUS status;    
    DRV_HANDLE wdrvHandle;    
    WDRV_PIC32MZW_AUTH_CONTEXT authCtx;
    WDRV_PIC32MZW_BSS_CONTEXT  bssCtx;
}SYS_WIFI_OBJ;         //Wi-Fi System Service Object


// *****************************************************************************
// *****************************************************************************
// Section: Global Data
// *****************************************************************************
// *****************************************************************************
SYS_WIFI_CALLBACK gWiFiCallBack[SYS_WIFI_MAX_CBS];               	                            // Storing Wi-Fi Service Callbacks 
static SYS_WIFI_OBJ gWiFiObj={SYS_WIFI_STATUS_NONE,NULL};                                       // Wi-Fi Service Object
static WDRV_PIC32MZW_ASSOC_HANDLE drvAssocHandle = WDRV_PIC32MZW_ASSOC_HANDLE_INVALID;          // Wi-Fi Driver ASSOC Handle
static WDRV_PIC32MZW_MAC_ADDR      ap_bssid;						                            // In Ap mode, Connected STA MAC Address 
static bool                        ap_connected;					                            // In Ap mode, Connection status
static SYS_WIFI_CONFIG gWiFiConfig;                         		                            // Wi-Fi  Service Configuration Structure
static void *gCookie ;                                                                          // Wi-Fi Service cookie
static bool InitWiFiSrvc = false;                                                               // Wi-Fi Service init enable wait status
static char cookie_wifiprov = 100;                               	                            // Cookie for Wi-Fi Provision 
static SYS_MODULE_OBJ WiFiProvObject ;                           	                            // Wi-Fi Provision Service Object
static OSAL_SEM_HANDLE_TYPE     g_SyswifiSemaphore;	                                            // Semaphore for Critical Section


static uint8_t SYS_WIFI_DisConnect();
static SYS_WIFI_RESULT SYS_WIFI_ConnectReq();
static SYS_WIFI_RESULT SYS_WIFI_SetScan(uint8_t channel,bool active);
void SYS_WIFI_WIFIPROVCallBack(uint32_t event, void * data,void *cookie );

// *****************************************************************************
// *****************************************************************************
// Section: Local Functions
// *****************************************************************************
// *****************************************************************************
static void Soft_Reset(void) {
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
	RSWRST=RSWRSTbits.SWRST;

    /*Reference code. We will not hit this due to reset. This is here for reference.*/
    int_flag = (bool) __builtin_disable_interrupts();

    SYSKEY = 0x33333333;

    if (int_flag) /* if interrupts originally were enabled, re-enable them */ {
        __builtin_mtc0(12, 0, (__builtin_mfc0(12, 0) | 0x0001));
    }

}
static inline void SYS_WIFI_CallBackFun( uint32_t event, void * data,void *cookie)
{    
     uint8_t Idx ;
    
    for (Idx=0;Idx < SYS_WIFI_MAX_CBS;Idx++)
    {
        if(gWiFiCallBack[Idx])
        {
            (gWiFiCallBack[Idx])(event,data,cookie);
        }
    }	
}
static inline SYS_WIFI_RESULT SYS_WIFI_REGCB(SYS_WIFI_CALLBACK callback)
{
    SYS_WIFI_RESULT ret = SYS_WIFI_FAILURE;
    uint8_t Idx ;
    
    for (Idx=0;Idx < SYS_WIFI_MAX_CBS;Idx++)
    {
        if(!gWiFiCallBack[Idx])
        {
            gWiFiCallBack[Idx] = callback;           
            ret = SYS_WIFI_SUCCESS;            
            break;
        }
    }
    return ret;
}

static inline void SYS_WIFI_SetCookie(void *cookie)
{
      gCookie = cookie;
}

static inline SYS_WIFI_MODE SYS_WIFI_GetMode()
{
    return gWiFiConfig.mode;
}
static inline bool SYS_WIFI_GetSaveConfig()
{
	return (gWiFiConfig.save_config);


}
static inline uint8_t * SYS_WIFI_GetSSID()
{
	return (gWiFiConfig.apconfig.ssid);
}
static inline uint8_t SYS_WIFI_GetSSIDLen()
{
	return strlen((const char *)gWiFiConfig.apconfig.ssid);
}
static inline uint8_t SYS_WIFI_GetChannel()
{
	return (gWiFiConfig.apconfig.channel);
}
static inline uint8_t SYS_WIFI_GetAuthType()
{
	return (gWiFiConfig.apconfig.auth_type);
}
static inline uint8_t *SYS_WIFI_GetPsk()
{
	return (gWiFiConfig.apconfig.psk);
   
}
static inline uint8_t SYS_WIFI_GetPskLen()
{
	return strlen((const char *)gWiFiConfig.apconfig.psk);
   
}
static inline bool SYS_WIFI_GetSSIDVisibility()
{
	return (gWiFiConfig.apconfig.ssid_visibility);
    
}
static inline const char *SYS_WIFI_GetCountryCode()
{
	return (const char *)gWiFiConfig.countrycode;
}
static inline bool SYS_WIFI_GetAutoConnect()
{
	//In AP mode, auto_connect is always enabled
	return true;
}

static inline void SYS_WIFI_SetTaskstatus(SYS_WIFI_STATUS status)
{
    gWiFiObj.status = status;
}
static inline SYS_WIFI_STATUS SYS_WIFI_GetTaskstatus()
{
    return gWiFiObj.status;
}
static inline void SYS_WIFI_PrintConfig()
{
    SYS_CONSOLE_PRINT("\r\n mode=%d (0-STA,1-AP) save_config=%d \r\n ",gWiFiConfig.mode,gWiFiConfig.save_config);

    if(gWiFiConfig.mode == SYS_WIFI_AP)
    SYS_CONSOLE_PRINT("\r\n AP Configuration :\r\n channel=%d \r\n ssid_visibility=%d \r\n ssid=%s \r\n passphase=%s \r\n authentication type=%d (1-Open,2-WEP,3-Mixed mode(WPA/WPA2),4-WPA2) \r\n",gWiFiConfig.apconfig.channel,gWiFiConfig.apconfig.ssid_visibility,gWiFiConfig.apconfig.ssid,gWiFiConfig.apconfig.psk,gWiFiConfig.apconfig.auth_type);

}
static void SYS_WIFI_APConnCallBack(DRV_HANDLE handle, WDRV_PIC32MZW_ASSOC_HANDLE assocHandle, WDRV_PIC32MZW_CONN_STATE currentState)
{
    switch(currentState)
    {
        case WDRV_PIC32MZW_CONN_STATE_CONNECTED:
        {            
            if (WDRV_PIC32MZW_STATUS_OK == WDRV_PIC32MZW_AssocPeerAddressGet(assocHandle, &ap_bssid))
            {  
                ap_connected   = true;
                drvAssocHandle = assocHandle;
                SYS_CONSOLE_PRINT("Connected STA MAC Address=%x:%x:%x:%x:%x:%x\r\n",ap_bssid.addr[0],ap_bssid.addr[1],ap_bssid.addr[2],ap_bssid.addr[3],ap_bssid.addr[4],ap_bssid.addr[5]);
                SYS_WIFI_SetTaskstatus(SYS_WIFI_STATUS_WAIT_FOR_STA_IP);
            }            
            break;
        }
        case WDRV_PIC32MZW_CONN_STATE_DISCONNECTED:
        {
            if (true == ap_connected)
            {
                drvAssocHandle = WDRV_PIC32MZW_ASSOC_HANDLE_INVALID;
                ap_connected   = false;
                SYS_WIFI_CallBackFun(SYS_WIFI_DISCONNECT,NULL,gCookie);
            }
            break;
        }
        default:
		{
			break;
		}
            
    }
        
}

static void SYS_WIFI_SetRegDomainCallback(DRV_HANDLE handle, uint8_t index, uint8_t ofTotal, bool isCurrent, const char *pRegDomain) 
{

    if ((1 != index) || (1 != ofTotal) || (false == isCurrent) || (NULL == pRegDomain)) {
    } else {
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

            //ATCMD_ReportOK(cmdTxt);
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

static SYS_WIFI_RESULT SYS_WIFI_SetScan(uint8_t channel,bool active)
{
	uint8_t ret = SYS_WIFI_FAILURE;
	
	if(WDRV_PIC32MZW_STATUS_OK == WDRV_PIC32MZW_BSSFindFirst(gWiFiObj.wdrvHandle, channel, active, (WDRV_PIC32MZW_BSSFIND_NOTIFY_CALLBACK)SYS_WIFI_ScanHandler))
    {
        ret = SYS_WIFI_SUCCESS ;
    }
	return ret;
}

static SYS_WIFI_RESULT SYS_WIFI_SetChannel()
{
    uint8_t ret = SYS_WIFI_FAILURE;
    uint8_t channel = SYS_WIFI_GetChannel();
    
	
    if(WDRV_PIC32MZW_STATUS_OK == WDRV_PIC32MZW_BSSCtxSetChannel(&gWiFiObj.bssCtx,channel)) 
    {
		ret = SYS_WIFI_SUCCESS ;
    }
    return ret;
}
static uint8_t SYS_WIFI_DisConnect()
{
    uint8_t ret = SYS_WIFI_FAILURE;
    
    if (WDRV_PIC32MZW_STATUS_OK == WDRV_PIC32MZW_BSSDisconnect(gWiFiObj.wdrvHandle))
    {
        ret = SYS_WIFI_SUCCESS ;
    }

    return ret;
    
}
static SYS_WIFI_RESULT SYS_WIFI_ConnectReq()
{
    SYS_WIFI_RESULT ret = SYS_WIFI_CONNECT_FAILURE;

    
	if (WDRV_PIC32MZW_STATUS_OK == WDRV_PIC32MZW_APStart(gWiFiObj.wdrvHandle, &gWiFiObj.bssCtx, &gWiFiObj.authCtx, SYS_WIFI_APConnCallBack))
    {
		ret = SYS_WIFI_SUCCESS ;
    }
	
	if(SYS_WIFI_CONNECT_FAILURE == ret)
       SYS_WIFI_SetTaskstatus(SYS_WIFI_STATUS_CONNECT_ERROR);	

    return ret;
}

static SYS_WIFI_RESULT SYS_WIFI_SetSSID()
{
    SYS_WIFI_RESULT ret = SYS_WIFI_CONFIG_FAILURE;
    uint8_t * SSID = SYS_WIFI_GetSSID();
    uint8_t ssid_lenth = SYS_WIFI_GetSSIDLen();

        
    if (WDRV_PIC32MZW_STATUS_OK == WDRV_PIC32MZW_BSSCtxSetSSID(&gWiFiObj.bssCtx,SSID ,ssid_lenth))
    {
        if(SYS_WIFI_AP == SYS_WIFI_GetMode())
        { 
            WDRV_PIC32MZW_BSSCtxSetSSIDVisibility(&gWiFiObj.bssCtx, SYS_WIFI_GetSSIDVisibility());
        }
        ret = SYS_WIFI_SUCCESS;
    }

    return ret;
}

static SYS_WIFI_RESULT SYS_WIFI_ConfigReq()
{
    SYS_WIFI_RESULT ret = SYS_WIFI_SUCCESS;
    
    uint8_t authtype = SYS_WIFI_GetAuthType();
    uint8_t *const Password = SYS_WIFI_GetPsk();
    uint8_t pwd_len = SYS_WIFI_GetPskLen();


    if (SYS_WIFI_SUCCESS == SYS_WIFI_SetSSID())
    {
        
        switch (authtype)
        {
            case SYS_WIFI_OPEN:
            {
                if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_AuthCtxSetOpen(&gWiFiObj.authCtx))
                {
                    ret = SYS_WIFI_CONFIG_FAILURE;
                }
                break;
            }
            case SYS_WIFI_WPA2:
            {
                
                if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_AuthCtxSetPersonal(&gWiFiObj.authCtx,Password, pwd_len, WDRV_PIC32MZW_AUTH_TYPE_WPA2_PERSONAL))
                {
                    ret = SYS_WIFI_CONFIG_FAILURE;
                }
                break;
            }
            case SYS_WIFI_WPAWPA2MIXED:
            {
                
                if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_AuthCtxSetPersonal(&gWiFiObj.authCtx, Password, pwd_len, WDRV_PIC32MZW_AUTH_TYPE_WPAWPA2_PERSONAL))
                {
                    ret = SYS_WIFI_CONFIG_FAILURE;
                }
                break;
            }
            case SYS_WIFI_WPA2WPA3MIXED:
            {
                
                if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_AuthCtxSetPersonal(&gWiFiObj.authCtx, Password, pwd_len, WDRV_PIC32MZW_AUTH_TYPE_WPA2WPA3_PERSONAL))
                {
                    ret = SYS_WIFI_CONFIG_FAILURE;
                }
                break;
            }
            case SYS_WIFI_WPA3:
            {
                
                if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_AuthCtxSetPersonal(&gWiFiObj.authCtx, Password, pwd_len, WDRV_PIC32MZW_AUTH_TYPE_WPA3_PERSONAL))
                {
                    ret = SYS_WIFI_CONFIG_FAILURE;
                }
                break;
            }
            case SYS_WIFI_WEP:
            {
               ret = SYS_WIFI_CONFIG_FAILURE; 
              //TODO
                break;
            }
            default:
            {
                ret = SYS_WIFI_CONFIG_FAILURE;
            }    
        }
    }
    
    if(SYS_WIFI_CONFIG_FAILURE == ret)
        SYS_WIFI_SetTaskstatus(SYS_WIFI_STATUS_CONFIG_ERROR);
    
    return ret;
}
static SYS_WIFI_RESULT SYS_WIFI_SetConfig(SYS_WIFI_CONFIG *wifi_config,SYS_WIFI_STATUS status)
{
    if(true == SYS_WIFI_GetSaveConfig())
     	return SYS_WIFIPROV_CtrlMsg(WiFiProvObject,SYS_WIFIPROV_SETCONFIG,wifi_config,sizeof(SYS_WIFI_CONFIG));
    else
    {
    	SYS_WIFI_RESULT ret = SYS_WIFI_SUCCESS;
    	memcpy(&gWiFiConfig,wifi_config,sizeof(SYS_WIFI_CONFIG));
    	SYS_WIFI_SetTaskstatus(status);
    	return ret;
    }

}


static uint32_t SYS_WIFI_ExecuteBlock(SYS_MODULE_OBJ object)
{

    SYS_STATUS          tcpipStat;
    static TCPIP_NET_HANDLE    netH;
    SYS_WIFI_OBJ *Obj = (SYS_WIFI_OBJ *)object;
    IPV4_ADDR    dwLastIP = {-1};
    IPV4_ADDR           ipAddr;
 
    switch ( Obj->status )
    {   
        case SYS_WIFI_STATUS_INIT:
        {
            OSAL_SEM_Pend(&g_SyswifiSemaphore, OSAL_WAIT_FOREVER);
            if(true == InitWiFiSrvc)
            {
                if (SYS_STATUS_READY == WDRV_PIC32MZW_Status(sysObj.drvWifiPIC32MZW1))
                {
                    Obj->status = SYS_WIFI_STATUS_WDRV_OPEN_REQ;                
                }
            }
            OSAL_SEM_Post(&g_SyswifiSemaphore);
            break;
        }

        case SYS_WIFI_STATUS_WDRV_OPEN_REQ:
        {
            OSAL_SEM_Pend(&g_SyswifiSemaphore, OSAL_WAIT_FOREVER);
            Obj->wdrvHandle = WDRV_PIC32MZW_Open(0, 0);

            if (DRV_HANDLE_INVALID != Obj->wdrvHandle)
            {
                Obj->status = SYS_WIFI_STATUS_AUTOCONNECT_WAIT;
            }
            OSAL_SEM_Post(&g_SyswifiSemaphore);
            break;
        }
        case SYS_WIFI_STATUS_AUTOCONNECT_WAIT:
        {
            OSAL_SEM_Pend(&g_SyswifiSemaphore, OSAL_WAIT_FOREVER);
            if(true == SYS_WIFI_GetAutoConnect())
            {
				if (WDRV_PIC32MZW_STATUS_OK == WDRV_PIC32MZW_RegDomainSet(Obj->wdrvHandle, SYS_WIFI_GetCountryCode(), SYS_WIFI_SetRegDomainCallback))
                {
					SYS_WIFI_PrintConfig();
                    Obj->status = SYS_WIFI_STATUS_TCPIP_WAIT_FOR_TCPIP_INIT;
                }
            }
			OSAL_SEM_Post(&g_SyswifiSemaphore);
			break;
        }
		case SYS_WIFI_STATUS_TCPIP_WAIT_FOR_TCPIP_INIT:
        {
            OSAL_SEM_Pend(&g_SyswifiSemaphore, OSAL_WAIT_FOREVER);
            tcpipStat = TCPIP_STACK_Status(sysObj.tcpip);
            
            if(tcpipStat < 0)
            {   
                SYS_CONSOLE_MESSAGE("  TCP/IP stack initialization failed!\r\n");
                Obj->status = SYS_WIFI_STATUS_TCPIP_ERROR;
            }
            else if(tcpipStat == SYS_STATUS_READY)
            {
                netH = TCPIP_STACK_NetHandleGet("PIC32MZW1");
					TCPIP_DHCPS_Enable(netH);							//Enable DHCP Server in AP mode
            }                
            Obj->status = SYS_WIFI_STATUS_CONNECT_REQ;
            OSAL_SEM_Post(&g_SyswifiSemaphore);
            break;
        }
        case SYS_WIFI_STATUS_CONNECT_REQ:
		{
            OSAL_SEM_Pend(&g_SyswifiSemaphore, OSAL_WAIT_FOREVER);
			if(SYS_WIFI_SUCCESS == SYS_WIFI_SetChannel())
            {								
                if(SYS_WIFI_SUCCESS == SYS_WIFI_ConfigReq())
                {
                    if(SYS_WIFI_SUCCESS == SYS_WIFI_ConnectReq())
                    {
						Obj->status = SYS_WIFI_STATUS_WAIT_FOR_AP_IP;
                    }
                }                
            }
            OSAL_SEM_Post(&g_SyswifiSemaphore);
            break;
		}
		case SYS_WIFI_STATUS_WAIT_FOR_AP_IP:
		{
            OSAL_SEM_Pend(&g_SyswifiSemaphore, OSAL_WAIT_FOREVER);
			ipAddr.Val = TCPIP_STACK_NetAddress(netH);
			if(dwLastIP.Val != ipAddr.Val)
            {
				dwLastIP.Val = ipAddr.Val;

				SYS_CONSOLE_MESSAGE(TCPIP_STACK_NetNameGet(netH));
				SYS_CONSOLE_MESSAGE(" AP Mode IP Address: ");
				SYS_CONSOLE_PRINT("%d.%d.%d.%d \r\n", ipAddr.v[0], ipAddr.v[1], ipAddr.v[2], ipAddr.v[3]);

                Obj->status = SYS_WIFI_STATUS_TCPIP_READY;
                OSAL_SEM_Post(&g_SyswifiSemaphore);
            bool prov_connectstatus = true;
            SYS_WIFIPROV_CtrlMsg(WiFiProvObject,SYS_WIFIPROV_CONNECT,&prov_connectstatus,sizeof(bool));

			}
			break;
		}
		case SYS_WIFI_STATUS_WAIT_FOR_STA_IP:
		{
			TCPIP_DHCPS_LEASE_HANDLE dhcpsLease = 0;
            TCPIP_DHCPS_LEASE_ENTRY dhcpsLeaseEntry;
             
            OSAL_SEM_Pend(&g_SyswifiSemaphore, OSAL_WAIT_FOREVER);
            if((true == ap_connected) && (true == ap_bssid.valid))
            {
                dhcpsLease = TCPIP_DHCPS_LeaseEntryGet(netH, &dhcpsLeaseEntry, dhcpsLease);

                if ((0 != dhcpsLease) && (0 == memcmp(&dhcpsLeaseEntry.hwAdd, ap_bssid.addr, WDRV_PIC32MZW_MAC_ADDR_LEN)))
                {
                    SYS_CONSOLE_PRINT("\r\n Connected STA IP:%d.%d.%d.%d \r\n", dhcpsLeaseEntry.ipAddress.v[0], dhcpsLeaseEntry.ipAddress.v[1], dhcpsLeaseEntry.ipAddress.v[2], dhcpsLeaseEntry.ipAddress.v[3]);
                    
                    ipAddr.Val = dhcpsLeaseEntry.ipAddress.Val;
                    Obj->status = SYS_WIFI_STATUS_TCPIP_READY;
                    OSAL_SEM_Post(&g_SyswifiSemaphore);
                    SYS_WIFI_CallBackFun(SYS_WIFI_CONNECT,&ipAddr,gCookie);
                    OSAL_SEM_Pend(&g_SyswifiSemaphore, OSAL_WAIT_FOREVER);
                }
            }
            OSAL_SEM_Post(&g_SyswifiSemaphore);
			break;
		}
        case SYS_WIFI_STATUS_TCPIP_READY:
        {  
            break;
        }
        case SYS_WIFI_STATUS_TCPIP_ERROR:
        {   
            SYS_STATUS tcpipStat;
            tcpipStat = TCPIP_STACK_Status(sysObj.tcpip);
            
            if(tcpipStat < 2)
			{
                Obj->status = SYS_WIFI_STATUS_TCPIP_ERROR;
			}
            else
                Obj->status = SYS_WIFI_STATUS_TCPIP_WAIT_FOR_TCPIP_INIT;
            
            break;
        }
        default:
        {
            break;
        }
    }
	
    SYS_WIFIPROV_Tasks (WiFiProvObject);

    return Obj->status;
}

void SYS_WIFI_WIFIPROVCallBack(uint32_t event, void * data,void *cookie )
{
    char *cookie_l = (void *) cookie ;
    if(cookie_wifiprov ==  *cookie_l)
    {
        switch(event)
        {
            case SYS_WIFIPROV_SETCONFIG:
            {
                SYS_WIFIPROV_CONFIG *wificonfig = (SYS_WIFIPROV_CONFIG*) data;
                if(false == InitWiFiSrvc)
                {
                    InitWiFiSrvc = true;
                    memcpy(&gWiFiConfig,wificonfig,sizeof(SYS_WIFIPROV_CONFIG));
                } 
                else
                {
						SYS_CONSOLE_MESSAGE("######################################Rebooting the Device ###############################\r\n"); 
						Soft_Reset();
				}
				SYS_WIFI_CallBackFun(SYS_WIFI_PROVCONFIG,data,gCookie);		
               
                break;
            }
            case SYS_WIFIPROV_GETCONFIG:
            {
                SYS_WIFI_CallBackFun(SYS_WIFI_GETCONFIG,data,gCookie);             
                break;
            }            
            default:
                break;            
        }
    }
}
// *****************************************************************************
// *****************************************************************************
// Section:  SYS WIFI Interface Functions
// *****************************************************************************
// *****************************************************************************
SYS_MODULE_OBJ SYS_WIFI_Initialize(SYS_WIFI_CONFIG *config,SYS_WIFI_CALLBACK callback,void *cookie )
{

	if(SYS_WIFI_STATUS_NONE == SYS_WIFI_GetTaskstatus())
	{
        if(OSAL_SEM_Create(&g_SyswifiSemaphore, OSAL_SEM_TYPE_BINARY, 1, 1) != OSAL_RESULT_TRUE)
        {
            SYS_CONSOLE_MESSAGE("Failed to Initialize Wi-Fi Service as Semaphore NOT created\r\n");
            return SYS_MODULE_OBJ_INVALID;
        }
        
		if(callback != NULL)
			SYS_WIFI_REGCB(callback);
        		
        WDRV_PIC32MZW_BSSCtxSetDefaults(&gWiFiObj.bssCtx);
        WDRV_PIC32MZW_AuthCtxSetDefaults(&gWiFiObj.authCtx);
		SYS_WIFI_SetTaskstatus(SYS_WIFI_STATUS_INIT);
		SYS_WIFI_SetCookie(cookie);
		WiFiProvObject= SYS_WIFIPROV_Initialize ((SYS_WIFIPROV_CONFIG *)config,SYS_WIFI_WIFIPROVCallBack,&cookie_wifiprov);
	 
		return (SYS_MODULE_OBJ) &gWiFiObj;
	}
	return SYS_MODULE_OBJ_INVALID;
}

SYS_WIFI_RESULT SYS_WIFI_Deinitialize (SYS_MODULE_OBJ object) 
{
    uint32_t ret = SYS_WIFI_FAILURE;

    
    if (&gWiFiObj != (SYS_WIFI_OBJ *)object)
        ret = SYS_WIFI_OBJ_INVALID;
    else
    {
		if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_APStop(gWiFiObj.wdrvHandle))
		{
			SYS_CONSOLE_MESSAGE(" AP mode Stop Failed \n");
		}
		WDRV_PIC32MZW_Close(gWiFiObj.wdrvHandle);
		InitWiFiSrvc = false;
        memset(&gWiFiObj,0,sizeof(SYS_WIFI_OBJ));
        memset(gWiFiCallBack,0,sizeof(gWiFiCallBack));
        SYS_WIFI_SetTaskstatus(SYS_WIFI_STATUS_NONE);
        SYS_WIFIPROV_Deinitialize(WiFiProvObject);
		if (OSAL_SEM_Delete(&g_SyswifiSemaphore) != OSAL_RESULT_TRUE)
        {
            SYS_CONSOLE_MESSAGE("Failed to Delete Wi-Fi Service Semaphore \r\n");
        }
		ret =SYS_WIFI_SUCCESS;
    }

    return ret;
}

uint8_t SYS_WIFI_GetStatus ( SYS_MODULE_OBJ object) 
{
    uint8_t ret = SYS_WIFI_FAILURE;
    
    OSAL_SEM_Pend(&g_SyswifiSemaphore, OSAL_WAIT_FOREVER);
    
    if (&gWiFiObj != (SYS_WIFI_OBJ *)object)
        ret = SYS_WIFI_OBJ_INVALID;
    else
        ret = ((SYS_WIFI_OBJ *)object)->status;
    
    OSAL_SEM_Post(&g_SyswifiSemaphore);
    
    return ret;
}

uint8_t SYS_WIFI_Tasks (SYS_MODULE_OBJ object)
{
    uint8_t ret = SYS_WIFI_FAILURE;
        
    if (&gWiFiObj != (SYS_WIFI_OBJ *)object)
        ret = SYS_WIFI_OBJ_INVALID;
    else
        ret = SYS_WIFI_ExecuteBlock(object);
    
    return ret;
}

SYS_WIFI_RESULT SYS_WIFI_CtrlMsg (SYS_MODULE_OBJ object,uint32_t event,void *buffer,uint32_t length )
{
    uint8_t ret = SYS_WIFI_FAILURE;
    uint8_t *channel;
    bool *scan_type;
	
	OSAL_SEM_Pend(&g_SyswifiSemaphore, OSAL_WAIT_FOREVER);
	
	if (&gWiFiObj != (SYS_WIFI_OBJ *)object)
        ret = SYS_WIFI_OBJ_INVALID;
    else
    {
        switch(event)
        {
            case SYS_WIFI_CONNECT:
                //if service is already processing pending request from client then ignore new request
                if (SYS_WIFI_STATUS_CONNECT_REQ != gWiFiObj.status)
                {
                    if((buffer) && (length == sizeof(SYS_WIFI_CONFIG)))
                        ret = SYS_WIFI_SetConfig((SYS_WIFI_CONFIG *)buffer,SYS_WIFI_STATUS_CONNECT_REQ);
                }
                else
                {
                    ret = SYS_WIFI_CONNECT_FAILURE;
                }
                break;
            
            case SYS_WIFI_REGCALLBACK:
                if((buffer) && (length == 4))
                    ret = SYS_WIFI_REGCB(buffer);
                break;
            case SYS_WIFI_DISCONNECT:
                ret = SYS_WIFI_DisConnect();
                break;
            case SYS_WIFI_GETCONFIG:
                if(true == InitWiFiSrvc)
                {
                    if((buffer) && (length == sizeof(SYS_WIFI_CONFIG)))
                    {
                        memcpy(buffer,&gWiFiConfig,sizeof(gWiFiConfig));
                        ret = SYS_WIFI_SUCCESS;
                    }
                }
                else
                {
                    ret = SYS_WIFI_SERVICE_UNINITIALIZE;
                }
                break;
            
            case SYS_WIFI_SCANREQ:
                //if service is already processing pending connection request from client then ignore new request
                if((SYS_WIFI_STATUS_CONNECT_REQ != gWiFiObj.status) && (buffer) && (length == 2))
                {
                    channel  = (uint8_t *) buffer;
                    scan_type = (bool *) buffer+1;
                    ret = SYS_WIFI_SetScan(*channel,*scan_type);
                }
                break;
        }
    }
	OSAL_SEM_Post(&g_SyswifiSemaphore);
    return ret;
}
/* *****************************************************************************
 End of File
 */
