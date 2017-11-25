#include <windows.h>

#define internal static			// internal function
#define global_variable static
#define local_persist static

// todo : global
global_variable bool Running ;   // static = 0 init
//
LRESULT CALLBACK MainWindowCallBack(
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
		local_persist DWORD Operation = WHITENESS;
		PatBlt(DeviceContext, X, Y, Width, Height, Operation);
		Operation = (Operation == WHITENESS ? BLACKNESS : WHITENESS);

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

	WindowsClass.lpfnWndProc = MainWindowCallBack;

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
