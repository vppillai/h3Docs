/*******************************************************************************
  Wi-Fi Provision System Service Implementation.

  Company:
    Microchip Technology Inc.

  File Name:
    sys_wifiprov.c

  Summary:
    Source code for the Wi-Fi Provision system service implementation.

  Description:
    This file contains the source code for the Wi-Fi Provision system service
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
#include "wdrv_pic32mzw_authctx.h"
#include "wdrv_pic32mzw_bssctx.h"
#include "wdrv_pic32mzw_bssfind.h"
#include "wdrv_pic32mzw_assoc.h"
#include "configuration.h"
#include "system/wifiprov/sys_wifiprov.h"
#include "system/wifiprov/cJSON.h"

// *****************************************************************************
// *****************************************************************************
// Section: Type Definitions
// *****************************************************************************
// *****************************************************************************


typedef enum
{
    SYS_WIFIPROV_NVM_WRITE=0,
    SYS_WIFIPROV_NVM_ERASE,
    SYS_WIFIPROV_NVM_READ,
	SYS_WIFIPROV_NONE = 255
}SYS_WIFIPROV_NVMTYPEOPER;    //NVM Operation


typedef struct {
    // The WiFi service current status 
    SYS_WIFIPROV_STATUS status;
	
	// The WiFi service NVM type operation  
    SYS_WIFIPROV_NVMTYPEOPER nvmtypeofOperation;
	
}SYS_WIFIPROV_OBJ;		//Wi-Fi Provision system service Object

// *****************************************************************************
// *****************************************************************************
// Section: Global Data
// *****************************************************************************
// *****************************************************************************

static SYS_WIFIPROV_OBJ gWiFiProvObj = { SYS_WIFIPROV_STATUS_NONE,SYS_WIFIPROV_NONE} ;
static SYS_WIFIPROV_CONFIG gWiFiProvConfig CACHE_ALIGN;
static SYS_WIFIPROV_CONFIG gWiFiProvConfig_read;
static SYS_WIFIPROV_CALLBACK gWiFiProvCallBack ;
static void *gCookie ;
TCP_SOCKET              wifi_prov_socket = INVALID_SOCKET;
TCPIP_TCP_SIGNAL_HANDLE wifi_prov_handle;


static void SYS_WIFIPROV_WriteConfig();
static bool SYS_WIFIPROV_CMDInit();
static int SYS_WIFIPROV_CMDProcess(SYS_CMD_DEVICE_NODE* pCmdIO, int argc, char** argv) ;
static int SYS_WIFIPROV_CMDHelp(SYS_CMD_DEVICE_NODE* pCmdIO, int argc, char** argv);
static void SYS_WIFIPROV_InitSocket();
static void SYS_WIFIPROV_DeInitSocket();
static uint8_t parser_data(uint8_t sbuff[],uint8_t dbuff[],uint8_t dbufflen,uint8_t val,uint8_t offset);
static void SYS_WIFIPROV_PrintConfig();
// *****************************************************************************
// *****************************************************************************
// Section: Local Functions
// *****************************************************************************
// *****************************************************************************

static inline void SYS_WIFIPROV_SetTaskstatus(SYS_WIFIPROV_STATUS val)
{
    gWiFiProvObj.status = val;
}
static inline SYS_WIFIPROV_STATUS SYS_WIFIPROV_GetTaskstatus()
{
    return gWiFiProvObj.status;
}

static inline void SYS_WIFIPROV_CallBackFun(uint32_t event, void * data,void *cookie)
{    
	if(gWiFiProvCallBack)
		gWiFiProvCallBack(event,data,cookie);
}
static void inline SYS_WIFIPROV_SetCookie(void *cookie)
{
    gCookie = cookie;
}
static void inline SYS_WIFIPROV_InitConfig(SYS_WIFIPROV_CONFIG *config)
{   
    if(!config)
    {   

        gWiFiProvConfig.mode = SYS_WIFI_DEVMODE;
		gWiFiProvConfig.save_config = SYS_WIFIPROV_SAVECONFIG;
		memcpy(gWiFiProvConfig.countrycode,SYS_WIFI_COUNTRYCODE,strlen(SYS_WIFI_COUNTRYCODE));


        gWiFiProvConfig.apconfig.channel = SYS_WIFI_AP_CHANNEL;
        gWiFiProvConfig.apconfig.ssid_visibility = SYS_WIFI_AP_SSIDVISIBILE;
        gWiFiProvConfig.apconfig.auth_type = SYS_WIFI_AP_AUTHTYPE;
        memcpy(gWiFiProvConfig.apconfig.ssid,SYS_WIFI_AP_SSID,sizeof(SYS_WIFI_AP_SSID));        
        memcpy(gWiFiProvConfig.apconfig.psk,SYS_WIFI_AP_PWD,sizeof(SYS_WIFI_AP_PWD));        
        
		SYS_WIFIPROV_SetTaskstatus(SYS_WIFIPROV_STATUS_NVM_READ);
		
    }
    else
    {

        memcpy(&gWiFiProvConfig,config,sizeof(SYS_WIFIPROV_CONFIG));
		SYS_WIFIPROV_WriteConfig();
    }
}

static void SYS_WIFIPROV_CheckConfig()
{

	//if NVM flash WiFi config read is empty than save_config will be 0xFF
    if(0xFF != gWiFiProvConfig_read.save_config)
    {
        SYS_WIFIPROV_SetTaskstatus(SYS_WIFIPROV_STATUS_WAITFORREQ);        
        memcpy(&gWiFiProvConfig,&gWiFiProvConfig_read,sizeof(SYS_WIFIPROV_CONFIG));
        memset(&gWiFiProvConfig_read,0,sizeof(SYS_WIFIPROV_CONFIG));
        SYS_WIFIPROV_CallBackFun(SYS_WIFIPROV_SETCONFIG,&gWiFiProvConfig,gCookie);

    }
    else         //Write valid Wi-Fi Config into NVM
    {
        SYS_WIFIPROV_SetTaskstatus(SYS_WIFIPROV_STATUS_NVM_ERASE);
    }    
}
static inline void SYS_WIFIPROV_NVMRead()
{
    gWiFiProvObj.nvmtypeofOperation = SYS_WIFIPROV_NVM_READ;
    NVM_Read( (uint32_t *)&gWiFiProvConfig_read, sizeof(gWiFiProvConfig_read), SYS_WIFIPROV_NVMADDR);

}
static inline void SYS_WIFIPROV_NVMWrite()
{
    gWiFiProvObj.nvmtypeofOperation = SYS_WIFIPROV_NVM_WRITE;
    NVM_RowWrite( (uint32_t *)&gWiFiProvConfig, SYS_WIFIPROV_NVMADDR);


}
static inline void SYS_WIFIPROV_NVMErase()
{
    gWiFiProvObj.nvmtypeofOperation = SYS_WIFIPROV_NVM_ERASE;
    NVM_PageErase(SYS_WIFIPROV_NVMADDR);   
}

/*void SYS_WIFIPROV_NVMCallback(void* context)
{
    
    SYS_WIFIPROV_OBJ * nvmData = (SYS_WIFIPROV_OBJ *)context;
    
    if(NVM_ErrorGet() == NVM_ERROR_NONE) {
    
        if(nvmData->nvmtypeofOperation == SYS_WIFIPROV_NVM_WRITE){
            SYS_WIFIPROV_CallBackFun(SYS_WIFIPROV_SETCONFIG,&gWiFiProvConfig,gCookie);        
        }        
    } 
}*/


