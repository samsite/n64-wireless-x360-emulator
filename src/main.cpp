#include <iostream>

#define DIRECTINPUT_VERSION 0x0800

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <dinput.h>
#include <comdef.h>
#include <atlstr.h>

#include <ViGEm/Client.h>
#pragma comment(lib, "setupapi.lib")

#include <vector>
#include <thread>
#include <atomic>
#include <unordered_set>
#include <algorithm>
#include <csignal>
#include <mutex>
#include <condition_variable>

using namespace std::chrono_literals;

std::string errToString(HRESULT hr)
{
	_com_error err(hr);
	return std::string(CT2A(err.ErrorMessage()));
}

// Here we define a custom data format to store input from a mouse. In a 
// real program you would almost certainly use either the predefined 
// DIMOUSESTATE or DIMOUSESTATE2 structure to store mouse input, but some 
// input devices such as the Sidewinder GameVoice controller are not well
// described by the provided types and may require custom formats.

// Structure must be DWORD multiple in size
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
	B = 0,
	A = 1,
	C_UP = 2,
	C_LEFT = 3,
	LEFT_BUMPER = 4,
	RIGHT_BUMPER = 5,
	Z = 6,
	C_DOWN = 7,
	C_RIGHT = 8,
	START = 9,
	ZR = 10,
	HOME = 12,
	CIRCLE = 13
};

void printState(N64ControllerState& state)
{
	std::cout << "--" << std::endl;
	std::cout << "D-PAD:        " << state.dpad << std::endl;
	std::cout << "X/Y:          " << state.xAxis << ", " << state.yAxis << std::endl;
	std::cout << "X/Y Rot:      " << state.xRotation << ", " << state.yRotation << std::endl;
	std::cout << "B:            " << (state.buttons[N64Button::B]  ? "PRESS" : "") << std::endl;
	std::cout << "A:            " << (state.buttons[N64Button::A]  ? "PRESS" : "") << std::endl;
	std::cout << "C-UP:         " << (state.buttons[2]  ? "PRESS" : "") << std::endl;
	std::cout << "C-LEFT:       " << (state.buttons[3]  ? "PRESS" : "") << std::endl;
	std::cout << "LEFT BUMPER:  " << (state.buttons[4]  ? "PRESS" : "") << std::endl;
	std::cout << "RIGHT BUMPER: " << (state.buttons[5]  ? "PRESS" : "") << std::endl;
	std::cout << "Z:            " << (state.buttons[6]  ? "PRESS" : "") << std::endl;
	std::cout << "C-DOWN:       " << (state.buttons[7]  ? "PRESS" : "") << std::endl;
	std::cout << "C-RIGHT:      " << (state.buttons[8]  ? "PRESS" : "") << std::endl;
	std::cout << "START:        " << (state.buttons[9]  ? "PRESS" : "") << std::endl;
	std::cout << "ZR:           " << (state.buttons[10] ? "PRESS" : "") << std::endl;
	std::cout << "10:           " << (state.buttons[11] ? "PRESS" : "") << std::endl;
	std::cout << "HOME:         " << (state.buttons[12] ? "PRESS" : "") << std::endl;
	std::cout << "CIRCLE:       " << (state.buttons[13] ? "PRESS" : "") << std::endl;
	std::cout << "13:           " << (state.buttons[14] ? "PRESS" : "") << std::endl;
}

// Each device object for which you want to receive input must have an entry
// in this DIOBJECTDATAFORMAT array which is stored in the custom DIDATAFORMAT.
// The DIOBJECTDATAFORMAT maps detected device object to a particular offset
// within MouseState structure declared above. Inside the input routine, a
// MouseState structure is provided to the GetDeviceState method, and
// DirectInput uses this offset to store the input data in the provided
// structure. 
// 
// Any of the elements which are not flagged as DIDFT_OPTIONAL, and
// which describe a device object which is not found on the actual device will
// cause the SetDeviceFormat call to fail. For the format defined below, the
// system mouse must have an x-axis, y-axis, and at least one button. 

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

// Finally, the DIDATAFORMAT is filled with the information defined above for 
// our custom data format. The format also defines whether the returned axis 
// data is absolute or relative. Usually mouse movement is reported in relative 
// coordinates, but our custom format will use absolute coordinates. 

DIDATAFORMAT N64ControllerFormat =
{
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDF_ABSAXIS, // TODO: is this right?
    sizeof(N64ControllerState),
    numN64Objects,
    g_aObjectFormats
};

GUID StringToGuid(const std::string& str)
{
	GUID guid;
	sscanf(str.c_str(),
	       "%8x-%4hx-%4hx-%2hhx%2hhx-%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx",
	       &guid.Data1, &guid.Data2, &guid.Data3,
	       &guid.Data4[0], &guid.Data4[1], &guid.Data4[2], &guid.Data4[3],
	       &guid.Data4[4], &guid.Data4[5], &guid.Data4[6], &guid.Data4[7] );
	return guid;
}

