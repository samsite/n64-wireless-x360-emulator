#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include <mutex>

class DInput
{
public:
    static HRESULT Create(
        HINSTANCE hinst,
        DWORD dwVersion,
        REFIID riidltf,
        LPVOID* ppvOut,
        LPUNKNOWN pUnkOuter
    );
    static HRESULT Release(LPDIRECTINPUT8 dinput);

    static HRESULT CreateDevice(LPDIRECTINPUT8 dinput, REFGUID rguid, LPDIRECTINPUTDEVICE8A* lplpDirectInputDevice, LPUNKNOWN pUnkOuter);
    static HRESULT DeviceSetDataFormat(LPDIRECTINPUTDEVICE8A device, LPCDIDATAFORMAT lpdf);
    static HRESULT DeviceSetEventNotification(LPDIRECTINPUTDEVICE8A device, HANDLE hEvent);
    static HRESULT DeviceGetDeviceState(LPDIRECTINPUTDEVICE8A device, DWORD cbData, LPVOID lpvData);
    static HRESULT DeviceAcquire(LPDIRECTINPUTDEVICE8A device);
    static HRESULT DeviceUnacquire(LPDIRECTINPUTDEVICE8A device);
    static HRESULT DeviceRelease(LPDIRECTINPUTDEVICE8A device);

private:
    static std::mutex sMutex;
};