static void SYS_WIFIPROV_PrintConfig()
{
    SYS_CONSOLE_PRINT("\r\n mode=%d (0-STA,1-AP) save_config=%d countrycode=%s\r\n ",gWiFiProvConfig.mode,gWiFiProvConfig.save_config,gWiFiProvConfig.countrycode);
    SYS_CONSOLE_PRINT("\r\n AP Configuration :\r\n  channel=%d \r\n ssid_visibility=%d \r\n ssid=%s \r\n passphase=%s \r\n authentication type=%d (1-Open,2-WEP,3-Mixed mode(WPA/WPA2),4-WPA2) \r\n",gWiFiProvConfig.apconfig.channel,gWiFiProvConfig.apconfig.ssid_visibility,gWiFiProvConfig.apconfig.ssid,gWiFiProvConfig.apconfig.psk,gWiFiProvConfig.apconfig.auth_type);
}

static void SYS_WIFIPROV_WriteConfig()
{
    if(true == gWiFiProvConfig.save_config)
    {
        SYS_WIFIPROV_SetTaskstatus(SYS_WIFIPROV_STATUS_NVM_ERASE);
    } 
    else
    {
        SYS_WIFIPROV_CallBackFun(SYS_WIFIPROV_SETCONFIG,&gWiFiProvConfig,gCookie);
    }    
}

