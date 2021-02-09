#include "stdafx.h"
#include "messaging.h"
#include "utilities.h"

#pragma warning (disable: 4091)

extern "C"
{
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}

HWND Hwnd;
WNDPROC BaseWindowProc;
PCOPYDATASTRUCT pDane;

LRESULT APIENTRY WndProc( HWND hWnd, // handle for this window
                         UINT uMsg, // message for this window
                         WPARAM wParam, // additional message information
                         LPARAM lParam) // additional message information
{
    switch( uMsg ) // check for windows messages
    {
        case WM_COPYDATA: {
            // obsługa danych przesłanych przez program sterujący
            pDane = (PCOPYDATASTRUCT)lParam;
            if( pDane->dwData == MAKE_ID4('E', 'U', '0', '7')) // sygnatura danych
                multiplayer::OnCommandGet( ( multiplayer::DaneRozkaz *)( pDane->lpData ) );
            break;
        }
		case WM_KEYDOWN:
		case WM_KEYUP: {
			if (wParam == VK_INSERT || wParam == VK_DELETE || wParam == VK_HOME || wParam == VK_END ||
				wParam == VK_PRIOR || wParam == VK_NEXT || wParam == VK_SNAPSHOT)
				lParam &= ~0x1ff0000;
			if (wParam == VK_INSERT)
				lParam |= 0x152 << 16;
			else if (wParam == VK_DELETE)
				lParam |= 0x153 << 16;
			else if (wParam == VK_HOME)
				lParam |= 0x147 << 16;
			else if (wParam == VK_END)
				lParam |= 0x14F << 16;
			else if (wParam == VK_PRIOR)
				lParam |= 0x149 << 16;
			else if (wParam == VK_NEXT)
				lParam |= 0x151 << 16;
			else if (wParam == VK_SNAPSHOT)
				lParam |= 0x137 << 16;
			break;
		}
    }
    // pass all messages to DefWindowProc
    return CallWindowProc( BaseWindowProc, Hwnd, uMsg, wParam, lParam );
};
