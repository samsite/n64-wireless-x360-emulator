#include "VigemWrapper.h"

#pragma comment(lib, "setupapi.lib")

std::mutex Vigem::sMutex;

PVIGEM_CLIENT Vigem::alloc()
{
    std::lock_guard<std::mutex> lock(sMutex);
    return vigem_alloc();
}

VIGEM_ERROR Vigem::connect(PVIGEM_CLIENT vigem)
{
    std::lock_guard<std::mutex> lock(sMutex);
    return vigem_connect(vigem);
}

void Vigem::disconnect(PVIGEM_CLIENT vigem)
{
    std::lock_guard<std::mutex> lock(sMutex);
    return vigem_disconnect(vigem);
}

void Vigem::free(PVIGEM_CLIENT vigem)
{
    std::lock_guard<std::mutex> lock(sMutex);
    return vigem_free(vigem);
}

VIGEM_ERROR Vigem::target_remove(PVIGEM_CLIENT vigem, PVIGEM_TARGET target)
{
    std::lock_guard<std::mutex> lock(sMutex);
    return vigem_target_remove(vigem, target);
}

void Vigem::target_free(PVIGEM_TARGET target)
{
    std::lock_guard<std::mutex> lock(sMutex);
    vigem_target_free(target);
}

PVIGEM_TARGET Vigem::target_x360_alloc()
{
    std::lock_guard<std::mutex> lock(sMutex);
    return vigem_target_x360_alloc();
}

VIGEM_ERROR Vigem::target_add(PVIGEM_CLIENT vigem, PVIGEM_TARGET target)
{
    std::lock_guard<std::mutex> lock(sMutex);
    return vigem_target_add(vigem, target);
}

VIGEM_ERROR Vigem::target_x360_update(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, XUSB_REPORT report)
{
    std::lock_guard<std::mutex> lock(sMutex);
    return vigem_target_x360_update(vigem, target, report);
}