static SYS_WIFIPROV_STATUS SYS_WIFIPROV_ExecuteBlock(SYS_MODULE_OBJ object)
{
    SYS_WIFIPROV_OBJ *Obj = (SYS_WIFIPROV_OBJ *)object;
    
    switch ( Obj->status )
    {
       case SYS_WIFIPROV_STATUS_NVM_READ:
        {
            if(!NVM_IsBusy())
            {
                SYS_WIFIPROV_NVMRead();
                Obj->status = SYS_WIFIPROV_STATUS_CONFIG_CHECK;
            }
            break;
        }
        case SYS_WIFIPROV_STATUS_CONFIG_CHECK:
        {
            SYS_WIFIPROV_CheckConfig();
            break;
        }
        case SYS_WIFIPROV_STATUS_NVM_ERASE:
        {
            if(!NVM_IsBusy())
            {
                SYS_WIFIPROV_NVMErase();
                Obj->status = SYS_WIFIPROV_STATUS_NVM_WRITE;  
            }
            break;
        }   
        case SYS_WIFIPROV_STATUS_NVM_WRITE:
        {
            if(!NVM_IsBusy())
            {
                SYS_WIFIPROV_NVMWrite();
                Obj->status = SYS_WIFIPROV_STATUS_WAITFORWRITE;
            }
            break;
        }
        case SYS_WIFIPROV_STATUS_WAITFORWRITE:
        {
            if(!NVM_IsBusy())
            {
                SYS_WIFIPROV_CallBackFun(SYS_WIFIPROV_SETCONFIG,&gWiFiProvConfig,gCookie);
                Obj->status = SYS_WIFIPROV_STATUS_WAITFORREQ;
            }
            break;
        }   
        case SYS_WIFIPROV_STATUS_WAITFORREQ:
        default :
        {
            break;
        }
    }
    
    return Obj->status;
}
static bool SYS_WIFIPROV_ConfigValidate(SYS_WIFIPROV_CONFIG lWiFiProvConfig)
{

	if(!(lWiFiProvConfig.mode == SYS_WIFIPROV_AP))
    {
        SYS_CONSOLE_MESSAGE(" set valid boot mode \r\n");
        return true;
    }    
    if(!((lWiFiProvConfig.save_config == true) || (lWiFiProvConfig.save_config == false)))    
    {
        SYS_CONSOLE_MESSAGE(" set valid save config value \r\n");
        return true;
    }
    if(SYS_WIFIPROV_AP == (SYS_WIFIPROV_MODE)lWiFiProvConfig.mode)
    {
        if(!((lWiFiProvConfig.apconfig.channel >= 1) && (lWiFiProvConfig.apconfig.channel <= 13)))
        {
            SYS_CONSOLE_MESSAGE(" set valid access point mode channel number \r\n");
            return true;
        }
        if(!((lWiFiProvConfig.apconfig.ssid_visibility == true) || (lWiFiProvConfig.apconfig.ssid_visibility == false)))
        {
            SYS_CONSOLE_MESSAGE(" set valid access point mode SSID visibility \r\n");
            return true;
        }
        if(!(((lWiFiProvConfig.apconfig.auth_type == SYS_WIFIPROV_OPEN) || (lWiFiProvConfig.apconfig.auth_type == SYS_WIFIPROV_WPA2)) || 
		(lWiFiProvConfig.apconfig.auth_type == SYS_WIFIPROV_WPAWPA2MIXED)))                //ignore WEP as not support 
        {
            SYS_CONSOLE_MESSAGE(" set valid access point mode Auth value \r\n");
            return true;
        }
        if((lWiFiProvConfig.apconfig.auth_type == SYS_WIFIPROV_WPAWPA2MIXED) || (lWiFiProvConfig.apconfig.auth_type == SYS_WIFIPROV_WPA2))
        {
            if(strlen((const char *)lWiFiProvConfig.apconfig.psk) < 8)
            {
                SYS_CONSOLE_MESSAGE(" set valid access point mode passphase \r\n");
                return true;
            }
        }
    } 
    return false;
}


