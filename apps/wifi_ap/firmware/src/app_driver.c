/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_driver.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include "app_driver.h"
#include "wdrv_pic32mzw_client_api.h"
#include "tcpip/tcpip_manager.h"
#include "tcpip/src/tcpip_manager_control.h"
#include "tcpip/dhcps.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

typedef struct wifiConfiguration 
{
    WDRV_PIC32MZW_AUTH_CONTEXT authCtx;
    WDRV_PIC32MZW_BSS_CONTEXT bssCtx;
} wifiConfig;

SYS_TIME_HANDLE timeHandle;
static wifiConfig g_wifiConfig;

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_DRIVER_Initialize function.

    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************

static void APP_TimerCallback(uintptr_t context)
{
    APP_APShowConnectedDevices();
}

static void APP_APNotifyCallback(DRV_HANDLE handle, WDRV_PIC32MZW_ASSOC_HANDLE associationHandle, WDRV_PIC32MZW_CONN_STATE currentState)
{
    WDRV_PIC32MZW_MAC_ADDR macAddr;
    WDRV_PIC32MZW_AssocPeerAddressGet(associationHandle, &macAddr);
    
    if (WDRV_PIC32MZW_CONN_STATE_CONNECTED == currentState)
    {
        timeHandle = SYS_TIME_CallbackRegisterMS(APP_TimerCallback, (uintptr_t)0, 500, SYS_TIME_SINGLE);
        SYS_CONSOLE_PRINT("Wifi State :: CONNECTED :: %02X:%02X:%02X:%02X:%02X:%02X\r\n", macAddr.addr[0], macAddr.addr[1], macAddr.addr[2], macAddr.addr[3], macAddr.addr[4], macAddr.addr[5]);
    }
    else if (WDRV_PIC32MZW_CONN_STATE_DISCONNECTED == currentState)
    {
        SYS_CONSOLE_PRINT("Wifi State :: DISCONNECTED :: %02X:%02X:%02X:%02X:%02X:%02X\r\n", macAddr.addr[0], macAddr.addr[1], macAddr.addr[2], macAddr.addr[3], macAddr.addr[4], macAddr.addr[5]);
        TCPIP_DHCPS_LeaseEntryRemove(appData.netHandle, (TCPIP_MAC_ADDR*) macAddr.addr);
    }
}

bool APP_APStart()
{
    if (WDRV_PIC32MZW_STATUS_OK == WDRV_PIC32MZW_APStart(appData.wdrvHandle, &g_wifiConfig.bssCtx, &g_wifiConfig.authCtx, &APP_APNotifyCallback))
    {
        SYS_CONSOLE_MESSAGE("APP: AP started\r\n");
        appData.isApServiceStarted = true;
        app_controlData.isAPServiceSuspended = false;
        appData.state = APP_WIFI_IDLE;
        return true;
    }
    else
    {
        appData.state = APP_WIFI_ERROR;
        return false;
    }
}

bool APP_APStop()
{
    if(appData.isApServiceStarted == true)
    {
        if (WDRV_PIC32MZW_STATUS_OK == WDRV_PIC32MZW_APStop(appData.wdrvHandle))
        {
            SYS_CONSOLE_MESSAGE("APP: AP Stop\r\n");
            appData.isApServiceStarted = false;
            app_controlData.isAPServiceSuspended = true;
            return true;
        }
    }
    
    return false;
}

