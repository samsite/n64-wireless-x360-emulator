#include "ControllerDetector.h"
#include "Utils.h"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include <iostream>
#include <unordered_set>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <chrono>

using namespace std::chrono_literals;

struct ControllerDetector::Impl
{
    Callback                addedCallback;
    Callback                removedCallback;
    std::mutex              mutex;
    std::condition_variable cvar;
    bool                    running { false };

    ~Impl();
    void run(LPDIRECTINPUT8 dinput, uint32_t msPollInterval);
    void stop();
};

ControllerDetector::Impl::~Impl()
{
    stop();
}

BOOL CALLBACK enumerateDevice(LPCDIDEVICEINSTANCE device, LPVOID pvRef)
{
    // Skip controllers that aren't wireless n64 controllers
    if (Utils::GuidToString(device->guidProduct) != "2019057e-0000-0000-0000-504944564944")
        return DIENUM_CONTINUE;

    reinterpret_cast<std::vector<std::string>*>(pvRef)->push_back(Utils::GuidToString(device->guidInstance));
    return DIENUM_CONTINUE;
}

void ControllerDetector::Impl::run(LPDIRECTINPUT8 dinput, uint32_t msPollInterval)
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        running = true;
    }

    std::vector<std::string> currentDeviceIDs;
    while (running)
    {
        std::vector<std::string> deviceIDs;
        dinput->EnumDevices(DI8DEVCLASS_GAMECTRL, enumerateDevice, &deviceIDs, DIEDFL_ATTACHEDONLY);

        std::vector<std::string> addedIDs;
        std::set_difference(deviceIDs.begin(), deviceIDs.end(), currentDeviceIDs.begin(), currentDeviceIDs.end(), std::inserter(addedIDs, addedIDs.begin()));

        std::vector<std::string> removedIDs;
        std::set_difference(currentDeviceIDs.begin(), currentDeviceIDs.end(), deviceIDs.begin(), deviceIDs.end(), std::inserter(removedIDs, removedIDs.begin()));

        if (addedCallback)
            for (const auto& id : addedIDs)
                addedCallback(id);
        if (removedCallback)
            for (const auto& id : removedIDs)
                removedCallback(id);

        // Update current device IDs
        currentDeviceIDs = deviceIDs;

        // Block for poll interval, or until we quit
        std::unique_lock<std::mutex> lock(mutex);
        cvar.wait_for(lock, 1ms * msPollInterval, [this]{ return !running; });
    }

    // Call removed callback for all ids
    if (removedCallback)
        for (const auto& id : currentDeviceIDs)
            removedCallback(id);
}

void ControllerDetector::Impl::stop()
{
    std::lock_guard<std::mutex> lock(mutex);
    running = false;
}

/////////////////////////////////////////////////////////////////////

ControllerDetector::ControllerDetector()
    : impl_(new Impl)
{   }

ControllerDetector::~ControllerDetector() = default;

void ControllerDetector::run(LPDIRECTINPUT8 dinput, uint32_t msPollInterval)
{
    return impl_->run(dinput, msPollInterval);
}

void ControllerDetector::stop()
{
    impl_->stop();
}

void ControllerDetector::setControllerAddedCallback(const Callback& callback)
{
    impl_->addedCallback = callback;
}

void ControllerDetector::setControllerRemovedCallback(const Callback& callback)
{
    impl_->removedCallback = callback;
}
