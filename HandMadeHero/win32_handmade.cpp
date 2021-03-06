#include <windows.h>

#include <stdint.h>

#include <Xinput.h>
#include <dsound.h>
#include <string>
#define internal static			// internal function
#define global_variable static
#define local_persist static

namespace
{
	typedef uint8_t uint8;
	typedef uint16_t uint16;
	typedef uint32_t uint32;
	typedef uint64_t uint64;


	typedef int8_t int8;
	typedef int16_t int16;
	typedef int32_t int32;
	typedef int64_t int64;

	typedef int32 bool32;


	// todo : global
	// int foo; this can be used extenal like
	// external int foo;  // you can do that with static, can not be called by name, 
	global_variable bool GlobalRunning;   // static = 0 init

	// ------------------dynamic load 
	// copy from XINPUT.h


	/*
	// typedef a function
	typedef DWORD WINAPI x_input_get_state
	(
	_In_  DWORD         dwUserIndex,  // Index of the gamer associated with the device
	_Out_ XINPUT_STATE* pState        // Receives the current state
	);

	typedef DWORD WINAPI x_input_set_state
	(
	_In_ DWORD             dwUserIndex,  // Index of the gamer associated with the device
	_In_ XINPUT_VIBRATION* pVibration    // The vibration information to send to the controller
	);
	*/

	/** the way to do function pointers : many function have the same signature
	 * 1. pound define a thing that generate the signature
	 * 2. typedef use that macro
	 * 3. define the pointer by using that new typedef
	 * 4. defint the stubs by using that macro
	 */

	// define function prototype once
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
	// use prototype to generate : function signature; then define a type as the signature
	typedef X_INPUT_GET_STATE(x_input_get_state);
	typedef X_INPUT_SET_STATE(x_input_set_state);
	// use prototype to generate : function definition ; with a different name
	X_INPUT_GET_STATE(XInputGetStateStub)
	{
		return(ERROR_DEVICE_NOT_CONNECTED);
	}
	X_INPUT_SET_STATE(XInputSetStateStub)
	{
		return(ERROR_DEVICE_NOT_CONNECTED);
	}

	//declare a function pointer
	global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
	global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

	// avoid conflict, two clever : can not undefine, so rename it to XInputGetState_
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

	/*--------------------------------------------------------------------------------------*/


	extern _Check_return_ HRESULT WINAPI DirectSoundCreate(_In_opt_ LPCGUID pcGuidDevice, _Outptr_ LPDIRECTSOUND *ppDS, _Pre_null_ LPUNKNOWN pUnkOuter);

	#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
	typedef DIRECT_SOUND_CREATE(direct_sound_create);



}





struct win32_offscreen_buffer
{

	BITMAPINFO Info;
	void * Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel = 4;
};

global_variable win32_offscreen_buffer GlobalBackBuffer;
// create a struct if we want to return multiple values

internal void 
Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
	//1.  load library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
	if (DSoundLibrary)
	{
		//2.  get a DirectSound object - cooperative mode
		direct_sound_create * DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
		
		LPDIRECTSOUND DirectSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			// wave format
			WAVEFORMATEX WaveFormat ={};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.nBlockAlign = WaveFormat.nChannels * WaveFormat.wBitsPerSample / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
	
			WaveFormat.cbSize = 0;

			if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{

				// set buffer format
				DSBUFFERDESC BufferDescription = { };
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
				// BufferDescription.dwBufferBytes is zero
				// create the primary buffer-------------------------------------
				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
				{
					HRESULT error = PrimaryBuffer->SetFormat(&WaveFormat);
					if (SUCCEEDED(error))
					{
						// finally set the format
						OutputDebugStringA("primary buffer format was set. \n");
					} else
					{
						//log error  - SetFormat
						OutputDebugStringA(("primary buffer format error: " + std::to_string(error)).c_str() );
					}
				} else
				{
					//log error - CreateSoundBuffer
				}

			} else
			{
				// log error - SetCooperativeLevel
			}
			

			//4. create a second buffer - to write to ---------------------------------
 
			// set buffer format
			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0; 
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;

			LPDIRECTSOUNDBUFFER SecondaryBuffer;
			HRESULT error = DirectSound->CreateSoundBuffer(&BufferDescription, &SecondaryBuffer, 0);
			if (SUCCEEDED(error))
			{
				OutputDebugStringA("secondary buffer created successfully. \n");
			} else
			{
				OutputDebugStringA(("primary buffer format error: " + std::to_string(error)).c_str());
			}

			//5. start it playing
		} else
		{
			// log error - DirectSoundCreate
		}



	} else
	{
		//log error - load library DirectSound
	}



}


