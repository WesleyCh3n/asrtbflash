//---------------------------------------------------------------------------

#ifndef ConsoleH
#define ConsoleH
//---------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>

#define CON_COLOR_RED           (FOREGROUND_RED | FOREGROUND_INTENSITY)
#define CON_COLOR_GREEN         (FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define CON_COLOR_BLUE          (FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define CON_COLOR_YELLOW        (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define CON_COLOR_MAGENTA       (FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define CON_COLOR_NORMAL        (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define CON_COLOR_HIGHLIGHT     (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)

#define CON_COLOR_RED_BK           (BACKGROUND_RED | BACKGROUND_INTENSITY)
#define CON_COLOR_GREEN_BK         (BACKGROUND_GREEN | BACKGROUND_INTENSITY)
#define CON_COLOR_BLUE_BK          (BACKGROUND_BLUE | BACKGROUND_INTENSITY)
#define CON_COLOR_YELLOW_BK        (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY)
#define CON_COLOR_MAGENTA_BK       (BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY)
#define CON_COLOR_NORMAL_BK        (0)
#define CON_COLOR_HIGHLIGHT_BK     (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY)

#define CON_COLOR_BLUE_BK_YELLOW_F (BACKGROUND_BLUE | BACKGROUND_INTENSITY) | CON_COLOR_YELLOW
#define CON_COLOR_RED_BK_YELLOW_F  (BACKGROUND_RED | BACKGROUND_INTENSITY) | CON_COLOR_YELLOW

class TConsole
{
    private:
        static TConsole* g_Instance;
        static HANDLE g_hStdOut;
        static HANDLE g_hStdIn;
        static DWORD g_PrevMode;
        TConsole();
        virtual ~TConsole();

        static BOOL WINAPI ConsoleHandler(DWORD signal);
    public:
        static TConsole* GetInstance();
        static void FreeInstance();
        static void SystemCloseButton(bool Enable);
        static DWORD SetTextColor(DWORD Color);
        static void CursorVisible(bool Enable);
        static DWORD MouseEnable(bool Enable);
        static void HideWindow(bool Enable);
};

#endif