static const SYS_CMD_DESCRIPTOR    WiFiCmdTbl[]=
{
    {"wifiprov",        (SYS_CMD_FNC) SYS_WIFIPROV_CMDProcess,          ": WiFi provision commands processing"},
    {"wifiprovhelp",    (SYS_CMD_FNC) SYS_WIFIPROV_CMDHelp, 			": WiFi provision commands help "},
};

static bool SYS_WIFIPROV_CMDInit()
{
    bool ret = SYS_WIFIPROV_SUCCESS;
    
    if (!SYS_CMD_ADDGRP(WiFiCmdTbl, sizeof(WiFiCmdTbl)/sizeof(*WiFiCmdTbl), "wifiprov", ": WiFi provision commands"))
    {
		ret = SYS_WIFIPROV_FAILURE;
    }
    return ret;
}


static int SYS_WIFIPROV_CMDProcess(SYS_CMD_DEVICE_NODE* pCmdIO, int argc, char** argv) 
{
    char val = '"';
    SYS_WIFIPROV_CONFIG lWiFiProvConfig;
    bool error = false;
    
    if((argc >= 7) && (!strcmp(argv[1],"set")))
    {        

		if(SYS_WIFIPROV_AP == (SYS_WIFIPROV_MODE)strtol(argv[2],NULL,0))
        {            
            lWiFiProvConfig.mode = strtol(argv[2],NULL,0);
            lWiFiProvConfig.save_config =  strtol(argv[3],NULL,0);
			if(strlen((const char *)argv[4]) <= sizeof(lWiFiProvConfig.countrycode))
            {
				parser_data((uint8_t *)argv[4],lWiFiProvConfig.countrycode,sizeof(lWiFiProvConfig.countrycode),val,0);
			} else
                error = true;
			lWiFiProvConfig.apconfig.channel = strtol(argv[5],NULL,0);            
            lWiFiProvConfig.apconfig.ssid_visibility =  strtol(argv[6],NULL,0);
            lWiFiProvConfig.apconfig.auth_type =strtol(argv[7],NULL,0);
            
            if(strlen((const char *)argv[8]) <= sizeof(lWiFiProvConfig.apconfig.ssid))
            {
                parser_data((uint8_t *)argv[8],lWiFiProvConfig.apconfig.ssid,sizeof(lWiFiProvConfig.apconfig.ssid),val,0);
            }
            else
                error = true;
            
            if(argc == 10)
            {
                if(strlen((const char *)argv[9]) <= sizeof(lWiFiProvConfig.apconfig.psk))
                {
                    parser_data((uint8_t *)argv[9],lWiFiProvConfig.apconfig.psk,sizeof(lWiFiProvConfig.apconfig.psk),val,0);                    
                }
                else
                    error = true;
            }
            else
            {
                memset(lWiFiProvConfig.apconfig.psk,0,sizeof(lWiFiProvConfig.apconfig.psk));
            }
            
            if((!error) &&(!SYS_WIFIPROV_ConfigValidate(lWiFiProvConfig)))
            {
                gWiFiProvConfig.mode = lWiFiProvConfig.mode;
                gWiFiProvConfig.save_config = lWiFiProvConfig.save_config;
                memcpy(&gWiFiProvConfig.apconfig,&lWiFiProvConfig.apconfig,sizeof(SYS_WIFIPROV_AP_CONFIG));
                SYS_WIFIPROV_WriteConfig();
                //SYS_WIFIPROV_PrintConfig();
            }else
                SYS_CONSOLE_PRINT(" Wrong Command\n");

        }
		else 
            SYS_CONSOLE_PRINT(" Wrong Command\n");
	}
    else if((argc == 2) && (!strcmp(argv[1],"get")))
    {
        SYS_WIFIPROV_PrintConfig();
    }
    else
    {
        SYS_CONSOLE_PRINT(" Wrong Command\n"); 
    }
    
	return SYS_WIFIPROV_SUCCESS;
}
static int SYS_WIFIPROV_CMDHelp(SYS_CMD_DEVICE_NODE* pCmdIO, int argc, char** argv)
{
    SYS_CONSOLE_MESSAGE("\r\nUsage information:");
    SYS_CONSOLE_MESSAGE("\r\nwifiprov set <bootmode> <save config> <countrycode> <channel> <auto_connect>/<ssid_visibility> <authtype> <ssid_name> <psk_name>\r\n\r\n");
	SYS_CONSOLE_MESSAGE("bootmode                     : 0 -STA,1 - AP \r\n\r\n"
                        "save config                  : 1- save wi-fi config in NVM\r\n"
                        "                               0- Don't save config in NVM\r\n\r\n"
                        "country code                 : Regulatory domain country code configuration \r\n\r\n"
                        "                               GEN - General(Used for Production Test)\r\n"
                        "                               USA - North America \r\n\r\n"
                        "                               EMEA - Europe \r\n\r\n"
                        "                               JPN - Japan \r\n\r\n"            
                        "channel                      : 0 - Enable all channel(STA mode Only)\r\n"
                        "                               1 to 13 - set specific channel \r\n\r\n"
                        "auto connect (STA mode only) : 1- auto connect to HomeAP\r\n"
                        "                               0- Wait for user input \r\n"
                        "ssid_visibility(AP mode only): 1-broadcast SSID\r\n"
                        "                               0-hidden SSID \r\n"
                        "authtype                     : 1-Open\r\n"
                        "                               2-WEP\r\n"
                        "                               3-Mixed mode(WPA/WPA2)\r\n"
                        "                               4-WPA2 \r\n"
                        "ssid_name                    : SSID name \r\n"
                        "psk                          : Passphrase\r\n\r\n");
    SYS_CONSOLE_MESSAGE("Example STA Mode             : wifiprov set 0 1 1 1 1 \"DEMO_AP\" \"password\" \r\n");
    SYS_CONSOLE_MESSAGE("Example AP Mode              : wifiprov set 1 1 1 1 1 \"DEMO_SOFTAP\" \"password\" \r\n");
    SYS_CONSOLE_MESSAGE("wifiprov get                 : Get WiFi Provision Configuration \r\n");
    SYS_CONSOLE_MESSAGE("wifiprov debug level <value> : Set WiFi Provision Debug level value in hex \r\n");
    SYS_CONSOLE_MESSAGE("wifiprov debug flow <value>  : Set WiFi Provision Debug flow value in hex \r\n");
    return SYS_WIFIPROV_SUCCESS;
}


