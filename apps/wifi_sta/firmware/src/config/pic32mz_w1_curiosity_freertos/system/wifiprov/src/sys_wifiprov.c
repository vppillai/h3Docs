/*******************************************************************************
  Wi-Fi Provision System Service Implementation

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
#include "wdrv_pic32mzw_authctx.h"
#include "wdrv_pic32mzw_bssctx.h"
#include "wdrv_pic32mzw_bssfind.h"
#include "wdrv_pic32mzw_assoc.h"
#include "configuration.h"
#include "system/wifiprov/sys_wifiprov.h"

// *****************************************************************************
// *****************************************************************************
// Section: Type Definitions
// *****************************************************************************
// *****************************************************************************

typedef enum 
{
    SYS_WIFIPROV_NVM_WRITE = 0,
    SYS_WIFIPROV_NVM_ERASE,
    SYS_WIFIPROV_NVM_READ,
    SYS_WIFIPROV_NONE = 255
} SYS_WIFIPROV_NVMTYPEOPER; //NVM Operation

typedef struct 
{
    // The WiFi service current status 
    SYS_WIFIPROV_STATUS status;
    // The WiFi service NVM type operation  
    SYS_WIFIPROV_NVMTYPEOPER nvmtypeofOperation;
} SYS_WIFIPROV_OBJ; //Wi-Fi Provision system service Object

// *****************************************************************************
// *****************************************************************************
// Section: Global Data
// *****************************************************************************
// *****************************************************************************

static      SYS_WIFIPROV_OBJ            g_wifiProvSrvcObj = {SYS_WIFIPROV_STATUS_NONE, SYS_WIFIPROV_NONE};
static      SYS_WIFIPROV_CONFIG         g_wifiProvSrvcConfig CACHE_ALIGN;
static      SYS_WIFIPROV_CONFIG         g_wifiProvSrvcConfigRead;
static      SYS_WIFIPROV_CALLBACK       g_wifiProvSrvcCallBack;
static      void *                      g_wifiProvSrvcCookie;

static      void                        SYS_WIFIPROV_WriteConfig(void);
static      bool                        SYS_WIFIPROV_CMDInit(void);
static      int                         SYS_WIFIPROV_CMDProcess(SYS_CMD_DEVICE_NODE* pCmdIO, int argc, char** argv);
static      int                         SYS_WIFIPROV_CMDHelp(SYS_CMD_DEVICE_NODE* pCmdIO, int argc, char** argv);
static      uint8_t                     parser_data(uint8_t sbuff[], uint8_t dbuff[], uint8_t dbufflen, uint8_t val, uint8_t offset);
static      void                        SYS_WIFIPROV_PrintConfig(void);

// *****************************************************************************
// *****************************************************************************
// Section: Local Functions
// *****************************************************************************
// *****************************************************************************
static inline void SYS_WIFIPROV_SetTaskstatus(SYS_WIFIPROV_STATUS val) 
{
    g_wifiProvSrvcObj.status = val;
}

static inline SYS_WIFIPROV_STATUS SYS_WIFIPROV_GetTaskstatus(void) 
{
    return g_wifiProvSrvcObj.status;
}

static inline void SYS_WIFIPROV_CallBackFun(uint32_t event, void * data, void *cookie) 
{
    if (g_wifiProvSrvcCallBack)
    {
        g_wifiProvSrvcCallBack(event, data, cookie);
    }
}

static void inline SYS_WIFIPROV_SetCookie(void *cookie) 
{
    g_wifiProvSrvcCookie = cookie;
}

static void inline SYS_WIFIPROV_InitConfig(SYS_WIFIPROV_CONFIG *config) 
{
    if (!config) 
    {
        g_wifiProvSrvcConfig.mode = SYS_WIFI_DEVMODE;
        g_wifiProvSrvcConfig.save_config = SYS_WIFIPROV_SAVECONFIG;
        memcpy(g_wifiProvSrvcConfig.countrycode, SYS_WIFI_COUNTRYCODE, strlen(SYS_WIFI_COUNTRYCODE));
        g_wifiProvSrvcConfig.staconfig.channel = 0;
        g_wifiProvSrvcConfig.staconfig.auto_connect = SYS_WIFI_STA_AUTOCONNECT;
        g_wifiProvSrvcConfig.staconfig.auth_type = SYS_WIFI_STA_AUTHTYPE;
        memcpy(g_wifiProvSrvcConfig.staconfig.ssid, SYS_WIFI_STA_SSID, sizeof (SYS_WIFI_STA_SSID));
        memcpy(g_wifiProvSrvcConfig.staconfig.psk, SYS_WIFI_STA_PWD, sizeof (SYS_WIFI_STA_PWD));
        SYS_WIFIPROV_SetTaskstatus(SYS_WIFIPROV_STATUS_NVM_READ);
    } 
    else 
    {
        memcpy(&g_wifiProvSrvcConfig, config, sizeof (SYS_WIFIPROV_CONFIG));
        SYS_WIFIPROV_WriteConfig();
    }
}

static void SYS_WIFIPROV_CheckConfig(void) 
{
    //if NVM flash WiFi config read is empty than save_config will be 0xFF
    if (0xFF != g_wifiProvSrvcConfigRead.save_config) 
    {
        SYS_WIFIPROV_SetTaskstatus(SYS_WIFIPROV_STATUS_WAITFORREQ);
        memcpy(&g_wifiProvSrvcConfig, &g_wifiProvSrvcConfigRead, sizeof (SYS_WIFIPROV_CONFIG));
        memset(&g_wifiProvSrvcConfigRead, 0, sizeof (SYS_WIFIPROV_CONFIG));
        SYS_WIFIPROV_CallBackFun(SYS_WIFIPROV_SETCONFIG, &g_wifiProvSrvcConfig, g_wifiProvSrvcCookie);
    } 
    else 
    {     //Write valid Wi-Fi Config into NVM  
        SYS_WIFIPROV_SetTaskstatus(SYS_WIFIPROV_STATUS_NVM_ERASE);
    }
}

static inline void SYS_WIFIPROV_NVMRead(void) 
{
    g_wifiProvSrvcObj.nvmtypeofOperation = SYS_WIFIPROV_NVM_READ;
    NVM_Read((uint32_t *) & g_wifiProvSrvcConfigRead, sizeof (g_wifiProvSrvcConfigRead), SYS_WIFIPROV_NVMADDR);
}

static inline void SYS_WIFIPROV_NVMWrite(void) 
{
    g_wifiProvSrvcObj.nvmtypeofOperation = SYS_WIFIPROV_NVM_WRITE;
    NVM_RowWrite((uint32_t *) & g_wifiProvSrvcConfig, SYS_WIFIPROV_NVMADDR);
}

static inline void SYS_WIFIPROV_NVMErase(void) 
{
    g_wifiProvSrvcObj.nvmtypeofOperation = SYS_WIFIPROV_NVM_ERASE;
    NVM_PageErase(SYS_WIFIPROV_NVMADDR);
}

static void SYS_WIFIPROV_PrintConfig(void) 
{
    SYS_CONSOLE_PRINT("\r\n mode=%d (0-STA,1-AP) save_config=%d countrycode=%s\r\n ", g_wifiProvSrvcConfig.mode, g_wifiProvSrvcConfig.save_config, g_wifiProvSrvcConfig.countrycode);
    SYS_CONSOLE_PRINT("\r\n STA Configuration :\r\n channel=%d \r\n auto_connect=%d \r\n ssid=%s \r\n passphase=%s \r\n authentication type=%d (1-Open,2-WEP,3-Mixed mode(WPA/WPA2),4-WPA2) \r\n", g_wifiProvSrvcConfig.staconfig.channel, g_wifiProvSrvcConfig.staconfig.auto_connect, g_wifiProvSrvcConfig.staconfig.ssid, g_wifiProvSrvcConfig.staconfig.psk, g_wifiProvSrvcConfig.staconfig.auth_type);
}

static void SYS_WIFIPROV_WriteConfig(void) 
{
    if (true == g_wifiProvSrvcConfig.save_config) 
    {
        SYS_WIFIPROV_SetTaskstatus(SYS_WIFIPROV_STATUS_NVM_ERASE);
    } 
    else 
    {
        SYS_WIFIPROV_CallBackFun(SYS_WIFIPROV_SETCONFIG, &g_wifiProvSrvcConfig, g_wifiProvSrvcCookie);
    }
}

static SYS_WIFIPROV_STATUS SYS_WIFIPROV_ExecuteBlock(SYS_MODULE_OBJ object) 
{
    SYS_WIFIPROV_OBJ *Obj = (SYS_WIFIPROV_OBJ *) object;
    switch (Obj->status) 
    {
       case SYS_WIFIPROV_STATUS_NVM_READ:
        {
            if (!NVM_IsBusy()) 
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
            if (!NVM_IsBusy()) 
            {
                SYS_WIFIPROV_NVMErase();
                Obj->status = SYS_WIFIPROV_STATUS_NVM_WRITE;
            }
            break;
        }
        case SYS_WIFIPROV_STATUS_NVM_WRITE:
        {
            if (!NVM_IsBusy()) 
            {
                SYS_WIFIPROV_NVMWrite();
                Obj->status = SYS_WIFIPROV_STATUS_WAITFORWRITE;
            }
            break;
        }
        case SYS_WIFIPROV_STATUS_WAITFORWRITE:
        {
            if (!NVM_IsBusy()) 
            {
                SYS_WIFIPROV_CallBackFun(SYS_WIFIPROV_SETCONFIG, &g_wifiProvSrvcConfig, g_wifiProvSrvcCookie);
                Obj->status = SYS_WIFIPROV_STATUS_WAITFORREQ;
            }
            break;
        }
        case SYS_WIFIPROV_STATUS_WAITFORREQ:
        default:
        {
            break;
        }
    }
    return Obj->status;
}

static bool SYS_WIFIPROV_ConfigValidate(SYS_WIFIPROV_CONFIG lWiFiProvConfig) 
{
    if(!((lWiFiProvConfig.mode == SYS_WIFIPROV_STA))) 
    {
        SYS_CONSOLE_MESSAGE(" set valid boot mode \r\n");
        return true;
    }    
    if (!((lWiFiProvConfig.save_config == true) || (lWiFiProvConfig.save_config == false))) 
    {
        SYS_CONSOLE_MESSAGE(" set valid save config value \r\n");
        return true;
    }
    if (SYS_WIFIPROV_STA == (SYS_WIFIPROV_MODE) lWiFiProvConfig.mode) 
    {
        if (!((lWiFiProvConfig.staconfig.channel >= 0) && (lWiFiProvConfig.staconfig.channel <= 13))) 
        {
            SYS_CONSOLE_MESSAGE(" set valid station mode channel number \r\n");
            return true;
        }
        if (!((lWiFiProvConfig.staconfig.auto_connect == true) || (lWiFiProvConfig.staconfig.auto_connect == false))) 
        {
            SYS_CONSOLE_MESSAGE(" set valid station mode Auto config value \r\n");
            return true;
        }
        if (!(((lWiFiProvConfig.staconfig.auth_type == SYS_WIFIPROV_OPEN) ||
             ((lWiFiProvConfig.staconfig.auth_type >= SYS_WIFIPROV_WPAWPA2MIXED) && (lWiFiProvConfig.staconfig.auth_type <= SYS_WIFIPROV_WPA3)))))  //ignore WEP as not support 
        {

            SYS_CONSOLE_MESSAGE(" set valid station mode Auth value \r\n");
            return true;
        }
        if ((lWiFiProvConfig.staconfig.auth_type >= SYS_WIFIPROV_WPAWPA2MIXED) && (lWiFiProvConfig.staconfig.auth_type <= SYS_WIFIPROV_WPA3)) 
        {
            if (strlen((const char *) lWiFiProvConfig.staconfig.psk) < 8) 
            {
                SYS_CONSOLE_MESSAGE(" set valid station mode passphase \r\n");
                return true;
            }
        }
    }
    return false;
}

static const SYS_CMD_DESCRIPTOR WiFiCmdTbl[] =
{
    {"wifiprov", (SYS_CMD_FNC) SYS_WIFIPROV_CMDProcess, ": WiFi provision commands processing"},
    {"wifiprovhelp", (SYS_CMD_FNC) SYS_WIFIPROV_CMDHelp, ": WiFi provision commands help "},
};

static bool SYS_WIFIPROV_CMDInit(void) 
{
    bool ret = SYS_WIFIPROV_SUCCESS;
    if (!SYS_CMD_ADDGRP(WiFiCmdTbl, sizeof (WiFiCmdTbl) / sizeof (*WiFiCmdTbl), "wifiprov", ": WiFi provision commands")) 
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

    if ((argc >= 7) && (!strcmp(argv[1], "set"))) 
    {
        if (SYS_WIFIPROV_STA == (SYS_WIFIPROV_MODE) strtol(argv[2], NULL, 0)) 
        {
            lWiFiProvConfig.mode = strtol(argv[2], NULL, 0);
            lWiFiProvConfig.save_config = strtol(argv[3], NULL, 0);
            if (strlen((const char *) argv[4]) <= sizeof (lWiFiProvConfig.countrycode)) 
            {
                parser_data((uint8_t *) argv[4], lWiFiProvConfig.countrycode, sizeof (lWiFiProvConfig.countrycode), val, 0);
            } 
            else
            {
                error = true;
            }
            lWiFiProvConfig.staconfig.channel = strtol(argv[5], NULL, 0);
            lWiFiProvConfig.staconfig.auto_connect = strtol(argv[6], NULL, 0);
            lWiFiProvConfig.staconfig.auth_type = strtol(argv[7], NULL, 0);
            if (strlen((const char *) argv[8]) <= sizeof (lWiFiProvConfig.staconfig.ssid)) 
            {
                parser_data((uint8_t *) argv[8], lWiFiProvConfig.staconfig.ssid, sizeof (lWiFiProvConfig.staconfig.ssid), val, 0);
            } 
            else 
            {
                error = true;
            }

            if (argc == 10) 
            {
                if (strlen((const char *) argv[9]) <= sizeof (lWiFiProvConfig.staconfig.psk)) 
                {
                    parser_data((uint8_t *) argv[9], lWiFiProvConfig.staconfig.psk, sizeof (lWiFiProvConfig.staconfig.psk), val, 0);
                } 
                else
                {
                    error = true;
                }
            } 
            else 
            {
                memset(lWiFiProvConfig.staconfig.psk, 0, sizeof (lWiFiProvConfig.staconfig.psk));
            }

            if ((!error) && (!SYS_WIFIPROV_ConfigValidate(lWiFiProvConfig))) 
            {
                g_wifiProvSrvcConfig.mode = lWiFiProvConfig.mode;
                g_wifiProvSrvcConfig.save_config = lWiFiProvConfig.save_config;
                memcpy(&g_wifiProvSrvcConfig.staconfig, &lWiFiProvConfig.staconfig, sizeof (SYS_WIFIPROV_STA_CONFIG));
                SYS_WIFIPROV_WriteConfig();
                //SYS_WIFIPROV_PrintConfig();
            } 
            else 
            {
                SYS_CONSOLE_PRINT(" Wrong Command\n");
            }
        }
    } 
    else if ((argc == 2) && (!strcmp(argv[1], "get"))) 
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
                        "                               5-Mixed mode(WPA2/WPA3) \r\n"
                        "                               6-WPA3 \r\n"
                        "ssid_name                    : SSID name \r\n"
                        "psk                          : Passphrase\r\n\r\n");
    SYS_CONSOLE_MESSAGE("Example STA Mode             : wifiprov set 0 1 \"GEN\" 1 1 1 \"DEMO_AP\" \"password\" \r\n");
    SYS_CONSOLE_MESSAGE("Example AP Mode              : wifiprov set 1 1 \"GEN\" 1 1 1 \"DEMO_SOFTAP\" \"password\" \r\n");
    SYS_CONSOLE_MESSAGE("wifiprov get                 : Get WiFi Provision Configuration \r\n");
    SYS_CONSOLE_MESSAGE("wifiprov debug level <value> : Set WiFi Provision Debug level value in hex \r\n");
    SYS_CONSOLE_MESSAGE("wifiprov debug flow <value>  : Set WiFi Provision Debug flow value in hex \r\n");
    return SYS_WIFIPROV_SUCCESS;
}

static uint8_t parser_data(uint8_t sbuff[], uint8_t dbuff[], uint8_t dbufflen, uint8_t val, uint8_t offset) 
{
    uint8_t idx1 = offset + 1, idx2 = offset, idx3 = 0;
    memset(dbuff, 0, dbufflen);

    for (; (sbuff[idx2] == val) && (sbuff[idx1] != val);) 
    {
        dbuff[idx3++] = sbuff[idx1++];
        /*SYS CMD service replacing space with NULL*/
        if (dbuff[idx3 - 1] == 0x00) 
        {
            dbuff[idx3 - 1] = 0x20;
        }
    }
    dbuff[idx3] = '\0';
    return idx3 + 1;
}