std::string GuidToString(GUID guid)
{
	char guid_cstr[39];
	snprintf(guid_cstr, sizeof(guid_cstr),
	         "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
	         guid.Data1, guid.Data2, guid.Data3,
	         guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
	         guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
	return std::string(guid_cstr);
}

BOOL CALLBACK enumerateDevice(LPCDIDEVICEINSTANCE device, LPVOID pvRef)
{
	// Skip controllers that aren't wireless n64 controllers
	if (GuidToString(device->guidProduct) != "2019057e-0000-0000-0000-504944564944")
		return DIENUM_CONTINUE;

	auto deviceIDs = reinterpret_cast<std::vector<GUID>*>(pvRef);
	deviceIDs->push_back(device->guidInstance);

	std::cout << "--" << std::endl;
	std::cout << "guidInstance:    " << GuidToString(device->guidInstance) << std::endl;
	std::cout << "guidProduct:     " << GuidToString(device->guidProduct) << std::endl;
	std::cout << "dwDevType:       " << device->dwDevType << std::endl;
	std::cout << "tszInstanceName: " << device->tszInstanceName << std::endl;
	std::cout << "tszProductName:  " << device->tszProductName << std::endl;
	std::cout << "guidFFDriver:    " << GuidToString(device->guidFFDriver) << std::endl;
	return DIENUM_CONTINUE;
}

class N64Controller;
typedef std::shared_ptr<N64Controller> N64ControllerPtr;

class N64Controller
{
public:
	~N64Controller()
	{
		// Stop listening for events
		threadRunning_ = false;
		SetEvent(dataAvailableEvent_);
		CloseHandle(dataAvailableEvent_);
		thread_.join();

		// Close device
		device_->Unacquire();
		device_->Release();
		device_ = nullptr;

		// Cleanup virtual pad
		vigem_target_remove(vigemClient_, vigemPad_);
		vigem_target_free(vigemPad_);
	}

	static N64ControllerPtr create(PVIGEM_CLIENT vigemClient, LPDIRECTINPUT8 dinput, GUID id)
	{
		auto controller = new N64Controller();
		if (!controller->init(vigemClient, dinput, id))
			return nullptr;
		return N64ControllerPtr(controller);
	}

private:
	N64Controller() = default;

	bool init(PVIGEM_CLIENT vigemClient, LPDIRECTINPUT8 dinput, GUID id)
	{
		vigemClient_ = vigemClient;

		auto checkDeviceOp = [this](HRESULT hr) -> bool
		{
			if (hr == DI_OK)
				return true;
			device_->Release();
			std::cout << "Error: " << errToString(hr) << std::endl;
			return false;
		};

		if (dinput->CreateDevice(id, &device_, nullptr) != DI_OK)
			return false;

		if (!checkDeviceOp(device_->SetDataFormat(&N64ControllerFormat)))
			return false;

		dataAvailableEvent_ = CreateEvent( 
	        nullptr, // default security attributes
	        false,   // automatically reset event
	        false,   // initial state is nonsignaled
	        nullptr  // object name
	    );

	    if (!checkDeviceOp(device_->SetEventNotification(dataAvailableEvent_)))
			return false;

		vigemPad_      = vigem_target_x360_alloc();
		const auto pir = vigem_target_add(vigemClient_, vigemPad_);
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

				    if ((hr = device_->GetDeviceState(sizeof(N64ControllerState), &state)) != DI_OK)
				    {
				    	std::cout << "Failed to read device state: " << errToString(hr) << std::endl;
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
						return static_cast<SHORT>(std::min(65535.0f, std::max(0L, value - min) / static_cast<float>(max - min) * 65535.0f) - 65535.0f/2.0f);
				    };
				    auto convertCButtonToAnalog = [](bool negative, bool positive) -> SHORT
				    {
				    	if (negative && positive)
				    		return 0;
				    	if (!negative && !positive)
				    		return 0;
				    	return negative ? -32767 : 32767;
				    };

				    // NOTE: the remaining unbound xbox controller buttons are:
				    // XUSB_GAMEPAD_LEFT_THUMB  = 0x0040,
				    // XUSB_GAMEPAD_RIGHT_THUMB = 0x0080,
				    // XUSB_GAMEPAD_Y           = 0x8000

				    x360Report.bLeftTrigger  = state.buttons[N64Button::Z] ? 255 : 0;
				    x360Report.bRightTrigger = 0;
				    x360Report.sThumbLX      = convertAnalog(state.xAxis, X_MIN, X_MAX);
				    x360Report.sThumbLY      = -convertAnalog(state.yAxis, Y_MIN, Y_MAX);
				    x360Report.sThumbRX      = convertCButtonToAnalog(state.buttons[N64Button::C_LEFT], state.buttons[N64Button::C_RIGHT]);
				    x360Report.sThumbRY      = convertCButtonToAnalog(state.buttons[N64Button::C_DOWN], state.buttons[N64Button::C_UP]);
				    x360Report.wButtons =
				    	state.buttons[N64Button::A] ? XUSB_GAMEPAD_A : 0 |
				    	state.buttons[N64Button::B] ? XUSB_GAMEPAD_B : 0 |
				    	state.buttons[N64Button::ZR] ? XUSB_GAMEPAD_X : 0 |
				    	state.buttons[N64Button::LEFT_BUMPER] ? XUSB_GAMEPAD_LEFT_SHOULDER : 0 |
				    	state.buttons[N64Button::RIGHT_BUMPER] ? XUSB_GAMEPAD_RIGHT_SHOULDER : 0 |
				    	state.buttons[N64Button::START] ? XUSB_GAMEPAD_START : 0 |
				    	state.buttons[N64Button::HOME] ? XUSB_GAMEPAD_GUIDE : 0 |
				    	state.buttons[N64Button::CIRCLE] ? XUSB_GAMEPAD_BACK : 0 |
				    	state.dpad == 0     ? XUSB_GAMEPAD_DPAD_UP : 0 |
				    	state.dpad == 4500  ? XUSB_GAMEPAD_DPAD_UP | XUSB_GAMEPAD_DPAD_RIGHT : 0 |
				    	state.dpad == 9000  ? XUSB_GAMEPAD_DPAD_RIGHT : 0 |
				    	state.dpad == 13500 ? XUSB_GAMEPAD_DPAD_RIGHT | XUSB_GAMEPAD_DPAD_DOWN : 0 |
				    	state.dpad == 18000 ? XUSB_GAMEPAD_DPAD_DOWN : 0 |
				    	state.dpad == 22500 ? XUSB_GAMEPAD_DPAD_DOWN | XUSB_GAMEPAD_DPAD_LEFT : 0 |
				    	state.dpad == 27000 ? XUSB_GAMEPAD_DPAD_LEFT : 0 |
				    	state.dpad == 31500 ? XUSB_GAMEPAD_DPAD_LEFT | XUSB_GAMEPAD_DPAD_UP : 0;

				    vigem_target_x360_update(vigemClient_, vigemPad_, x360Report);

				    lastState = state;
	    		}
	    	}
	    );

	    if (!checkDeviceOp(device_->Acquire()))
			return false;

	    return true;
	}

	LPDIRECTINPUTDEVICE8A device_;
	HANDLE dataAvailableEvent_;
	std::thread thread_;
	std::atomic_bool threadRunning_{ true };
	PVIGEM_CLIENT vigemClient_;
	PVIGEM_TARGET vigemPad_;
};