void APP_APShowConnectedDevices()
{
    int numDhcpsPoolEntries;
    numDhcpsPoolEntries = TCPIP_DHCPS_GetPoolEntries(appData.netHandle, DHCP_SERVER_POOL_ENTRY_IN_USE);

    if (numDhcpsPoolEntries > 0)
    {
        TCPIP_DHCPS_LEASE_HANDLE dhcpsLease = 0;
        TCPIP_DHCPS_LEASE_ENTRY dhcpsLeaseEntry;
        SYS_CONSOLE_MESSAGE("--------------------------------------------\r\n");
        SYS_CONSOLE_MESSAGE("           Connected devices summary\r\n");
        SYS_CONSOLE_MESSAGE("--------------------------------------------\r\n\r\n");
        SYS_CONSOLE_MESSAGE("MAC Address              IP Address\r\n");

        do
        {
            dhcpsLease = TCPIP_DHCPS_LeaseEntryGet(appData.netHandle, &dhcpsLeaseEntry, dhcpsLease);

            if (NULL != dhcpsLease)
            {
                char ipAddr[20];
                char macAddr[20];
                
                TCPIP_Helper_MACAddressToString(&dhcpsLeaseEntry.hwAdd, macAddr, sizeof(macAddr));
                TCPIP_Helper_IPAddressToString(&dhcpsLeaseEntry.ipAddress, ipAddr, sizeof(ipAddr));

                SYS_CONSOLE_PRINT("%s", macAddr);
                SYS_CONSOLE_PRINT("        %s\r\n", ipAddr);
            }
        }
        while (NULL != dhcpsLease);
    }
    else
    {
        SYS_CONSOLE_MESSAGE("No device is connected\r\n");
    }
}

static bool APP_WIFI_Config() 
{    
    WIFI_AUTH_MODE authMode = (WIFI_AUTH_MODE)app_controlData.wlanConfig.authMode;
    uint8_t *ssid           = (uint8_t *)app_controlData.wlanConfig.ssid;
    uint8_t ssidLength      = app_controlData.wlanConfig.ssidLength;
    uint8_t *password       = (uint8_t *)app_controlData.wlanConfig.password;
    uint8_t passwordLength  = strlen((char *)password);

    SYS_CONSOLE_PRINT("APP: SSID is %.*s \r\n",ssidLength, ssid);
    if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_BSSCtxSetSSID(&g_wifiConfig.bssCtx, ssid, ssidLength)) 
    {
        SYS_CONSOLE_PRINT("SSID set fail \r\n");
        return false;
    }
    
    if(WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_BSSCtxSetChannel(&g_wifiConfig.bssCtx, app_controlData.wlanConfig.channel))
    {
        SYS_CONSOLE_PRINT("channel %d \r\n", app_controlData.wlanConfig.channel);
        return false;
    }
    
    if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_BSSCtxSetSSIDVisibility(&g_wifiConfig.bssCtx, 1))
    {
        SYS_CONSOLE_MESSAGE("Set visibility fail \r\n");
        return false;
    }

    
    switch (authMode) 
    {
        case OPEN:
        {
            if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_AuthCtxSetOpen(&g_wifiConfig.authCtx)) 
            {
                SYS_CONSOLE_MESSAGE("APP: Unable to set Authentication\r\n");
                return false;
            }
            break;
        }
        case WPA2:
        {
            if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_AuthCtxSetPersonal(&g_wifiConfig.authCtx, (uint8_t *)password, passwordLength, WDRV_PIC32MZW_AUTH_TYPE_WPA2_PERSONAL)) 
            {
                SYS_CONSOLE_MESSAGE("APP: Unable to set authentication to WPA2\r\n");
                return false;
            }
            break;
        }
        case WPAWPA2MIXED:
        {
            if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_AuthCtxSetPersonal(&g_wifiConfig.authCtx, password, passwordLength, WDRV_PIC32MZW_AUTH_TYPE_WPAWPA2_PERSONAL)) 
            {
                SYS_CONSOLE_MESSAGE("APP: Unable to set authentication to WPAWPA2 MIXED\r\n");
                return false;
            }
            break;
        }
#ifdef WDRV_PIC32MZW_WPA3_SUPPORT
        case WPA3:
        {
            if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_AuthCtxSetPersonal(&g_wifiConfig.authCtx, password, passwordLength, WDRV_PIC32MZW_AUTH_TYPE_WPA3_PERSONAL)) 
            {
                SYS_CONSOLE_MESSAGE("APP: Unable to set authentication to WPA3 \r\n");
                return false;
            }
            break;
        }
        case WPA2WPA3MIXED:
        {
            if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_AuthCtxSetPersonal(&g_wifiConfig.authCtx, password, passwordLength, WDRV_PIC32MZW_AUTH_TYPE_WPA2WPA3_PERSONAL)) 
            {
                SYS_CONSOLE_MESSAGE("APP: Unable to set authentication to WPA2WPA3 MIXED \r\n");
                return false;
            }
            break;
        }