// *****************************************************************************
// *****************************************************************************
// Section:  SYS WIFI PROVISION Interface Functions
// *****************************************************************************
// *****************************************************************************
SYS_MODULE_OBJ SYS_WIFIPROV_Initialize(SYS_WIFIPROV_CONFIG *config, SYS_WIFIPROV_CALLBACK callback, void *cookie) 
{
    SYS_WIFIPROV_OBJ *wifiprovObj = (SYS_WIFIPROV_OBJ *) & g_wifiProvSrvcObj;
    g_wifiProvSrvcCallBack = callback;
    if (SYS_WIFIPROV_STATUS_NONE == SYS_WIFIPROV_GetTaskstatus()) 
    {
        SYS_WIFIPROV_SetCookie(cookie);
        SYS_WIFIPROV_InitConfig(config);
        if (SYS_WIFIPROV_FAILURE == SYS_WIFIPROV_CMDInit()) 
        {
            SYS_CONSOLE_MESSAGE("Failed to create WiFi provision Commands\r\n");
        }
        return (SYS_MODULE_OBJ) wifiprovObj;
    }
    return SYS_MODULE_OBJ_INVALID;
}

SYS_WIFIPROV_RESULT SYS_WIFIPROV_Deinitialize(SYS_MODULE_OBJ object) 
{
    uint32_t ret = SYS_WIFIPROV_FAILURE;
    if (&g_wifiProvSrvcObj != (SYS_WIFIPROV_OBJ *) object) 
    {
        ret = SYS_WIFIPROV_OBJ_INVALID;
    } 
    else
    {
        memset(&g_wifiProvSrvcObj, 0, sizeof (SYS_WIFIPROV_OBJ));
        memset(&g_wifiProvSrvcConfig, 0, sizeof (SYS_WIFIPROV_CONFIG));
        memset(&g_wifiProvSrvcConfigRead, 0, sizeof (SYS_WIFIPROV_CONFIG));
        SYS_WIFIPROV_SetTaskstatus(SYS_WIFIPROV_STATUS_NONE);
        ret = SYS_WIFIPROV_SUCCESS;
    }
    return ret;
}

