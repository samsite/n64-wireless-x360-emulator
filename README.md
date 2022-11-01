# What is this

This is a command line utility that sets up virtual Xbox 360 controllers for each connected official wireless Nintendo N64 controller.

# Clone

Make sure to `git submodule update --init` to get the `ViGEmClient` submodule.

# Install Dependencies

 - Install [Windows SDK](https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/)
 - Install [ViGEmBus](https://github.com/ViGEm/ViGEmBus/releases/latest)
 - Install [HidHide](https://github.com/ViGEm/HidHide/releases/latest)

Reboot your computer after installing these.

# Build

Standard CMake workflow:
```
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

# Configure HidHide

 - Run `HidHide Configuration Client`
 - Add the path of `n64-controller.exe` to the `Applications` list
 - Select `Devices` tab
 - Make sure `Enable device hiding` is enabled
 - Do the following for each controller (one time only):
   - Connect controller
   - Check the box next to `Nintendo Wireless Gamepad`
   - Disconnect and connect the controller again

# Run

```
cd Release
n64-controller.exe
```

# TODO

 - System tray app instead of a CLI app
 - Config file / log file