std::mutex appMutex;
std::condition_variable appCVar;
bool appRunning { true };
void signalHandler(int)
{
	std::lock_guard<std::mutex> lock(appMutex);

	appRunning = false;
	appCVar.notify_one();
}

int main(int argc, char** argv)
{
	signal(SIGINT, signalHandler);  

	const auto client = vigem_alloc();

	if (client == nullptr)
	{
		std::cout << "Unable to allocate vigem client" << std::endl;
	    return -1;
	}

	const auto retval = vigem_connect(client);
	if (!VIGEM_SUCCESS(retval))
	{
	    std::cout << "ViGEm Bus connection failed with error code: 0x" << std::hex << retval << std::endl;
	    return -1;
	}

	auto hInstance = GetModuleHandle(0);

	LPDIRECTINPUT8 dinput = nullptr;
	if (DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8A, reinterpret_cast<LPVOID*>(&dinput), nullptr) != DI_OK)
	{
		std::cout << "Failed to create direct input interface" << std::endl;
		return -1;
	}

	std::vector<GUID> deviceIDs;
	dinput->EnumDevices(DI8DEVCLASS_GAMECTRL, enumerateDevice, &deviceIDs, DIEDFL_ATTACHEDONLY);

	std::vector<N64ControllerPtr> controllers;
	for (const auto& id : deviceIDs)
	{
		auto dev = N64Controller::create(client, dinput, id);
		if (dev != nullptr)
			controllers.push_back(dev);
	}

	std::cout << "Configured " << controllers.size() << " controllers" << std::endl;
	{
        std::unique_lock<std::mutex> lk(appMutex);
        appCVar.wait(lk, []{ return !appRunning; });
    }

	// Cleanup controllers
	controllers.clear();

	// Cleanup DirectInput
	dinput->Release();

	// Cleanup ViGEm
	vigem_disconnect(client);
	vigem_free(client);

	return 0;
}