static uint8_t parser_data(uint8_t sbuff[],uint8_t dbuff[],uint8_t dbufflen,uint8_t val,uint8_t offset)
{
    uint8_t idx1=offset+1,idx2=offset,idx3=0;
        
    memset(dbuff,0,dbufflen);
    for(;(sbuff[idx2] == val) && (sbuff[idx1] != val);)
    {
        dbuff[idx3++] = sbuff[idx1++];
        
       /*SYS CMD service replacing space with NULL*/
       if(dbuff[idx3-1] == 0x00)
            dbuff[idx3-1] =0x20;

    }
    dbuff[idx3]='\0';

    return idx3+1;
}
static void SYS_WIFIPROV_DataUpdate(uint8_t Buffer[])
{
 
    cJSON *root = NULL,*child =NULL,*sub =NULL;
    SYS_WIFIPROV_CONFIG lWiFiProvConfig;
    bool error = false;
    root= cJSON_Parse((const char*)Buffer);
	if(root)
    {
        memset(&lWiFiProvConfig,0,sizeof(SYS_WIFIPROV_CONFIG));
		child = cJSON_GetObjectItem(root, "mode");
        if(child)
            lWiFiProvConfig.mode = child->valueint;
		else
			error = true;
		
		child = cJSON_GetObjectItem(root, "save_config");
        if(child)
            lWiFiProvConfig.save_config = child->valueint;
		else
			error = true;
		 
		child = cJSON_GetObjectItem(root, "countrycode");
        if(child)    
    		memcpy(lWiFiProvConfig.countrycode,child->valuestring,strlen(child->valuestring));
		else
			error = true;

		child = cJSON_GetObjectItem(root, "AP");
        if(child)
        {
            sub = cJSON_GetObjectItem(child, "ch");
            if(sub)
                lWiFiProvConfig.apconfig.channel = sub->valueint;
			else
                error = true;

            sub = cJSON_GetObjectItem(child, "ssidv");
            if(sub)
                lWiFiProvConfig.apconfig.ssid_visibility = sub->valueint;
			else
                error = true;
            
			sub = cJSON_GetObjectItem(child, "auth");
            if(sub)
                lWiFiProvConfig.apconfig.auth_type = sub->valueint;
			else
                error = true;

            sub = cJSON_GetObjectItem(child, "SSID");		
            if(sub)
            {
                if(strlen(sub->valuestring) <= sizeof(lWiFiProvConfig.apconfig.ssid))
                {
                    memcpy(lWiFiProvConfig.apconfig.ssid,sub->valuestring,strlen(sub->valuestring));
                }
				else
					error = true;
            }
			else
                error = true;

            sub = cJSON_GetObjectItem(child, "PWD");
            if(sub)
            {
                if(strlen(sub->valuestring) <= sizeof(lWiFiProvConfig.apconfig.psk))
                {
                    memcpy(lWiFiProvConfig.apconfig.psk,sub->valuestring,strlen(sub->valuestring));
                }else
                    error = true;
            }
			else
                error = true;
        }


       if((!error) && (!SYS_WIFIPROV_ConfigValidate(lWiFiProvConfig)))
       {
            gWiFiProvConfig.mode = lWiFiProvConfig.mode;
            gWiFiProvConfig.save_config = lWiFiProvConfig.save_config;
            memcpy(&gWiFiProvConfig.apconfig,&lWiFiProvConfig.apconfig,sizeof(SYS_WIFIPROV_AP_CONFIG));
            //SYS_WIFIPROV_PrintConfig();
            SYS_WIFIPROV_WriteConfig();
       }
       else
           SYS_CONSOLE_PRINT(" Wrong Command\n");

	}

}

