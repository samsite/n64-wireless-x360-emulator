#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <ViGEm/Client.h>

#include <mutex>

class Vigem
{
public:
    static PVIGEM_CLIENT alloc();
    static VIGEM_ERROR   connect(PVIGEM_CLIENT vigem);
    static void          disconnect(PVIGEM_CLIENT vigem);
    static void          free(PVIGEM_CLIENT vigem);

    static VIGEM_ERROR   target_remove(PVIGEM_CLIENT vigem, PVIGEM_TARGET target);
    static void          target_free(PVIGEM_TARGET target);
    static PVIGEM_TARGET target_x360_alloc();
    static VIGEM_ERROR   target_add(PVIGEM_CLIENT vigem, PVIGEM_TARGET target);
    static VIGEM_ERROR   target_x360_update(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, XUSB_REPORT report);

private:
    static std::mutex sMutex;
};