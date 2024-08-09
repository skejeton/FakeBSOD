// clang-format off
#include <Windows.h>

const char *Text;

const char *DefaultText = "\n"
"A problem has been detected and Windows has been shut down to prevent damage\n"
"to your computer.\n"
"\n"
"The problem seems to be caused by the following file: SPCMDCON.SYS\n"
"\n"
"PAGE_FAULT_IN_NONPAGED_AREA\n"
"\n"
"If this is the first time you've seen this stop error screen,\n"
"restart your computer. If this screen appears again, follow\n"
"these steps:\n"
"\n"
"Check to make sure any new hardware or software is properly installed.\n"
"If this is a new installation, ask your hardware or software manufacturer\n"
"for any Windows updates you might need.\n"
"\n"
"If problems continue, disable or remove any newly installed hardware\n"
"or software. Disable BIOS memory options such as caching or shadowing.\n"
"If you need to use Safe Mode to remove or disable components, restart\n"
"your computer, press F8 to select Advanced Startup Options, and then\n"
"select Safe Mode.\n"
"\n"
"Technical Information:\n"
"\n"
"*** STOP: 0x00000050 (0xFD3094C2,0x00000001,0xFBFE7617,0x00000000)\n"
"\n"
"*** SPCMDCON.SYS - Address FBFE7617 base at FBFE5000, DateStamp 3d6dd67c\n"
"\n"
"Beginning dump of physical memory\n"
"Physical memory dump complete.\n"
"\n"
"Contact your system administrator or technical support group for further\n"
"assistance.\n";


BITMAPINFO BitmapInfo = {0};
void *BitmapPixels;
HBITMAP Bmp;
HDC DC;
HFONT Font;
size_t Ticks = 0;
size_t Len;
HANDLE TimerEvent;
HANDLE TimerQueue;
HANDLE Timer;
HBRUSH Bkg;
HWND Window;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }
}

void CALLBACK TimerProc(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
    BOOL Done = FALSE;
    Ticks++;
    if (Ticks > 50) {
        size_t Chars = (Ticks-50)*10;
        if (Chars > Len) {
            Chars = Len;
            Done = TRUE;
        }
        DrawTextExA(DC, (char*)Text, Chars, &(RECT){0, 0, 800, 600}, DT_LEFT | DT_TOP, NULL);
    } else {
        FillRect(DC, &(RECT){0, 0, 800, Ticks*12}, Bkg);
    }
    RECT R;
    GetWindowRect(Window, &R);
    StretchDIBits(GetDC(Window), 0, 0, R.right-R.left, R.bottom-R.top, 0, 0, 800, 600, BitmapPixels, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
    SetEvent(TimerEvent);
    if (!Done)
        CreateTimerQueueTimer( &Timer, TimerQueue, (WAITORTIMERCALLBACK)TimerProc, 0, 25, 0, 0);
    else
        DeleteTimerQueueTimer(TimerQueue, Timer, TimerEvent);
}

char *strcpy(char *dest, const char *src)
{
    char *ret = dest;
    while ((*dest++ = *src++));
    return ret;
}

size_t strlen(const char *str)
{
    size_t len = 0;
    while (*str++)
        len++;
    return len;
}

int WinMainCRTStartup()
{
    Text = DefaultText;
    OFSTRUCT Dummy;
    HFILE CustomInput = OpenFile("bsod.txt", &Dummy, OF_READ);
    if (CustomInput != HFILE_ERROR) {
        DWORD Size = GetFileSize((HANDLE)CustomInput, NULL);
        char *Buf = VirtualAlloc(NULL, Size+1, MEM_COMMIT, PAGE_READWRITE);
        ReadFile((HANDLE)CustomInput, Buf, Size, &Size, NULL);
        Buf[Size] = 0;
        Text = Buf;
    }

    WNDCLASSA WC = { 0 };
    WC.lpfnWndProc = WindowProc;
    WC.hInstance = GetModuleHandleA(0);
    WC.lpszClassName = "FakeBSODWindowClass";
    RegisterClassA(&WC);

    Window = CreateWindowA(WC.lpszClassName, "BSOD", WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, 800, 600, NULL, NULL, WC.hInstance, NULL);

    ShowWindow(Window, TRUE);

    DC = CreateCompatibleDC(NULL);

    BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    BitmapInfo.bmiHeader.biWidth = 800;
    BitmapInfo.bmiHeader.biHeight = -600;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 24;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;
    BitmapInfo.bmiHeader.biSizeImage = 800*600*3;
  
    Bmp = CreateDIBSection(DC, &BitmapInfo, DIB_RGB_COLORS, (void**)&BitmapPixels, NULL, 0);

    LOGFONTA LogFont = { 0 };
    LogFont.lfHeight = 13;
    LogFont.lfWeight = FW_NORMAL;
    LogFont.lfCharSet = DEFAULT_CHARSET;
    LogFont.lfOutPrecision = OUT_OUTLINE_PRECIS;
    LogFont.lfQuality = NONANTIALIASED_QUALITY;
    LogFont.lfPitchAndFamily = VARIABLE_PITCH;
    strcpy(LogFont.lfFaceName, "Lucida Console");
    HFONT Font = CreateFontIndirect(&LogFont);
    SelectObject(DC, Bmp);
    SelectObject(DC, Font);
    ShowCursor(0);
    SetTextColor(DC, RGB(255, 255, 255));
    SetBkColor(DC, RGB(0, 0, 130));

    TimerEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    TimerQueue = CreateTimerQueue();

    CreateTimerQueueTimer( &Timer, TimerQueue, (WAITORTIMERCALLBACK)TimerProc, 0, 1000, 0, 0);

    ChangeDisplaySettingsA(&(DEVMODEA){
        .dmSize = sizeof(DEVMODEA),
        .dmFields = DM_PELSWIDTH | DM_PELSHEIGHT,
        .dmPelsWidth = 800,
        .dmPelsHeight = 600
    }, CDS_FULLSCREEN);

    TimerEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    Len = strlen(Text);

    Bkg = CreateSolidBrush(0x800000);

    MSG Msg = { 0 };
    while (GetMessageA(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessageA(&Msg);
    }

    return 0;
}