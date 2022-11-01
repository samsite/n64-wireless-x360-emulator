#pragma once

#include "Controller.h"

#include <memory>
#include <cstdint>
#include <functional>
#include <string>

class ControllerDetector
{
public:
    using Callback = std::function<void(const std::string&)>;

public:
    ControllerDetector();
    ~ControllerDetector();

    void run(LPDIRECTINPUT8 dinput, uint32_t msPollInterval);
    void stop();
    void setControllerAddedCallback(const Callback& callback);
    void setControllerRemovedCallback(const Callback& callback);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
