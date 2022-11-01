#pragma once

#include <dinput.h>
#include <ViGEm/Client.h>

#include <memory>

class Controller;
typedef std::shared_ptr<Controller> ControllerPtr;

class Controller
{
public:
    ~Controller() = default;

    static ControllerPtr create(PVIGEM_CLIENT vigemClient, LPDIRECTINPUT8 dinput, GUID id);

private:
    Controller();
    bool init(PVIGEM_CLIENT vigemClient, LPDIRECTINPUT8 dinput, GUID id);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