uint8_t SYS_WIFIPROV_GetStatus(SYS_MODULE_OBJ object) 
{

    if (&g_wifiProvSrvcObj != (SYS_WIFIPROV_OBJ *) object) 
    {
        return SYS_WIFIPROV_OBJ_INVALID;
    } 
    else
    {
        return ((SYS_WIFIPROV_OBJ *) object)->status;
    }
}

uint8_t SYS_WIFIPROV_Tasks(SYS_MODULE_OBJ object) 
{

    if (&g_wifiProvSrvcObj != (SYS_WIFIPROV_OBJ *) object) 
    {
        return SYS_WIFIPROV_OBJ_INVALID;
    } 
    else
    {
        return SYS_WIFIPROV_ExecuteBlock(object);
    }
}

SYS_WIFIPROV_RESULT SYS_WIFIPROV_CtrlMsg(SYS_MODULE_OBJ object, uint32_t event, void *buffer, uint32_t length) 
{
    SYS_WIFIPROV_RESULT ret = SYS_WIFIPROV_SUCCESS;

    if (&g_wifiProvSrvcObj != (SYS_WIFIPROV_OBJ *) object) 
    {
        ret = SYS_WIFIPROV_OBJ_INVALID;
    }
    else
    {
        switch (event) 
        {
            case SYS_WIFIPROV_SETCONFIG:
            {
                if (buffer) 
                {
                    memcpy(&g_wifiProvSrvcConfig, (SYS_WIFIPROV_CONFIG *) buffer, sizeof (SYS_WIFIPROV_CONFIG));
                }
                SYS_WIFIPROV_WriteConfig();
                break;
            }
            case SYS_WIFIPROV_GETCONFIG:
            {
                SYS_WIFIPROV_CallBackFun(SYS_WIFIPROV_GETCONFIG, &g_wifiProvSrvcConfig, g_wifiProvSrvcCookie);
                break;
            }
            case SYS_WIFIPROV_CONNECT:
            {
                break;
            }
            default:
            {
                break;
            }
        }
    }
    return ret;
}
/* *****************************************************************************
 End of File
 */
