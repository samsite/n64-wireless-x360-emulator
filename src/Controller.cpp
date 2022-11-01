#include "Controller.h"
#include "Utils.h"
#include "VigemWrapper.h"
#include "DInputWrapper.h"

#include <thread>
#include <atomic>
#include <iostream>
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
//
//  Custom data format
//
///////////////////////////////////////////////////////////////////////////////

struct N64ControllerState
{
    BYTE data[20];
    LONG dpad;
    LONG yRotation;
    LONG xRotation;
    LONG yAxis;
    LONG xAxis;
    BYTE buttons[16];
};

enum N64Button : uint32_t
{
    B            = 0,
    A            = 1,
    C_UP         = 2,
    C_LEFT       = 3,
    LEFT_BUMPER  = 4,
    RIGHT_BUMPER = 5,
    Z            = 6,
    C_DOWN       = 7,
    C_RIGHT      = 8,
    START        = 9,
    ZR           = 10,
    HOME         = 12,
    CIRCLE       = 13
};

DIOBJECTDATAFORMAT g_aObjectFormats[] =
{
    { &GUID_POV, static_cast<DWORD>(FIELD_OFFSET(N64ControllerState, dpad)), DIDFT_POV | DIDFT_ANYINSTANCE, 0 },

    { &GUID_XAxis, static_cast<DWORD>(FIELD_OFFSET(N64ControllerState, xAxis)), DIDFT_ABSAXIS | DIDFT_ANYINSTANCE, 0 },
    { &GUID_YAxis, static_cast<DWORD>(FIELD_OFFSET(N64ControllerState, yAxis)), DIDFT_ABSAXIS | DIDFT_ANYINSTANCE, 0 },

    { &GUID_RxAxis, static_cast<DWORD>(FIELD_OFFSET(N64ControllerState, xRotation)), DIDFT_ABSAXIS | DIDFT_ANYINSTANCE, 0 },
    { &GUID_RyAxis, static_cast<DWORD>(FIELD_OFFSET(N64ControllerState, yRotation)), DIDFT_ABSAXIS | DIDFT_ANYINSTANCE, 0 },

    { 0, static_cast<DWORD>(FIELD_OFFSET(N64ControllerState, buttons[0])),  DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE, 0 },
    { 0, static_cast<DWORD>(FIELD_OFFSET(N64ControllerState, buttons[1])),  DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE, 0 },
    { 0, static_cast<DWORD>(FIELD_OFFSET(N64ControllerState, buttons[2])),  DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE, 0 },
    { 0, static_cast<DWORD>(FIELD_OFFSET(N64ControllerState, buttons[3])),  DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE, 0 },
    { 0, static_cast<DWORD>(FIELD_OFFSET(N64ControllerState, buttons[4])),  DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE, 0 },
    { 0, static_cast<DWORD>(FIELD_OFFSET(N64ControllerState, buttons[5])),  DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE, 0 },
    { 0, static_cast<DWORD>(FIELD_OFFSET(N64ControllerState, buttons[6])),  DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE, 0 },
    { 0, static_cast<DWORD>(FIELD_OFFSET(N64ControllerState, buttons[7])),  DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE, 0 },
    { 0, static_cast<DWORD>(FIELD_OFFSET(N64ControllerState, buttons[8])),  DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE, 0 },
    { 0, static_cast<DWORD>(FIELD_OFFSET(N64ControllerState, buttons[9])),  DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE, 0 },
    { 0, static_cast<DWORD>(FIELD_OFFSET(N64ControllerState, buttons[10])), DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE, 0 },
    { 0, static_cast<DWORD>(FIELD_OFFSET(N64ControllerState, buttons[11])), DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE, 0 },
    { 0, static_cast<DWORD>(FIELD_OFFSET(N64ControllerState, buttons[12])), DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE, 0 },
    { 0, static_cast<DWORD>(FIELD_OFFSET(N64ControllerState, buttons[13])), DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE, 0 },
    { 0, static_cast<DWORD>(FIELD_OFFSET(N64ControllerState, buttons[14])), DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE, 0 },
    { 0, static_cast<DWORD>(FIELD_OFFSET(N64ControllerState, buttons[15])), DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE, 0 },
};
#define numN64Objects (sizeof(g_aObjectFormats) / sizeof(DIOBJECTDATAFORMAT))

DIDATAFORMAT N64ControllerFormat =
{
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDF_ABSAXIS,
    sizeof(N64ControllerState),
    numN64Objects,
    g_aObjectFormats
};

