#include "DInputWrapper.h"

std::mutex DInput::sMutex;

HRESULT DInput::Create(
        HINSTANCE hinst,
        DWORD dwVersion,
        REFIID riidltf,
        LPVOID* ppvOut,
        LPUNKNOWN pUnkOuter
    )
{
    std::lock_guard<std::mutex> lock(sMutex);
    return DirectInput8Create(hinst, dwVersion, riidltf, ppvOut, pUnkOuter);
}

HRESULT DInput::Release(LPDIRECTINPUT8 dinput)
{
    std::lock_guard<std::mutex> lock(sMutex);
    return dinput->Release();
}

HRESULT DInput::CreateDevice(LPDIRECTINPUT8 dinput, REFGUID rguid, LPDIRECTINPUTDEVICE8A* lplpDirectInputDevice, LPUNKNOWN pUnkOuter)
{
    std::lock_guard<std::mutex> lock(sMutex);
    return dinput->CreateDevice(rguid, lplpDirectInputDevice, pUnkOuter);
}

HRESULT DInput::DeviceSetDataFormat(LPDIRECTINPUTDEVICE8A device, LPCDIDATAFORMAT lpdf)
{
    std::lock_guard<std::mutex> lock(sMutex);
    return device->SetDataFormat(lpdf);
}

HRESULT DInput::DeviceSetEventNotification(LPDIRECTINPUTDEVICE8A device, HANDLE hEvent)
{
    std::lock_guard<std::mutex> lock(sMutex);
    return device->SetEventNotification(hEvent);
}

HRESULT DInput::DeviceGetDeviceState(LPDIRECTINPUTDEVICE8A device, DWORD cbData, LPVOID lpvData)
{
    std::lock_guard<std::mutex> lock(sMutex);
    return device->GetDeviceState(cbData, lpvData);
}

HRESULT DInput::DeviceAcquire(LPDIRECTINPUTDEVICE8A device)
{
    std::lock_guard<std::mutex> lock(sMutex);
    return device->Acquire();
}

HRESULT DInput::DeviceUnacquire(LPDIRECTINPUTDEVICE8A device)
{
    std::lock_guard<std::mutex> lock(sMutex);
    return device->Unacquire();
}

HRESULT DInput::DeviceRelease(LPDIRECTINPUTDEVICE8A device)
{
    std::lock_guard<std::mutex> lock(sMutex);
    return device->Release();
}
