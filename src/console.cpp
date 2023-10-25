//---------------------------------------------------------------------------

#pragma hdrstop

#include "console.hpp"
#include <stdarg.h>

//---------------------------------------------------------------------------

#pragma package(smart_init)

TConsole *TConsole::g_Instance = NULL;
HANDLE TConsole::g_hStdOut = NULL;
HANDLE TConsole::g_hStdIn = NULL;
DWORD TConsole::g_PrevMode = 0;

TConsole *TConsole::GetInstance() {
  if (g_Instance == NULL) {
    g_Instance = new TConsole();
  }
  return g_Instance;
}

void TConsole::FreeInstance() {
  if (g_Instance != NULL) {
    delete g_Instance;
    g_Instance = NULL;
  }
}

TConsole::TConsole() {
  g_hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
  g_hStdIn = GetStdHandle(STD_INPUT_HANDLE);
  GetConsoleMode(g_hStdIn, &g_PrevMode);
  /*
  HANDLE hInput;
  DWORD prev_mode;
  hInput = GetStdHandle(STD_INPUT_HANDLE);
  GetConsoleMode(hInput, &prev_mode);
  SetConsoleMode(hInput, prev_mode & ENABLE_EXTENDED_FLAGS);
  cout<<"The quick edit mode stopped now press any key to re enable it"<<endl;
  _getch();
  SetConsoleMode(hInput, prev_mode);
  cout<<"Quick edit mode reenabled click any key to exit";
  _getch();
  return 0;
  */
}

TConsole::~TConsole() {}

BOOL WINAPI TConsole::ConsoleHandler(DWORD signal) {

  if (signal == CTRL_C_EVENT) {
    // printf("Ctrl-C handled\n"); // do cleanup
    //        SetTextColor(CON_COLOR_RED);
    //        printf("Please wait for a while.\n");
    //        SetTextColor(CON_COLOR_NORMAL);
  }

  return TRUE;
}

void TConsole::SystemCloseButton(bool Enable) {
  if (Enable) {
    SetConsoleCtrlHandler(ConsoleHandler, FALSE);
  } else {
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
  }
  EnableMenuItem(GetSystemMenu(GetConsoleWindow(), false), SC_CLOSE,
                 Enable ? MF_ENABLED : MF_GRAYED);
  DrawMenuBar(GetConsoleWindow());
}

void TConsole::CursorVisible(bool Enable) {
  if (g_hStdOut == NULL) {
    g_hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
  }
  CONSOLE_CURSOR_INFO info;
  info.dwSize = 100;
  info.bVisible = Enable;
  SetConsoleCursorInfo(g_hStdOut, &info);
}

DWORD TConsole::SetTextColor(DWORD color) {
  if (g_hStdOut == NULL) {
    g_hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
  }
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  DWORD oldColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

  if (g_hStdOut != NULL) {
    if (GetConsoleScreenBufferInfo(g_hStdOut, &csbi)) {
      oldColor = csbi.wAttributes;
    }

    if (color == 0x00) {
      oldColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }

    SetConsoleTextAttribute(g_hStdOut, color);
  }
  return oldColor;
}

DWORD TConsole::MouseEnable(bool Enable) {
  DWORD Mode;
  if (Enable == false) {
    Mode = (g_PrevMode & ENABLE_EXTENDED_FLAGS);
  } else {
    Mode = g_PrevMode;
  }
  SetConsoleMode(g_hStdIn, Mode);

  return g_PrevMode;
}

void TConsole::HideWindow(bool Enable) {
  if (Enable) {
    ShowWindow(GetConsoleWindow(), SW_HIDE);
  } else {
    ShowWindow(GetConsoleWindow(), SW_RESTORE);
  }
}