#endif
        case WEP:
        {
            uint8_t *wepKey        = app_controlData.wlanConfig.wepKey;
            uint8_t wepKeyLength  = strlen((char *)app_controlData.wlanConfig.wepKey);
            if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_AuthCtxSetWEP(&g_wifiConfig.authCtx, app_controlData.wlanConfig.wepIdx, wepKey, wepKeyLength))
            {
                SYS_CONSOLE_MESSAGE("APP: Unable to set authentication to WEP \r\n");
                return false;
            }
            if (WDRV_PIC32MZW_STATUS_OK != WDRV_PIC32MZW_AuthCtxSharedKey(&g_wifiConfig.authCtx, true))
            {
                SYS_CONSOLE_MESSAGE("APP: Unable to Enable shared key authentication \r\n");
                return false;
            }
            break;
        }
        default:
        {
            SYS_CONSOLE_PRINT("APP: Unknown Authentication type\r\n");
            return false;
        }
    }

    if (false == TCPIP_DHCPS_Enable(appData.netHandle))
    {
        return false;
    }
    return true;
}



// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_DRIVER_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_DRIVER_Initialize ( void )
{
    appData.state                   = APP_STATE_INIT;
    appData.wdrvHandle              = DRV_HANDLE_INVALID;
    appData.netHandle               = TCPIP_STACK_NetHandleGet(TCPIP_NETWORK_DEFAULT_INTERFACE_NAME_IDX0);
    
    WDRV_PIC32MZW_BSSCtxSetDefaults(&g_wifiConfig.bssCtx);
    WDRV_PIC32MZW_AuthCtxSetDefaults(&g_wifiConfig.authCtx);
    
    SYS_CONSOLE_MESSAGE("APP: Initialization Successful\r\n");
}


/******************************************************************************
  Function:
    void APP_DRIVER_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_DRIVER_Tasks ( void )
{
    SYS_STATUS tcpipStat;
    bool status;

    /* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APP_STATE_INIT:
        {
            if (SYS_STATUS_READY == WDRV_PIC32MZW_Status(sysObj.drvWifiPIC32MZW1))
            {
                appData.state = APP_STATE_WDRV_INIT_READY;
            }
            break;
        }
        
        case APP_STATE_WDRV_INIT_READY:
        {
            appData.wdrvHandle = WDRV_PIC32MZW_Open(0, 0);
            if (DRV_HANDLE_INVALID != appData.wdrvHandle) 
            {
                appData.state = APP_TCPIP_WAIT_FOR_TCPIP_INIT;
            }
            break;
        }

        case APP_TCPIP_WAIT_FOR_TCPIP_INIT:
        {
            tcpipStat = TCPIP_STACK_Status(sysObj.tcpip);

            if (tcpipStat < 0)
            {
                SYS_CONSOLE_MESSAGE( "TCP/IP stack initialization failed!\r\n" );
                appData.state = APP_TCPIP_ERROR;
            }
            else if(SYS_STATUS_READY == tcpipStat)
            {
                appData.state = APP_WIFI_CONFIG;
            }
            break;
        }
        
        case APP_WIFI_CONFIG:
        {
            if(app_controlData.wlanConfigValid) 
            {
                status = APP_WIFI_Config();
                
                if(status)
                {
                    appData.state = APP_WIFI_AP_START;
                }
                
                app_controlData.wlanConfigValid = 0;
            }
            else
            {
                appData.state = APP_WIFI_IDLE;
            }
            
            break;
        }
        
        case APP_WIFI_AP_START:
        {
            APP_APStart();
            break;
        }
        
        case APP_WIFI_IDLE:
        {
            if(app_controlData.wlanConfigChanged) 
            {
                appData.state = APP_WIFI_CONFIG;
                app_controlData.wlanConfigChanged = false;
            }
            break;
        }
        
        case APP_WIFI_ERROR:
        {
            break;
        }
        
        case APP_TCPIP_ERROR:
        {
            break;
        }
        
        default:
        {
            break;
        }
    }
}


/*******************************************************************************
 End of File
 */
