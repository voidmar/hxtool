#pragma once

#include <windows.h>

struct SerialPort
{
    HANDLE handle = INVALID_HANDLE_VALUE;

    ~SerialPort()
    {
        Close();
    }

    bool Open(const wchar_t* port)
    {
        handle = CreateFile(port,
            GENERIC_READ | GENERIC_WRITE,
            0,
            0,
            OPEN_EXISTING,
            0,
            0);

        if (handle == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        DCB dcb = { sizeof(dcb) };
        BuildCommDCB(L"19200,n,8,1", &dcb);
        dcb.fDtrControl = 0;
        dcb.fRtsControl = 0;
        SetCommState(handle, &dcb);

        COMMTIMEOUTS timeouts = { MAXDWORD, 0 };
        SetCommTimeouts(handle, &timeouts);

        SetCommMask(handle, EV_RXCHAR | EV_TXEMPTY);

        PurgeComm(handle, PURGE_RXABORT | PURGE_TXABORT);

        return true;
    }

    void Close()
    {
        if (handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(handle);
            handle = INVALID_HANDLE_VALUE;
        }
    }

    bool Is_Open() const
    {
        return handle != INVALID_HANDLE_VALUE;
    }
};