///////////////////////////////////////////////////////////////////////////////
//
//  Impl
//
///////////////////////////////////////////////////////////////////////////////

struct Controller::Impl
{
    ~Impl();
    bool init(PVIGEM_CLIENT vigemClient, LPDIRECTINPUT8 dinput, GUID id);

    LPDIRECTINPUTDEVICE8A device_;
    HANDLE dataAvailableEvent_;
    std::thread thread_;
    std::atomic_bool threadRunning_{ true };
    PVIGEM_CLIENT vigemClient_;
    PVIGEM_TARGET vigemPad_;
};

Controller::Impl::~Impl()
{
    // Stop listening for events
    threadRunning_ = false;
    SetEvent(dataAvailableEvent_);
    CloseHandle(dataAvailableEvent_);
    thread_.join();

    // Close device
    DInput::DeviceUnacquire(device_);
    DInput::DeviceRelease(device_);
    device_ = nullptr;

    // Cleanup virtual pad
    Vigem::target_remove(vigemClient_, vigemPad_);
    Vigem::target_free(vigemPad_);
}

bool Controller::Impl::init(PVIGEM_CLIENT vigemClient, LPDIRECTINPUT8 dinput, GUID id)
{
    vigemClient_ = vigemClient;

    auto checkDeviceOp = [this](HRESULT hr) -> bool
    {
        if (hr == DI_OK)
            return true;
        DInput::DeviceRelease(device_);
        std::cout << "Error: " << Utils::ErrToString(hr) << std::endl;
        return false;
    };

    if (DInput::CreateDevice(dinput, id, &device_, nullptr) != DI_OK)
        return false;

    if (!checkDeviceOp(DInput::DeviceSetDataFormat(device_, &N64ControllerFormat)))
        return false;

    dataAvailableEvent_ = CreateEvent( 
        nullptr, // default security attributes
        false,   // automatically reset event
        false,   // initial state is nonsignaled
        nullptr  // object name
    );

    if (!checkDeviceOp(DInput::DeviceSetEventNotification(device_, dataAvailableEvent_)))
        return false;

    vigemPad_      = Vigem::target_x360_alloc();
    const auto pir = Vigem::target_add(vigemClient_, vigemPad_);
    if (!VIGEM_SUCCESS(pir))
    {
        std::cout << "Target plugin failed with error code: 0x" << std::hex << pir << std::endl;
        return false;
    }

    thread_ = std::thread(
        [this]()
        {
            HRESULT hr = DI_OK;
            N64ControllerState lastState;
            N64ControllerState state;

            XUSB_REPORT x360Report;
            XUSB_REPORT_INIT(&x360Report);

            while (threadRunning_)
            {
                auto waitResult = WaitForSingleObject( 
                    dataAvailableEvent_,
                    INFINITE
                );
                if (!threadRunning_)
                    break;

                if (waitResult != WAIT_OBJECT_0)
                {
                    std::cout << "error" << std::endl;
                    break;
                }

                if ((hr = DInput::DeviceGetDeviceState(device_, sizeof(N64ControllerState), &state)) != DI_OK)
                {
                    std::cout << "Failed to read device state: " << Utils::ErrToString(hr) << std::endl;
                    continue;
                }

                static constexpr LONG X_DEADZONE_START = 31700;
                static constexpr LONG X_DEADZONE_END   = 32000;
                static constexpr LONG Y_DEADZONE_START = 29600;
                static constexpr LONG Y_DEADZONE_END   = 29900;

                if (state.xAxis > X_DEADZONE_START && state.xAxis < X_DEADZONE_END)
                    state.xAxis = (X_DEADZONE_START + X_DEADZONE_END) / 2;
                if (state.yAxis > Y_DEADZONE_START && state.yAxis < Y_DEADZONE_END)
                    state.yAxis = (Y_DEADZONE_START + Y_DEADZONE_END) / 2;

                if (memcmp(&state, &lastState, sizeof(N64ControllerState)) == 0)
                    continue;

                static constexpr LONG X_MIN = 9800;
                static constexpr LONG X_MAX = 54300;
                static constexpr LONG Y_MIN = 6700;
                static constexpr LONG Y_MAX = 51800;

                auto convertAnalog = [](LONG value, LONG min, LONG max) -> SHORT
                {
                    return static_cast<SHORT>((std::min)(65535.0f, (std::max)(0L, value - min) / static_cast<float>(max - min) * 65535.0f) - 65535.0f/2.0f);
                };
                auto convertCButtonToAnalog = [](bool negative, bool positive) -> SHORT
                {
                    if (negative && positive)
                        return 0;
                    if (!negative && !positive)
                        return 0;
                    return negative ? -1 : 1;
                };
                auto normalizedCButtonVector = [](SHORT x, SHORT y) -> std::pair<SHORT,SHORT>
                {
                    auto mag = std::sqrt(static_cast<float>(x*x + y*y));
                    if (mag == 0)
                        return {0,0};
                    return {
                        static_cast<SHORT>(std::round(static_cast<float>(x) / mag * 32767)),
                        static_cast<SHORT>(std::round(static_cast<float>(y) / mag * 32767))
                    };
                };

                // NOTE: the remaining unbound xbox controller buttons are:
                // XUSB_GAMEPAD_LEFT_THUMB  = 0x0040,
                // XUSB_GAMEPAD_RIGHT_THUMB = 0x0080,
                // XUSB_GAMEPAD_Y           = 0x8000

                auto cButtonVector = normalizedCButtonVector(
                    convertCButtonToAnalog(state.buttons[N64Button::C_LEFT], state.buttons[N64Button::C_RIGHT]),
                    convertCButtonToAnalog(state.buttons[N64Button::C_DOWN], state.buttons[N64Button::C_UP])
                );

                x360Report.bLeftTrigger  = state.buttons[N64Button::Z] ? 255 : 0;
                x360Report.bRightTrigger = 0;
                x360Report.sThumbLX      = convertAnalog(state.xAxis, X_MIN, X_MAX);
                x360Report.sThumbLY      = -convertAnalog(state.yAxis, Y_MIN, Y_MAX);
                x360Report.sThumbRX      = cButtonVector.first;
                x360Report.sThumbRY      = cButtonVector.second;
                x360Report.wButtons =
                    (state.buttons[N64Button::A] ? XUSB_GAMEPAD_A : 0) |
                    (state.buttons[N64Button::B] ? XUSB_GAMEPAD_B : 0) |
                    (state.buttons[N64Button::ZR] ? XUSB_GAMEPAD_X : 0) |
                    (state.buttons[N64Button::LEFT_BUMPER] ? XUSB_GAMEPAD_LEFT_SHOULDER : 0) |
                    (state.buttons[N64Button::RIGHT_BUMPER] ? XUSB_GAMEPAD_RIGHT_SHOULDER : 0) |
                    (state.buttons[N64Button::START] ? XUSB_GAMEPAD_START : 0) |
                    (state.buttons[N64Button::HOME] ? XUSB_GAMEPAD_GUIDE : 0) |
                    (state.buttons[N64Button::CIRCLE] ? XUSB_GAMEPAD_BACK : 0) |
                    (state.dpad == 0     ? XUSB_GAMEPAD_DPAD_UP : 0) |
                    (state.dpad == 4500  ? XUSB_GAMEPAD_DPAD_UP | XUSB_GAMEPAD_DPAD_RIGHT : 0) |
                    (state.dpad == 9000  ? XUSB_GAMEPAD_DPAD_RIGHT : 0) |
                    (state.dpad == 13500 ? XUSB_GAMEPAD_DPAD_RIGHT | XUSB_GAMEPAD_DPAD_DOWN : 0) |
                    (state.dpad == 18000 ? XUSB_GAMEPAD_DPAD_DOWN : 0) |
                    (state.dpad == 22500 ? XUSB_GAMEPAD_DPAD_DOWN | XUSB_GAMEPAD_DPAD_LEFT : 0) |
                    (state.dpad == 27000 ? XUSB_GAMEPAD_DPAD_LEFT : 0) |
                    (state.dpad == 31500 ? XUSB_GAMEPAD_DPAD_LEFT | XUSB_GAMEPAD_DPAD_UP : 0);

                Vigem::target_x360_update(vigemClient_, vigemPad_, x360Report);

                lastState = state;
            }
        }
    );

    if (!checkDeviceOp(DInput::DeviceAcquire(device_)))
        return false;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Public interface
//
///////////////////////////////////////////////////////////////////////////////

Controller::Controller()
    : impl_(new Impl)
{   }

ControllerPtr Controller::create(PVIGEM_CLIENT vigemClient, LPDIRECTINPUT8 dinput, GUID id)
{
    auto controller = new Controller();
    if (!controller->init(vigemClient, dinput, id))
        return nullptr;
    return ControllerPtr(controller);
}

bool Controller::init(PVIGEM_CLIENT vigemClient, LPDIRECTINPUT8 dinput, GUID id)
{
    return impl_->init(vigemClient, dinput, id);
}
