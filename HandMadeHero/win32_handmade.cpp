#include <windows.h>

#include <stdint.h>
#define internal static			// internal function
#define global_variable static
#define local_persist static



typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;


typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

// todo : global
global_variable bool Running ;   // static = 0 init
global_variable BITMAPINFO BitmapInfo;
global_variable void * BitmapMemory;
global_variable int BitmapWidth ;
global_variable int BitmapHeight ;
global_variable int BytesPerPixel = 4;
//
internal void 
RenderWeirdGradient( int XOffset, int YOffset)
{

	int Width = BitmapWidth;
	int Height = BitmapHeight;
	int Pitch = Width * BytesPerPixel;// different between this row and next row
	// casting void* to different size types to make pointer arithmetic simpler for bitmap access
	uint8 * Row = (uint8*)BitmapMemory;

	for (int Y = 0; Y < BitmapHeight; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0; X < BitmapWidth; ++X)
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
		Row += Pitch;
	}
}

// device independant Bitmap
internal void 
Win32ResizeDIBSection(int Width, int Height)
{
	// todo : Bulletproof this.
	// maybe dont free first , free after, then free after if that fails
	// making sure we free any allocated memory before we resize , using virtual free
	if (BitmapMemory){
		VirtualFree(BitmapMemory, 0, MEM_RELEASE);  // MEM_RELEASE|MEM_DECOMMIT
	}
	BitmapWidth = Width;
	BitmapHeight = Height;
	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = BitmapWidth;
	BitmapInfo.bmiHeader.biHeight = - BitmapHeight; // negative number make windo framebuffer use a top-down coordinate system
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;  // 24bit needed ,for allgning on boundary
	BitmapInfo.bmiHeader.biCompression = BI_RGB;
	//BitmapInfo.bmiHeader.biSizeImage = 0;
	//BitmapInfo.bmiHeader.biXPelsPerMeter = 0;
	//BitmapInfo.bmiHeader.biYPelsPerMeter = 0;
	//BitmapInfo.bmiHeader.biClrUsed = 0;
	//BitmapInfo.bmiHeader.biClrImportant = 0;

	// note : thank you to Chris Hecker of Spy Party fame
	// for clarify the deal with StretchDIBits and BitBlt! 
	// No more DC for us

	int BitmapMemorySize = (BitmapWidth * BitmapHeight) * BytesPerPixel;
	BitmapMemory = VirtualAlloc(0, //address
		BitmapMemorySize,  //size
		MEM_COMMIT, // MEM_RESERVE
		PAGE_READWRITE);	//memory protection

	// todo : clear to black
}
internal void 
Win32UpdateWindow(HDC DeviceContext,RECT *ClientRect, int X, int Y, int Width, int Height)
{
	/*
	
	*/
	int WindowWidth = ClientRect->right - ClientRect->left;
	int WindowHeight = ClientRect->bottom - ClientRect->top;

	StretchDIBits(DeviceContext,
		0,0,BitmapWidth, BitmapHeight, //X, Y, Width, Height, // destination
		0,0,WindowWidth, WindowHeight, //X, Y, Width, Height, // source
		BitmapMemory, &BitmapInfo,
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
		RECT ClientRect;
		GetClientRect(Window, &ClientRect);
		int Width = ClientRect.right - ClientRect.left;
		int Height = ClientRect.bottom - ClientRect.top;

		Win32ResizeDIBSection(Width, Height);
		OutputDebugStringA("WM_SIZE");
	} break;
	case WM_DESTROY:
	{
		Running = false;  // todo : error
		//OutputDebugStringA("WM_DESTROY");
	} break;
	case WM_CLOSE:
	{
		Running = false; // todo: message to user
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

		RECT ClientRect;
		GetClientRect(Window, &ClientRect);
		Win32UpdateWindow(DeviceContext, &ClientRect,  X, Y, Width, Height);



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
		// CS_CLASSDC - device context, horizon vertical redraw
	WindowsClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;

	WindowsClass.lpfnWndProc = Win32MainWindowCallBack;

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
			HWND Window =   CreateWindowEx(
				0 ,//     dwExStyle,
				WindowsClass.lpszClassName, //   lpClassName,
				"HandMade Hero",//   lpWindowName,
				WS_OVERLAPPEDWINDOW|WS_VISIBLE   , // dwStyle,
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
				Running = true;


				while (Running){

					MSG Message;
					while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
					{
						if (Message.message == WM_QUIT)
						{
							Running = false;
						}
						TranslateMessage(&Message);
						DispatchMessage(&Message);
					}
	
					RenderWeirdGradient(XOffset, YOffset);

					HDC DeviceContext = GetDC(Window);
					RECT ClientRect;
					GetClientRect(Window, &ClientRect);
					int WindowWidth = ClientRect.right - ClientRect.left;
					int WindowHeight = ClientRect.bottom - ClientRect.top;

					Win32UpdateWindow(DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);
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
