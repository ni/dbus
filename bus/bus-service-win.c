/* service-win32.c  dbus-daemon win32 service handling
 *
 * Copyright (C) 2006 Ralf Habacker
 *
 * Licensed under the Academic Free License version 2.1
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

// based on https://learn.microsoft.com/en-us/windows/win32/services/installing-a-service
// you can view the debug output with http://www.sysinternals.com/Utilities/DebugView.html

#include <stdio.h>

#include <config.h>
#include "bus.h"
#include "driver.h"
#include <dbus/dbus-internals.h>
#include <dbus/dbus-watch.h>
#include <stdlib.h>
#include <string.h>

#include <dbus/dbus-sysdeps-win.h>


#define SERVICE_NAME "dbus-daemon"
#define SERVICE_DISPLAY_NAME "DBus-Daemon"

SERVICE_STATUS          DBusServiceStatus;
SERVICE_STATUS_HANDLE   DBusServiceStatusHandle;

SC_HANDLE schSCManager;
SC_HANDLE schService;

VOID SvcDebugOut(LPSTR String, DWORD Status);
VOID  WINAPI DBusServiceCtrlHandler (DWORD opcode);

BOOL verbose = TRUE;

static BusContext *context;

static void
check_two_config_files (const DBusString *config_file,
                        const char       *extra_arg)
{
    if (_dbus_string_get_length (config_file) > 0)
    {
        SvcDebugOut(" [DBUS_SERVICE] check_two_config_files.\n", 0);
        exit (1);
    }
}

BOOL SetWindowRegisterKey(const char* key, const char* value)
{
    HKEY hKey;
    LONG result;

    // Open the registry key
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", 0, KEY_SET_VALUE, &hKey);
    if (result != ERROR_SUCCESS)
    {
        // Failed to open the key
        return FALSE;
    }

    // Set the value of the registry key
    result = RegSetValueEx(hKey, key, 0, REG_SZ, (const BYTE*)value, strlen(value) + 1);
    if (result != ERROR_SUCCESS)
    {
        // Failed to set the value
        RegCloseKey(hKey);
        return FALSE;
    }

    // Close the registry key
    RegCloseKey(hKey);

    return TRUE;
}

int _dbus_main_init(int argc, char **argv)
{
    DBusError error;
    DBusString config_file;
    DBusString address;
    DBusPipe print_addr_pipe;
    DBusPipe print_pid_pipe;
    BusContextFlags flags;
    void *ready_event_handle;
    char *addr_str;

    ready_event_handle = NULL;

    if (!_dbus_string_init (&config_file))
        return 1;

    if (!_dbus_string_init (&address))
        return 1;

    flags = BUS_CONTEXT_FLAG_WRITE_PID_FILE;
    // No syslog and no fork for session
    flags &= ~(BUS_CONTEXT_FLAG_SYSLOG_ALWAYS | BUS_CONTEXT_FLAG_FORK_ALWAYS);
    flags |= (BUS_CONTEXT_FLAG_SYSLOG_NEVER | BUS_CONTEXT_FLAG_FORK_NEVER);

    // Init for session
    check_two_config_files (&config_file, "session");

    if (!_dbus_get_session_config_file (&config_file))
    {
        SvcDebugOut(" [DBUS_SERVICE] No Session configuration file specified.\n", 0);
        return 2;
    }

    if (_dbus_string_get_length (&config_file) == 0)
    {
        SvcDebugOut(" [DBUS_SERVICE] No configuration file specified.\n", 0);
        return 3;
    }

    // init print address pipe
    _dbus_pipe_invalidate (&print_pid_pipe);
    _dbus_pipe_invalidate (&print_addr_pipe);

    // Init bux context
    dbus_error_init (&error);
    if (_dbus_string_get_length(&config_file) > 0)
    {
        addr_str = _dbus_string_get_data (&config_file);
        SvcDebugOut(" [DBUS_SERVICE] config_file : ", 0);
        SvcDebugOut(addr_str, 0);
    }
    else
    {
        SvcDebugOut(" [DBUS_SERVICE] config_file zero\n", 0);
    }
    context = bus_context_new (&config_file, flags,
                &print_addr_pipe, &print_pid_pipe, ready_event_handle,
                _dbus_string_get_length(&address) > 0 ? &address : NULL,
                &error);
    _dbus_string_free (&config_file);
    _dbus_string_free (&address);
    
    if (context == NULL)
    {
        SvcDebugOut(" [DBUS_SERVICE] Failed to start message bus\n", 0);
        dbus_error_free (&error);
        return 6;
    }
    addr_str = bus_context_get_address (context);
    SvcDebugOut(" [DBUS_SERVICE] Bus address : ", 0);
    SvcDebugOut(addr_str, 0);

    if (SetWindowRegisterKey("DBUS_SESSION_BUS_ADDRESS", addr_str) == FALSE)
    {
        SvcDebugOut(" [DBUS_SERVICE] Failed to set DBUS_SESSION_BUS_ADDRESS in registry\n", 0);
        return 7;
    }
    return 0;
}

// Open a handle to the SC Manager database.
BOOL OpenDataBase()
{
    schSCManager = OpenSCManager(
                       NULL,                    // local machine
                       NULL,                    // ServicesActive database
                       SC_MANAGER_ALL_ACCESS);  // full access rights

    if (schSCManager == NULL)
    {
        if  (verbose)
            printf("OpenSCManager failed (%d)\n", GetLastError());
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


BOOL InstallService()
{
    TCHAR szPath[MAX_PATH];


    if( !GetModuleFileName( NULL, szPath, MAX_PATH ) )
    {
        if  (verbose)
            printf("GetModuleFileName failed (%d)\n", GetLastError());
        return FALSE;
    }

    strcat(szPath," --run-service");

    if  (OpenDataBase() == FALSE)
    {
        return FALSE;
    }

    schService = CreateService(
                     schSCManager,              // SCManager database
                     TEXT(SERVICE_NAME),        // name of service
                     TEXT(SERVICE_DISPLAY_NAME),// service name to display
                     SERVICE_ALL_ACCESS,        // desired access
                     SERVICE_WIN32_OWN_PROCESS, // service type
                     SERVICE_AUTO_START,      // start type
                     SERVICE_ERROR_NORMAL,      // error control type
                     szPath,                    // path to service's binary
                     NULL,                      // no load ordering group
                     NULL,                      // no tag identifier
                     NULL,                      // no dependencies
                     NULL,                      // LocalSystem account
                     NULL);                     // no password

    if (schService == NULL)
    {
        if (verbose)
            printf("CreateService failed (%d)\n", GetLastError());
        return FALSE;
    }
    else
    {
        if (verbose)
            printf("Service %s installed\n", SERVICE_NAME);
        CloseServiceHandle(schService);
        return TRUE;
    }
}

BOOL RemoveService()
{
    if  (OpenDataBase() == FALSE)
    {
        return FALSE;
    }

    schService = OpenService(
                     schSCManager,       // SCManager database
                     TEXT(SERVICE_NAME), // name of service
                     DELETE);            // only need DELETE access

    if (schService == NULL)
    {
        if (verbose)
            printf("OpenService failed (%d)\n", GetLastError());
        return FALSE;
    }

    if (! DeleteService(schService) )
    {
        if (verbose)
            printf("DeleteService failed (%d)\n", GetLastError());
        return FALSE;
    }
    else
    {
        if (verbose)
            printf("Service %s removed\n",SERVICE_NAME);
        CloseServiceHandle(schService);
        return TRUE;
    }
}

// Stub initialization function.
DWORD DBusServiceInitialization(DWORD   argc, LPTSTR  *argv,
                                DWORD *specificError)
{
    DWORD status;
    specificError;
    

	status = _dbus_main_init(argc, argv); 

    SvcDebugOut(" [DBUS_SERVICE] Init Service\n",0);
    return status;
}

VOID WINAPI DBusServiceCtrlHandler (DWORD Opcode)
{
    DWORD status;

    switch(Opcode)
    {
    case SERVICE_CONTROL_PAUSE:
        // Do whatever it takes to pause here.
        DBusServiceStatus.dwCurrentState = SERVICE_PAUSED;
        SvcDebugOut(" [DBUS_SERVICE] Service paused\n",0);
        break;

    case SERVICE_CONTROL_CONTINUE:
        // Do whatever it takes to continue here.
        DBusServiceStatus.dwCurrentState = SERVICE_RUNNING;
        SvcDebugOut(" [DBUS_SERVICE] Service continued\n",0);
        break;

    case SERVICE_CONTROL_STOP:
        // Do whatever it takes to stop here.
        bus_context_shutdown (context);
        bus_context_unref (context);
        SvcDebugOut(" [DBUS_SERVICE] Service Stopped\n",0);
        DBusServiceStatus.dwWin32ExitCode = 0;
        DBusServiceStatus.dwCurrentState  = SERVICE_STOPPED;
        DBusServiceStatus.dwCheckPoint    = 0;
        DBusServiceStatus.dwWaitHint      = 0;

        if (!SetServiceStatus (DBusServiceStatusHandle,
                               &DBusServiceStatus))
        {
            status = GetLastError();
            SvcDebugOut(" [DBUS_SERVICE] SetServiceStatus error %ld\n",
                        status);
        }

        SvcDebugOut(" [DBUS_SERVICE] Leaving Service\n",0);
        return;

    case SERVICE_CONTROL_INTERROGATE:
        // Fall through to send current status.
        break;

    default:
        SvcDebugOut(" [DBUS_SERVICE] Unrecognized opcode %ld\n",
                    Opcode);
    }

    // Send current status.
    if (!SetServiceStatus (DBusServiceStatusHandle,  &DBusServiceStatus))
    {
        status = GetLastError();
        SvcDebugOut(" [DBUS_SERVICE] SetServiceStatus error %ld\n",
                    status);
    }
    return;
}

void WINAPI DBusServiceStart (DWORD argc, LPTSTR *argv)
{
    DWORD status;
    DWORD specificError;

    DBusServiceStatus.dwServiceType        = SERVICE_WIN32;
    DBusServiceStatus.dwCurrentState       = SERVICE_START_PENDING;
    DBusServiceStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
            SERVICE_ACCEPT_PAUSE_CONTINUE;
    DBusServiceStatus.dwWin32ExitCode      = 0;
    DBusServiceStatus.dwServiceSpecificExitCode = 0;
    DBusServiceStatus.dwCheckPoint         = 0;
    DBusServiceStatus.dwWaitHint           = 0;

    DBusServiceStatusHandle = RegisterServiceCtrlHandler(
                                  "DBusService",
                                  DBusServiceCtrlHandler);

    if (DBusServiceStatusHandle == (SERVICE_STATUS_HANDLE)0)
    {
        SvcDebugOut(" [DBUS_SERVICE] RegisterServiceCtrlHandler failed %d\n", GetLastError());
        return;
    }

    // Initialization code goes here.
    status = DBusServiceInitialization(argc, argv, &specificError);

    // Handle error condition
    if (status != NO_ERROR)
    {
        DBusServiceStatus.dwCurrentState       = SERVICE_STOPPED;
        DBusServiceStatus.dwCheckPoint         = 0;
        DBusServiceStatus.dwWaitHint           = 0;
        DBusServiceStatus.dwWin32ExitCode      = status;
        DBusServiceStatus.dwServiceSpecificExitCode = specificError;

        SetServiceStatus (DBusServiceStatusHandle, &DBusServiceStatus);
        return;
    }

    // Initialization complete - report running status.
    DBusServiceStatus.dwCurrentState       = SERVICE_RUNNING;
    DBusServiceStatus.dwCheckPoint         = 0;
    DBusServiceStatus.dwWaitHint           = 0;

    if (!SetServiceStatus (DBusServiceStatusHandle, &DBusServiceStatus))
    {
        status = GetLastError();
        SvcDebugOut(" [DBUS_SERVICE] SetServiceStatus error %ld\n",status);
    }

    // This is where the service does its work.
    SvcDebugOut(" [DBUS_SERVICE] Returning the Main Thread \n",0);

    _dbus_loop_run (bus_context_get_loop (context));

	return;
}

void RunService()
{
    SERVICE_TABLE_ENTRY   DispatchTable[] =
        {
            { "DBusService", DBusServiceStart      },
            { NULL,              NULL          }
        };

    if (!StartServiceCtrlDispatcher( DispatchTable))
    {
        SvcDebugOut(" [DBUS_SERVICE] StartServiceCtrlDispatcher (%d)\n",
                    GetLastError());
    }
}

int main(int argc, char **argv )
{

    if (argc >= 2)
    {
        // internal switch for running service
        if (strcmp(argv[1],"--run-service") == 0)
        {
            RunService();
            return 0;
        }
        else if(strcmp(argv[1],"--install-service") == 0)
        {
            InstallService();
            return 0;
        }
        else if(strcmp(argv[1],"--remove-service") == 0)
        {
            RemoveService();
            return 0;
        }
    }
    else
       RunService();

    return 0;
}

VOID SvcDebugOut(LPSTR String, DWORD Status)
{
    CHAR  Buffer[1024];
    if (strlen(String) < 1000)
    {
        sprintf(Buffer, String, Status);
        OutputDebugStringA(Buffer);
    }
}
