#include <windows.h>

#define internal static			// internal function
#define global_variable static
#define local_persist static

// todo : global
global_variable bool Running ;   // static = 0 init
global_variable BITMAPINFO BitmapInfo;
global_variable void * BitmapMemory;
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDeviceContext;
// device independant Bitmap
internal void 
Win32ResizeDIBSection(int Width, int Height)
{
	// todo : Bulletproof this.
	// maybe dont free first , free after, then free after if that fails

	// free out DIBSection
	if (BitmapHandle)
	{
		DeleteObject(BitmapHandle); // if we have bitmapHandle, delete it
	}
	if (! BitmapDeviceContext)
	{ 
		// todo : should we create this 
		BitmapDeviceContext = CreateCompatibleDC(0); //
	}
	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = Width;
	BitmapInfo.bmiHeader.biHeight = Height;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;
	//BitmapInfo.bmiHeader.biSizeImage = 0;
	//BitmapInfo.bmiHeader.biXPelsPerMeter = 0;
	//BitmapInfo.bmiHeader.biYPelsPerMeter = 0;
	//BitmapInfo.bmiHeader.biClrUsed = 0;
	//BitmapInfo.bmiHeader.biClrImportant = 0;


	//HBITMAP - bitmap handle;

	BitmapHandle = CreateDIBSection(
		BitmapDeviceContext, &BitmapInfo,
		DIB_RGB_COLORS, 
		&BitmapMemory, 
		0,0		);

}
internal void 
Win32UpdateWindow(HDC DeviceContext, int X, int Y, int Width, int Height)
{
	StretchDIBits(DeviceContext,
		X, Y, Width, Height, // destination
		X, Y, Width, Height, // source
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

		Win32UpdateWindow(DeviceContext, X, Y, Width, Height);


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
			HWND WindowHandle =   CreateWindowEx(
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
			if (WindowHandle)
			{
				Running = true;


				while (Running){
					MSG Message;
					BOOL MessageResult = GetMessage(&Message,
						0,
						0,// wMsgFilterMin,
						0 //wMsgFilterMax
						);

					if (MessageResult > 0)
					{
						TranslateMessage(&Message);
						DispatchMessage(&Message);
					}
					else
					{
						break;
					}


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