static void SYS_WIFIPROV_Socket_CB (TCP_SOCKET hTCP, TCPIP_NET_HANDLE hNet,TCPIP_TCP_SIGNAL_TYPE sigType, const void* param)
{
   uint8_t Buffer[512];
    switch(sigType)
    {

        case TCPIP_TCP_SIGNAL_RX_DATA:
        {
            memset(Buffer,0,sizeof(Buffer));
            uint8_t byte = TCPIP_TCP_ArrayGet(wifi_prov_socket, Buffer, sizeof(Buffer));

            if(byte) 
            {                
                SYS_WIFIPROV_DataUpdate(Buffer);
             }
            TCPIP_TCP_Discard(wifi_prov_socket);
            break;
        }        
        default:
            break;
    }
}

static void SYS_WIFIPROV_InitSocket()
{
    
    TCPIP_NET_HANDLE    netH   = TCPIP_STACK_NetHandleGet("PIC32MZW1");
    IPV4_ADDR           IPAddr;
    IPAddr.Val= TCPIP_STACK_NetAddress(netH); 
    if(IPAddr.Val)
    {
        if(wifi_prov_socket != INVALID_SOCKET)
        {
            TCPIP_TCP_Close(wifi_prov_socket);
            wifi_prov_socket = INVALID_SOCKET;

        }
        wifi_prov_socket = TCPIP_TCP_ServerOpen(IP_ADDRESS_TYPE_IPV4, SYS_WIFIPROV_SOCKETPORT, 0);
        if (wifi_prov_socket == INVALID_SOCKET)
        {
			SYS_CONSOLE_MESSAGE("Couldn't open Wi-Fi Provision service server socket \r\n");
        }
        wifi_prov_handle=TCPIP_TCP_SignalHandlerRegister(wifi_prov_socket,TCPIP_TCP_SIGNAL_RX_DATA,SYS_WIFIPROV_Socket_CB,NULL);
        if (wifi_prov_handle == NULL)
        {
			SYS_CONSOLE_MESSAGE("Couldn't create socket handle\r\n");
        }
    }
}

static void SYS_WIFIPROV_DeInitSocket()
{

    if(TCPIP_TCP_SignalHandlerDeregister(wifi_prov_socket,wifi_prov_handle))
        TCPIP_TCP_Close(wifi_prov_socket);

}