struct win32_window_dimension
{
	int Width;
	int Height;

};

internal
win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;
	return(Result);
}



internal void
Win32LoadXInput(void)
{
	HMODULE XInputLibrary  = LoadLibraryA("xinput1_4.dll");
	if (!XInputLibrary)
	{
		XInputLibrary = LoadLibraryA("xinput1_3.dll");
	}
	
	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state *) GetProcAddress(XInputLibrary,"XInputSetState" );
	}
}



internal void
RenderWeirdGradient(win32_offscreen_buffer *Buffer, int XOffset, int YOffset)
{

	int Width = Buffer->Width;
	int Height = Buffer->Height;

	// casting void* to different size types to make pointer arithmetic simpler for bitmap access
	uint8 * Row = (uint8*)Buffer->Memory;

	for (int Y = 0; Y < Height; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0; X < Width; ++X)
		{
			uint8 Blue = (X + XOffset);
			uint8 Green = (Y + YOffset);
			*Pixel++ = ((Green << 8) | Blue);



			/*                0B 1G 2R  3
			pixel in memory : 00 00 00 00
			Little endian architecture,
			in register     : xx BB GG RR

			*Pixel = (uint8)(X + XOffset);
			++Pixel;

			*Pixel = (uint8)(Y + YOffset);
			++Pixel;


			*Pixel = 0;
			++Pixel;

			*Pixel = 0;
			++Pixel;
			*/
		}
		//int Pitch = Width * Buffer.BytesPerPixel;// different between this row and next row
		Row += Buffer->Pitch;
	}
}

// device independant Bitmap
internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
	// todo : Bulletproof this.
	// maybe dont free first , free after, then free after if that fails
	// making sure we free any allocated memory before we resize , using virtual free
	if (Buffer->Memory){
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);  // MEM_RELEASE|MEM_DECOMMIT
	}
	Buffer->Width = Width;
	Buffer->Height = Height;
	Buffer->BytesPerPixel = 4;
	/*
	 * when the biHeight field is negative,
	 * this is the clue to windows to treat this bitmap as top-down, not bottom-up
	 * meaning that the first three bytes of the image are the color for the top left pixel
	 * in the bitmap, not the bottom left.	 *
	 */
	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height; // negative number make windo framebuffer use a top-down coordinate system
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;  // 24bit needed ,for allgning on boundary
	Buffer->Info.bmiHeader.biCompression = BI_RGB;
	//BitmapInfo.bmiHeader.biSizeImage = 0;
	//BitmapInfo.bmiHeader.biXPelsPerMeter = 0;
	//BitmapInfo.bmiHeader.biYPelsPerMeter = 0;
	//BitmapInfo.bmiHeader.biClrUsed = 0;
	//BitmapInfo.bmiHeader.biClrImportant = 0;

	// note : thank you to Chris Hecker of Spy Party fame
	// for clarify the deal with StretchDIBits and BitBlt! 
	// No more DC for us

	int BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;

	// allocate a big chunk of memory by pages
	Buffer->Memory = VirtualAlloc(0, //address
		BitmapMemorySize,  //size
		MEM_COMMIT, // MEM_RESERVE
		PAGE_READWRITE);	//memory protection

	Buffer->Pitch = Width * Buffer->BytesPerPixel;   // calculate pitch
	// todo : clear to black

}
/**
 * \brief
 * \param DeviceContext
 * \param ClientRect
 * \param X
 * \param Y
 * \param Width
 * \param Height
 */
internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight)
{
	//todo (Jason) aspect ratio correction
	StretchDIBits(DeviceContext,
		0, 0, WindowWidth, WindowHeight, //X, Y, Width, Height, // source
		0, 0, Buffer->Width, Buffer->Height, //X, Y, Width, Height, // destination
		Buffer->Memory, &Buffer->Info,
		DIB_RGB_COLORS, //DIB_PAL_COLORS  16 colors
		SRCCOPY // raster operation code : SRCCOPY, SRCAND
		);

}

//
LRESULT CALLBACK Win32MainWindowCallBack(
	HWND   Window,
	UINT   Message,
	WPARAM WParam,
	LPARAM LParam
	){
	LRESULT Result = 0;

	switch (Message){
	case WM_SIZE:
	{

		OutputDebugStringA("WM_SIZE");
	} break;
	case WM_DESTROY:
	{
		GlobalRunning = false;  // todo : error
		//OutputDebugStringA("WM_DESTROY");
	} break;
	case WM_CLOSE:
	{
		GlobalRunning = false; // todo: message to user
		//PostQuitMessage(0);  // quit window
		//DestroyWindow(Window);
		OutputDebugStringA("WM_CLOSE");
	} break;
	case WM_ACTIVATEAPP:
	{
		OutputDebugStringA("WM_ACTIVATEAPP");
	} break;

	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		uint32 VKCode = WParam;
		bool WasDown = ((LParam &(1 << 30) )!= 0);  // 30th bit 
		bool IsDown = ((LParam &(1 << 31)) == 0);   // 31st bit
		if (IsDown != WasDown) // eat repeat
		{
			if (VKCode == 'W')
			{
				OutputDebugStringA("w\n");
			}
			else if (VKCode == 'A')
			{
			}
			else if (VKCode == 'S')
			{
			}
			else if (VKCode == 'D')
			{
			}
			else if (VKCode == 'Q')
			{
			}
			else if (VKCode == 'E')
			{
			}
			else if (VKCode == VK_UP)
			{
			}
			else if (VKCode == VK_DOWN)
			{
			}
			else if (VKCode == VK_LEFT)
			{
			}
			else if (VKCode == VK_RIGHT)
			{
			}
			else if (VKCode == VK_ESCAPE)
			{
				OutputDebugStringA("ESCAPE:");
				if (IsDown)
				{
					OutputDebugStringA("IsDown ");
				}
				if (WasDown)
				{
					OutputDebugStringA("WasDown");
				}
				OutputDebugStringA("\n");
			}
			else if (VKCode == VK_SPACE)
			{

			}
			else
			{
				OutputDebugStringA("some key pressed\n");
			}
		}
		bool AltKeyWasDown = (LParam & (1 <<29) ) != 0;
		if (VKCode == VK_F4 && AltKeyWasDown)
		{
			GlobalRunning = false;
		}
	
	}break;

	case WM_PAINT:
	{
		PAINTSTRUCT Paint;
		HDC DeviceContext = BeginPaint(Window, &Paint);

		int X = Paint.rcPaint.left;
		int Y = Paint.rcPaint.top;
		int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
		int Width = Paint.rcPaint.right - Paint.rcPaint.left;


		win32_window_dimension Dimension = Win32GetWindowDimension(Window);

		Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);



		//local_persist DWORD Operation = WHITENESS;
		//PatBlt(DeviceContext, X, Y, Width, Height, Operation);
		//Operation = (Operation == WHITENESS ? BLACKNESS : WHITENESS);

		EndPaint(Window, &Paint);
	} break;
	default:
	{
		//OutputDebugStringA("WM_DESTROY");
		Result = DefWindowProc(Window, Message, WParam, LParam);
	} break;
	}

	return(Result);
}




