#include <windows.h>

#include <stdint.h>

#include <Xinput.h>
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
}



// todo : global
// int foo; this can be used extenal like
// external int foo;  // you can do that with static, can not be called by name, 
global_variable bool GlobalRunning;   // static = 0 init

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
struct win32_window_dimension
{
	int Width;
	int Height;

};

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


// ------------------dynamic load 
// copy from XINPUT.h
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
//declare a function pointer
global_variable x_input_get_state *XInputGetState_;
global_variable x_input_set_state *XInputSetState_;

// avoid conflict, two clever
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_


/*
DWORD WINAPI XInputGetState
(
_In_  DWORD         dwUserIndex,  // Index of the gamer associated with the device
_Out_ XINPUT_STATE* pState        // Receives the current state
);

DWORD WINAPI XInputSetState
(
_In_ DWORD             dwUserIndex,  // Index of the gamer associated with the device
_In_ XINPUT_VIBRATION* pVibration    // The vibration information to send to the controller
);
*
*/




internal void
RenderWeirdGradient(win32_offscreen_buffer Buffer, int XOffset, int YOffset)
{

	int Width = Buffer.Width;
	int Height = Buffer.Height;

	// casting void* to different size types to make pointer arithmetic simpler for bitmap access
	uint8 * Row = (uint8*)Buffer.Memory;

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
		Row += Buffer.Pitch;
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
Win32DisplayBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight, win32_offscreen_buffer Buffer,
int X, int Y, int Width, int Height)
{
	//todo (Jason) aspect ratio correction
	StretchDIBits(DeviceContext,
		0, 0, WindowWidth, WindowHeight, //X, Y, Width, Height, // source
		0, 0, Buffer.Width, Buffer.Height, //X, Y, Width, Height, // destination
		Buffer.Memory, &Buffer.Info,
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
	case WM_PAINT:
	{
		PAINTSTRUCT Paint;
		HDC DeviceContext = BeginPaint(Window, &Paint);

		int X = Paint.rcPaint.left;
		int Y = Paint.rcPaint.top;
		int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
		int Width = Paint.rcPaint.right - Paint.rcPaint.left;


		win32_window_dimension Dimension = Win32GetWindowDimension(Window);

		Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackBuffer, X, Y, Dimension.Width, Dimension.Height);



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

	/*  open a windows --------------*/

	WNDCLASS WindowsClass = {}; // init  all to 0
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


					}
					else // the controller is not available
					{
						
					}
				}
				RenderWeirdGradient(GlobalBackBuffer, XOffset, YOffset);

				HDC DeviceContext = GetDC(Window);

				win32_window_dimension Dimension = Win32GetWindowDimension(Window);

				RECT ClientRect;
				GetClientRect(Window, &ClientRect);


				Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackBuffer, 0, 0, Dimension.Width, Dimension.Height);
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