// *****************************************************************************
// *****************************************************************************
// Section:  SYS WIFI PROVISION Interface Functions
// *****************************************************************************
// *****************************************************************************
SYS_MODULE_OBJ SYS_WIFIPROV_Initialize( SYS_WIFIPROV_CONFIG *config,SYS_WIFIPROV_CALLBACK callback,void *cookie)
{
    SYS_WIFIPROV_OBJ    *wifiprovObj  =  (SYS_WIFIPROV_OBJ *) &gWiFiProvObj ;
    gWiFiProvCallBack = callback;
	if(SYS_WIFIPROV_STATUS_NONE == SYS_WIFIPROV_GetTaskstatus())
	{
		SYS_WIFIPROV_SetCookie(cookie);
		SYS_WIFIPROV_InitConfig(config);

		//NVM_CallbackRegister((NVM_CALLBACK) SYS_WIFIPROV_NVMCallback,(uintptr_t)&gWiFiProvObj);

		if(SYS_WIFIPROV_FAILURE ==SYS_WIFIPROV_CMDInit())
		{
			SYS_CONSOLE_MESSAGE("Failed to create WiFi provision Commands\r\n");
		}
		return (SYS_MODULE_OBJ) wifiprovObj;
	}
	return SYS_MODULE_OBJ_INVALID;
}


SYS_WIFIPROV_RESULT SYS_WIFIPROV_Deinitialize (SYS_MODULE_OBJ object) 
{
    uint32_t ret = SYS_WIFIPROV_FAILURE;
    if (&gWiFiProvObj != (SYS_WIFIPROV_OBJ *)object)
	{
        ret = SYS_WIFIPROV_OBJ_INVALID;
	}
    else
    {
        memset(&gWiFiProvObj,0,sizeof(SYS_WIFIPROV_OBJ));
        memset(&gWiFiProvConfig,0,sizeof(SYS_WIFIPROV_CONFIG));
        memset(&gWiFiProvConfig_read,0,sizeof(SYS_WIFIPROV_CONFIG));
        SYS_WIFIPROV_SetTaskstatus(SYS_WIFIPROV_STATUS_NONE);
        ret =SYS_WIFIPROV_SUCCESS;
    }
    return ret;
}

uint8_t SYS_WIFIPROV_GetStatus ( SYS_MODULE_OBJ object) 
{
    
    if (&gWiFiProvObj != (SYS_WIFIPROV_OBJ *)object)
	{
        return SYS_WIFIPROV_OBJ_INVALID;
	}
    else
        return ((SYS_WIFIPROV_OBJ *)object)->status;
}


uint8_t SYS_WIFIPROV_Tasks (SYS_MODULE_OBJ object)
{
    if (&gWiFiProvObj != (SYS_WIFIPROV_OBJ *)object)
	{
        return SYS_WIFIPROV_OBJ_INVALID;
	}
    else
        return SYS_WIFIPROV_ExecuteBlock(object);
}

SYS_WIFIPROV_RESULT SYS_WIFIPROV_CtrlMsg (SYS_MODULE_OBJ object,uint32_t event,void *buffer,uint32_t length )
{
    SYS_WIFIPROV_RESULT ret = SYS_WIFIPROV_SUCCESS;
    uint8_t *val;

	if (&gWiFiProvObj != (SYS_WIFIPROV_OBJ *)object)
	{
           ret = SYS_WIFIPROV_OBJ_INVALID;
	}
    else
    {
        switch(event)
        {
            case SYS_WIFIPROV_SETCONFIG:
                    if(buffer)
                       memcpy(&gWiFiProvConfig,(SYS_WIFIPROV_CONFIG *)buffer,sizeof(SYS_WIFIPROV_CONFIG));
                    SYS_WIFIPROV_WriteConfig();
                    break;
                    
            case SYS_WIFIPROV_GETCONFIG:
                    SYS_WIFIPROV_CallBackFun(SYS_WIFIPROV_GETCONFIG,&gWiFiProvConfig,gCookie);
                    break;
            
            case SYS_WIFIPROV_CONNECT:
                    val =  (uint8_t *)buffer;
                    if(*val == true)
                    {
                        SYS_WIFIPROV_InitSocket();
                    } 
                    else
                    {
                        SYS_WIFIPROV_DeInitSocket();
                    }
                    break;
           
            default :
                    break;                
        }
    }
	return ret;
}