int CALLBACK
WinMain(
HINSTANCE Instance,
HINSTANCE PrevInstance,
LPSTR     CommandLine,
int       ShowCode
){

	/* ------------------demo msg box

		GetModuleHandle(0);
		MessageBox(0, "This is handmade hero.",  "HandMade Hero", MB_OK|MB_ICONINFORMATION);
		*/

	// load xinput
	Win32LoadXInput();

	/*  open a windows --------------*/

	WNDCLASSA WindowsClass = {}; // init  all to 0
	//resize  bitmap
	//win32_window_dimension Dimension = Win32GetWindowDimension(Window);
	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
	// CS_CLASSDC - device context, horizon vertical redraw the whole window, not just the changed part
	WindowsClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;

	// callback - who to call 
	WindowsClass.lpfnWndProc = Win32MainWindowCallBack;
	// instance of process
	WindowsClass.hInstance = Instance; // GetModuleHandle(0)
	//hInstance.hIcon;
	WindowsClass.lpszClassName = "HandMadeHeroWindowClass";

	// pattern : use structure as parameter
	// type def foo foo ; emulate c++ for c

	/*
	typedef struct tagWNDCLASS { // tagWNDCLASS is not needed
	UINT      style;
	WNDPROC   lpfnWndProc;
	int       cbClsExtra;
	int       cbWndExtra;
	HINSTANCE hInstance;
	HICON     hIcon;
	HCURSOR   hCursor;
	HBRUSH    hbrBackground;
	LPCTSTR   lpszMenuName;
	LPCTSTR   lpszClassName;
	} WNDCLASS, *PWNDCLASS;
	*/
	// WNDCLASS => struct tagWNDCLASS
	// PWNDCLASS => struct tagWNDCLASS *


	if (RegisterClass(&WindowsClass))
	{
		HWND Window = CreateWindowEx(
			0,//     dwExStyle,
			WindowsClass.lpszClassName, //   lpClassName,
			"HandMade Hero",//   lpWindowName,
			WS_OVERLAPPEDWINDOW | WS_VISIBLE, // dwStyle,
			CW_USEDEFAULT,    // x,
			CW_USEDEFAULT, //y,
			CW_USEDEFAULT,//       nWidth,
			CW_USEDEFAULT,//       nHeight,
			0, //     hWndParent,
			0,//     hMenu,
			Instance, // hInstance,
			0//    lpParam
			);
		if (Window)
		{
			int XOffset = 0;
			int YOffset = 0;

			// only after open a window, we can load DirectSound
			Win32InitDSound(Window, 48000, 48000*sizeof(int16)*2);

			GlobalRunning = true;
			while (GlobalRunning){

				MSG Message;
				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					if (Message.message == WM_QUIT)
					{
						GlobalRunning = false;
					}
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}
				// 4 controller
				for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++)
				{
					XINPUT_STATE ControllerState;
					if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS) // get state
					{
						//ControllerState.dwPacketNumber;  // change numbers
						XINPUT_GAMEPAD * Pad = & ControllerState.Gamepad;
						bool Up = Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
						bool Down = Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
						bool Left = Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
						bool Right = Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
						bool Start = Pad->wButtons & XINPUT_GAMEPAD_START;
						bool Back = Pad->wButtons & XINPUT_GAMEPAD_BACK;
						bool LeftShoulder = Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
						bool RightShoulder = Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
						bool AButton = Pad->wButtons & XINPUT_GAMEPAD_A;
						bool BButton = Pad->wButtons & XINPUT_GAMEPAD_B;
						bool XButton = Pad->wButtons & XINPUT_GAMEPAD_X;
						bool YButton = Pad->wButtons & XINPUT_GAMEPAD_Y;

						int16 StickX = Pad->sThumbLX;
						int16 StickY = Pad->sThumbLY;
						if (AButton)
						{
							YOffset += 2;
						}

					}
					else // the controller is not available
					{
						
					}
				}
				XINPUT_VIBRATION Vibration;
				Vibration.wLeftMotorSpeed = 60000;
				Vibration.wRightMotorSpeed = 60000;
				XInputSetState_(0, &Vibration );
				RenderWeirdGradient(&GlobalBackBuffer, XOffset, YOffset);

				HDC DeviceContext = GetDC(Window);

				win32_window_dimension Dimension = Win32GetWindowDimension(Window);

				RECT ClientRect;
				GetClientRect(Window, &ClientRect);


				Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, 
											Dimension.Width, Dimension.Height);
				// release DC
				ReleaseDC(Window, DeviceContext);
				++XOffset;



			}


		}
		else{
			// todo : logging
		}
	}
	else
	{
		// todo: logging
	}

	return(0);
}
