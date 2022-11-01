#include "ControllerDetector.h"
#include "Utils.h"
#include "VigemWrapper.h"
#include "DInputWrapper.h"

#include <csignal>
#include <string>
#include <iostream>
#include <unordered_map>

ControllerDetector detector;

void signalHandler(int)
{
    detector.stop();
}

int main(int argc, char** argv)
{
    signal(SIGINT, signalHandler);

    const auto client = Vigem::alloc();
    if (client == nullptr)
    {
        std::cout << "Unable to allocate vigem client" << std::endl;
        return -1;
    }

    const auto connectResult = Vigem::connect(client);
    if (!VIGEM_SUCCESS(connectResult))
    {
        std::cout << "ViGEm Bus connection failed with error code: 0x" << std::hex << connectResult << std::endl;
        return -1;
    }

    LPDIRECTINPUT8 dinput;
    if (DInput::Create(GetModuleHandle(0), DIRECTINPUT_VERSION, IID_IDirectInput8A, reinterpret_cast<LPVOID*>(&dinput), nullptr) != DI_OK)
    {
        std::cout << "Failed to create direct input interface" << std::endl;
        return -1;
    }

    std::unordered_map<std::string, ControllerPtr> controllers;

    detector.setControllerAddedCallback(
        [&](const std::string& id)
        {
            auto controller = Controller::create(client, dinput, Utils::StringToGuid(id));
            if (!controller)
            {
                std::cout << "Failed to create controller instance for " << id << std::endl;
                return;
            }
            controllers.insert({ id, controller });
            std::cout << "added:   " << id << std::endl;
        }
    );
    detector.setControllerRemovedCallback(
        [&](const std::string& id)
        {
            controllers.erase(id);
            std::cout << "removed: " << id << std::endl;
        }
    );

    detector.run(dinput, 500);

    // Cleanup DirectInput
    DInput::Release(dinput);
    dinput = nullptr;

    // Cleanup ViGEm
    Vigem::disconnect(client);
    Vigem::free(client);

    return 0;
}
