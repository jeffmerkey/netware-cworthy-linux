/***************************************************************************
*
*   Copyright(c) Jeff V. Merkey 1997-2019.  All rights reserved.
*   Open CWorthy Look Alike Terminal Library.
*
*   Licensed under the GNU Public License v2.
*
*   Permission to use, copy, modify, distribute, and sell this software and its
*   documentation for any purpose is hereby granted without fee, provided that
*   the above copyright notice appear in all copies and that both that
*   copyright notice and this permission notice appear in supporting
*   documentation.  No representations are made about the suitability of
*   this software for any purpose.  It is provided "as is" without express or
*   implied warranty.
*
*   For those folks who liked the Novell NetWare Network Administration
*   tools, this library is an open source version of the Standard Novell CWorthy
*   interface.  The current library works under DOS, Windows, and Linux.
*
*   This library uses the standard DOS screen color attributes which have been
*   mapped to ncurses in Linux, and the Microsoft Windows Terminal API's.
*
*   The DOS build of this library uses the DOS standard 0xB8000 CGA direct
*   screen read/write interface.
*
*   The Linux build maps to ncurses and supports most terminal types including
*   Xterm, linux, and the VT100 or greater terminal types.  The interface can
*   be programmed to support a wide range of terminal types at both compile and
*   run time and to display the output as either simple text or special chars
*   specific to the detected or specified terminal type.  Unicode and other
*   special chars are fully supported for Linux, Xterm, and VT100 or greater
*   terminal types.
*
*   To make the libraries:
*
*   Set the platform build value in cworthy.h.
*   Only chose one option for either DOS, Linux,
*   or Windows NT set to the value of 1.
*
*   #define LINUX_UTIL       1  <- select Linux build
*   #define DOS_UTIL         0
*   #define WINDOWS_NT_UTIL  0
*
*   Linux
*
*   make clean
*   make install
*
*   DOS
*
*   Install the DJGPP Compiler and tools
*   on DOS or Windows.  Tested on DJGPP
*   gcc-4.44 version and gcc-9.0
*
*   make -f Makefile.dos clean
*   make -f Makefile.dos
*
*   WINDOWS
*
*   Install the Windows SDK from Microsoft.
*   Tested on Microsoft Windows SDK
*
*   nmake -f Makefile.windows clean
*   nmake -f Makefile.windows
*
**************************************************************************/

#define __GNU_SOURCE

#include "cworthy.h"

#if (LINUX_UTIL)
#include "netware-screensaver.h"
#endif

ULONG bar_attribute = BLUE | BGWHITE;
ULONG field_attribute = BLUE | BGWHITE;
ULONG field_popup_highlight_attribute = YELLOW | BGBLUE;
ULONG field_popup_normal_attribute = BRITEWHITE | BGBLUE;
ULONG error_attribute = BRITEWHITE | BGMAGENTA;

#if (LINUX_UTIL)
pthread_mutex_t vidmem_mutex;
int has_color = 0;
int linux_term = 0;
int xterm = 0;
int ansi = 0;
BYTE terminal_name[256];
ULONG screensaver;
ULONG sstime = 60 * 10; // default screensaver activates in 10 minutes
#endif

ULONG text_mode = 0;
ULONG mono_mode = 0;
ULONG unicode_mode = 0;
NWSCREEN console_screen =
{
   NULL,                   // VGA video address
   NULL,                   // VGA saved video address
   0,                      // curr_x = 0
   0,                      // curr_y = 0
   80,                     // columns
   25,                     // lines
   0,                      // mode
   0x07,                   // normal video
   0x71,                   // reverse video
   8                       // tab size
};

CWFRAME frame[MAX_MENU];
int screen_x, screen_y;

#if WINDOWS_NT_UTIL
int snprintff(char *buf, int size, const char *fmt, ...)
{
    va_list args;
    register int err;

    va_start(args, fmt);
    err = vsprintf_s(buf, size, fmt, args);
    va_end(args);
    return err;
}
#endif

NWSCREEN *get_console_screen(void)
{
   return &console_screen;
}

int get_screen_lines(void)
{
   return console_screen.nlines;
}

int get_screen_cols(void)
{
   return console_screen.ncols;
}

void set_text_mode(int mode)
{
   text_mode = mode ? 1 : 0;
}

void set_mono_mode(int mode)
{
   mono_mode = mode ? 1 : 0;
}

void set_unicode_mode(int mode)
{
   unicode_mode = mode ? 1 : 0;
}

#if DOS_UTIL
BYTE *get_term_name(void)
{
   return "DOS";
}
#endif

#if WINDOWS_NT_UTIL
BYTE *get_term_name(void)
{
   return "Windows";
}
#endif

#if (LINUX_UTIL)

//  Here we attempt to map the ncurses color pair numbers into
//  something a little more PC friendly.  Most programs that run
//  under DOS and Windows NT use a direct screen write interface
//  that uses coloring codes that conform to the 6845 CRT controller
//  bits.  This interface creates 64 ncurses color pairs with all possible
//  background colors, then provides a table driven interface that
//  allows you to use PC style color attributes.  This allows us to
//  use one consistent color attribute selection interface for all
//  the platforms this library supports (Linux, DOS, and Windows NT/2000).

int color_map[128]=
{
    1,  2,  3,  4,  5,  6,  7,  8, 8,  2,  3,  4,  5,  6,  7,  8,
    9, 10, 11, 12, 13, 14, 15, 16, 16, 10, 11, 12, 13, 14, 15, 16,
   17, 18, 19, 20, 21, 22, 23, 24, 24, 18, 19, 20, 21, 22, 23, 24,
   25, 26, 27, 28, 29, 30, 31, 32, 32, 26, 27, 28, 29, 30, 31, 32,
   33, 34, 35, 36, 37, 38, 39, 40, 40, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 48, 42, 43, 44, 45, 46, 47, 48,
   49, 50, 51, 52, 53, 54, 55, 56, 56, 50, 51, 52, 53, 54, 55, 56,
   57, 58, 59, 60, 61, 62, 53, 64, 64, 58, 59, 60, 61, 62, 53, 64
};

int attr_map[128]=
{
   0, 0, 0, 0, 0, 0, 0, A_BOLD, 0, A_BOLD, A_BOLD, A_BOLD,
   A_BOLD, A_BOLD, A_BOLD, A_BOLD,
   0, 0, 0, 0, 0, 0, 0, A_BOLD, 0, A_BOLD, A_BOLD, A_BOLD,
   A_BOLD, A_BOLD, A_BOLD, A_BOLD,
   0, 0, 0, 0, 0, 0, 0, A_BOLD, 0, A_BOLD, A_BOLD, A_BOLD,
   A_BOLD, A_BOLD, A_BOLD, A_BOLD,
   0, 0, 0, 0, 0, 0, 0, A_BOLD, 0, A_BOLD, A_BOLD, A_BOLD,
   A_BOLD, A_BOLD, A_BOLD, A_BOLD,
   0, 0, 0, 0, 0, 0, 0, A_BOLD, 0, A_BOLD, A_BOLD, A_BOLD,
   A_BOLD, A_BOLD, A_BOLD, A_BOLD,
   0, 0, 0, 0, 0, 0, 0, A_BOLD, 0, A_BOLD, A_BOLD, A_BOLD,
   A_BOLD, A_BOLD, A_BOLD, A_BOLD,
   0, 0, 0, 0, 0, 0, 0, A_BOLD, 0, A_BOLD, A_BOLD, A_BOLD,
   A_BOLD, A_BOLD, A_BOLD, A_BOLD,
   0, 0, 0, 0, 0, 0, 0, A_BOLD, 0, A_BOLD, A_BOLD, A_BOLD,
   A_BOLD, A_BOLD, A_BOLD, A_BOLD
};

ULONG get_color_pair(ULONG attr)
{
   return ((COLOR_PAIR(color_map[attr & 0x7F]) |
	  attr_map[attr & 0x7F] | ((attr & BLINK) ? A_BLINK : 0)));
}

void set_color(ULONG attr)
{
    if (has_color)
       attrset(get_color_pair(attr));
    else
    if (attr == bar_attribute)
       attrset(A_REVERSE);
}

void clear_color(void)
{
    attroff(A_BOLD | A_BLINK | A_REVERSE);
}

int is_xterm(void)
{
   return xterm;
}

int is_linux_term(void)
{
   return linux_term;
}

int is_ansi_term(void)
{
   return ansi;
}

BYTE *get_term_name(void)
{
   return terminal_name;
}

// this function remaps single byte box and line characters into utf8
// unicode characters for display on wide character terminals.  This
// allows the program to store multi byte characters as single byte
// ASCII codes in a screen map for overlapping windows under ncurses.

void mvputc(ULONG row, ULONG col, const chtype ch)
{
   if (text_mode)
   {
      switch (ch & 0xFF)
      {
         // Up Arrow
         case 0x1E:
            mvaddch(row, col, '*');
            break;
         // Down Arrow
         case 0x1F:
            mvaddch(row, col, '*');
            break;

         default:
            mvaddch(row, col, ch > 127 ? ' ' : ch);
            break;
      }
      return;
   }

   switch (ch & 0xFF)
   {
      // solid block
      case 219:
         mvprintw(row, col, "\u2588");
         break;
      // dark shade block
      case 178:
         mvprintw(row, col, "\u2593");
         break;
      // medium shade block
      case 177:
         mvprintw(row, col, "\u2592");
         break;
      // default background fill character
      // light shade block
      case 176:
         mvprintw(row, col, "\u2591");
         break;

      // SINGLE_BORDER
      // Upper Left
      case 218:
         mvprintw(row, col, "\u250c");
         break;
      // Upper Right
      case 191:
         mvprintw(row, col, "\u2510");
         break;
      // Lower Left
      case 192:
         mvprintw(row, col, "\u2514");
         break;
      // Lower Right
      case 217:
         mvprintw(row, col, "\u2518");
         break;
      // Left Frame
      case 195:
         mvprintw(row, col, "\u251c");
         break;
      // Right Frame
      case 180:
         mvprintw(row, col, "\u2524");
         break;
      // Vertical Frame 
      case 179:
         mvprintw(row, col, "\u2502");
         break;
      // Horizontal Frame
      case 196:
         mvprintw(row, col, "\u2500");
         break;

      // Up Arrow
      case 0x1E:
#ifdef UNICODE_SCROLL_CHAR
	 mvprintw(row, col, "\u25b3");
#else
	 mvprintw(row, col, "*");
#endif
         break;
      // Down Arrow
      case 0x1F:
#ifdef UNICODE_SCROLL_CHAR
         mvprintw(row, col, "\u25bd");
#else
         mvprintw(row, col, "*");
#endif
         break;

      // SINGLE_BORDER
      // Upper Left
      case 201:
         mvprintw(row, col, "\u2554");
         break;
      // Upper Right
      case 187:
         mvprintw(row, col, "\u2557");
         break;
      // Lower Left
      case 200:
         mvprintw(row, col, "\u255a");
         break;
      // Lower Right
      case 188:
         mvprintw(row, col, "\u255d");
         break;
      // Left Frame
      case 204:
         mvprintw(row, col, "\u2560");
         break;
      // Right Frame
      case 185:
         mvprintw(row, col, "\u2563");
         break;
      // Vertical Frame
      case 186:
         mvprintw(row, col, "\u2551");
         break;
      // Horizontal Frame
      case 205:
         mvprintw(row, col, "\u2550");
         break;

      default:
         mvaddch(row, col, ch);
         break;
    }
    return;
}

#endif

void screen_write(BYTE *p)
{

#if (DOS_UTIL)
    ScreenUpdate(p);
#endif

#if (LINUX_UTIL)
#endif

}

void hard_xy(ULONG row, ULONG col)
{

#if (WINDOWS_NT_UTIL)
   HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
   COORD coordScreen = { 0, 0 };
   BOOL bSuccess;

   coordScreen.X = (short)col;
   coordScreen.Y = (short)row;

   bSuccess = SetConsoleCursorPosition(hConsole, coordScreen);
#endif

#if (DOS_UTIL)
    ScreenSetCursor(row, col);
#endif

#if (LINUX_UTIL)
    move(row, col);
#endif

    return;
}

void enable_cursor(int insert_mode)
{
#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    CONSOLE_CURSOR_INFO CursorInfo;

    bSuccess = GetConsoleCursorInfo(hConsole, &CursorInfo);
    CursorInfo.bVisible = TRUE;
    bSuccess = SetConsoleCursorInfo(hConsole, &CursorInfo);
#endif

#if (LINUX_UTIL)
    insert_mode ? curs_set(2) : curs_set(1);
#endif

}

void disable_cursor(void)
{
#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    CONSOLE_CURSOR_INFO CursorInfo;

    bSuccess = GetConsoleCursorInfo(hConsole, &CursorInfo);
    CursorInfo.bVisible = 0;
    bSuccess = SetConsoleCursorInfo(hConsole, &CursorInfo);
#endif

#if (DOS_UTIL)
    ScreenSetCursor(100, 100);
#endif

#if (LINUX_UTIL)
    curs_set(0);  // turn off the cursor
#endif

}

#if (LINUX_UTIL)
void refresh_screen(void)
{
   if (pthread_mutex_lock(&vidmem_mutex))
      return;
   refresh();
   pthread_mutex_unlock(&vidmem_mutex);
   return;
}
#endif

ULONG init_cworthy(void)
{

#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */

    /* get the number of character cells in the current buffer */
    bSuccess = GetConsoleScreenBufferInfo( hConsole, &csbi );

    console_screen.ncols = csbi.srWindow.Right - csbi.srWindow.Left;
    console_screen.nlines = csbi.srWindow.Bottom - csbi.srWindow.Top;

    if ((console_screen.ncols < 80) || (console_screen.nlines < 19))
    {
        printf("Your display is too small to run cworthy (%d lines, %d cols).\n"
	       "The program requires a minimum of"
	       " 19 lines and 80 columns\n",
	       console_screen.nlines, console_screen.ncols);
	return -1;
    }

    console_screen.p_vidmem = malloc(console_screen.ncols *
				   console_screen.nlines * 2);
    if (!console_screen.p_vidmem)
       return -1;

    console_screen.p_saved = malloc(console_screen.ncols *
				  console_screen.nlines * 2);
    if (!console_screen.p_saved)
    {
       free(console_screen.p_vidmem);
       return -1;
    }

    clear_screen(&console_screen);
    disable_cursor();
    return 0;
#endif

#if (DOS_UTIL)
     console_screen.ncols = ScreenCols();
     console_screen.nlines = ScreenRows();

     if ((console_screen.ncols < 80) || (console_screen.nlines < 19))
     {
        printf("Your display is too small to run cworthy (%d lines, %d cols).\n"
	       "The program requires a minimum of"
	       " 19 lines and 80 columns\n",
	       console_screen.nlines, console_screen.ncols);
	return -1;
     }

     console_screen.p_vidmem = malloc(console_screen.ncols *
				    console_screen.nlines * 2);
     if (!console_screen.p_vidmem)
	return -1;

     console_screen.p_saved = malloc(console_screen.ncols *
				    console_screen.nlines * 2);
     if (!console_screen.p_saved)
     {
	free(console_screen.p_vidmem);
	return -1;
     }

     ScreenRetrieve(console_screen.p_vidmem);
     ScreenRetrieve(console_screen.p_saved);
     ScreenGetCursor(&screen_x, &screen_y);

     disable_cursor();
#endif

#if (LINUX_UTIL)
     register int i, pair;
     register BYTE *tname;
     unsigned long w;
     FILE *f;
     char wait[100];
     int bg_colors[8]=
     {
        COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN,
        COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE
     };

     pthread_mutex_init(&vidmem_mutex, NULL);

     // setlocale must be called to enable utf8 (unicode) character
     // display settings.
     setlocale(LC_ALL, "");

     initscr();
     nonl();
     intrflush(stdscr, FALSE);
     keypad(stdscr, TRUE);
     noecho();

     tname = (BYTE *)termname();
     if (tname)
     {
        memset(terminal_name, 0, 256);
        strncpy((char *)terminal_name, (const char *)tname, 255);
        if (strcasestr((const char *)terminal_name, "linux"))
           linux_term = TRUE;
        if (strcasestr((const char *)terminal_name, "xterm"))
           xterm = TRUE;
        if (strcasestr((const char *)terminal_name, "ansi"))
           ansi = TRUE;
     }

     if ((COLS < 80) || (LINES < 19))
     {
        endwin();
        printf("Your display is too small to run cworthy (%d lines, %d cols).\n"
	       "The program requires a minimum of"
	       " 19 lines and 80 columns\n", LINES, COLS);
	return -1;
     }

     console_screen.ncols = COLS;
     console_screen.nlines = LINES;

     console_screen.p_vidmem = (BYTE *)malloc(console_screen.ncols *
				            console_screen.nlines * 2);
     if (!console_screen.p_vidmem)
     {
        endwin();
	return -1;
     }

     console_screen.p_saved = (BYTE *)malloc(console_screen.ncols *
				           console_screen.nlines * 2);
     if (!console_screen.p_saved)
     {
        free(console_screen.p_vidmem);
        endwin();
        return -1;
     }

     // if the terminal does not support colors, or if the
     // terminal cannot support at least eight primary colors
     // for foreground/background color pairs, then default
     // the library to use ASCII characters < 127 (7 bit), disable
     // ncurses color attributes, and do not attempt to use
     // the alternate character set for graphic characters.

     if (has_colors() && !mono_mode)
     {
	if (start_color() == OK)
	{
	   if (COLORS >= 8)
	   {
	      has_color = TRUE;
              pair = 1;

              // We create our color pairs in the order defined
              // by the PC based text attribute color scheme.  We do
              // this to make it relatively simple to use a table
              // driven method for mapping the PC style text attributes
              // to ncurses.

              for (i=0; i < 8; i++)
              {
	         init_pair(pair++, COLOR_BLACK, bg_colors[i]);
	         init_pair(pair++, COLOR_BLUE, bg_colors[i]);
	         init_pair(pair++, COLOR_GREEN, bg_colors[i]);
	         init_pair(pair++, COLOR_CYAN, bg_colors[i]);
	         init_pair(pair++, COLOR_RED, bg_colors[i]);
	         init_pair(pair++, COLOR_MAGENTA, bg_colors[i]);
	         init_pair(pair++, COLOR_YELLOW, bg_colors[i]);
	         init_pair(pair++, COLOR_WHITE, bg_colors[i]);
	      }
	   }
	}
     }

     wclear(stdscr);
     disable_cursor();
     refresh_screen();

     f = fopen("/sys/module/kernel/parameters/consoleblank", "r");
     if (f != NULL)
     {
        fgets(wait, 98, f);
        fclose(f);
        sscanf(wait, "%lu", &w);
#if VERBOSE
	printf("console blank timer: %lu\n", w);
#endif
     }
     // disable screen blanking and enable
     // cworthy screensaver
     if (w)
        system("setterm -blank 0");
#endif
     return 0;
}

ULONG release_cworthy(void)
{
#if (WINDOWS_NT_UTIL)
    clear_screen(&console_screen);
    enable_cursor(0);
    if (console_screen.p_vidmem)
       free(console_screen.p_vidmem);
    if (console_screen.p_saved)
       free(console_screen.p_saved);
#endif

    clear_screen(&console_screen);

#if (DOS_UTIL)
    clear_screen(&console_screen);
    enable_cursor(0);
    ScreenSetCursor(screen_x, screen_y);
    screen_write(console_screen.p_saved);
    if (console_screen.p_vidmem)
       free(console_screen.p_vidmem);
    if (console_screen.p_saved)
       free(console_screen.p_saved);
#endif

#if (LINUX_UTIL)
    pthread_mutex_destroy(&vidmem_mutex);

    enable_cursor(0);
    endwin();

    // reset terminal escape sequence
    printf("%c%c", 0x1B, 'c');

    if (console_screen.p_vidmem)
       free(console_screen.p_vidmem);
    if (console_screen.p_saved)
       free(console_screen.p_saved);

    // enable screen blanking
#if 0
    system("setterm -blank 10");
#endif
#endif
    return 0;
}

void copy_data(ULONG *src, ULONG *dest, ULONG len)
{
    memcpy(dest, src, len);
#if 0
   register ULONG i;

   for (i=0; i < ((len + 3) / 4); i++)
      *dest++ = *src++;
   return;
#endif
}

void set_data(ULONG *dest, ULONG value, ULONG len)
{
   memset(dest, value, len);
#if 0
   register ULONG i;

   for (i=0; i < ((len + 3) / 4); i++)
      *dest++ = value;
   return;
#endif
}

void set_data_b(BYTE *dest, BYTE value, ULONG len)
{
   memset(dest, value, len);
#if 0
   register ULONG i;

   for (i=0; i < len; i++)
      *dest++ = value;
   return;
#endif

}

#if (LINUX_UTIL)
int _kbhit(void)
{
   int STDIN = 0, bytes = 0;
   struct termios init;
   struct termios term;

   tcgetattr(STDIN, &init);
   tcgetattr(STDIN, &term);
   term.c_lflag &=	~ICANON;
   tcsetattr(STDIN, TCSANOW, &term);

   setbuf(stdin, NULL);
   ioctl(STDIN, FIONREAD, &bytes);

   tcsetattr(STDIN, TCSANOW, &init);
   return bytes;
}
#endif

#if (LINUX_UTIL)
ULONG set_screensaver_interval(ULONG seconds)
{
   register ULONG t = sstime;
   sstime = seconds;
   return t;
}
#endif

ULONG get_key(void)
{
#if (WINDOWS_NT_UTIL)
    ULONG c;

    c = _getch();
    if ((!c) || ((c & 0xFF) == 0xE0))
    {
       c = _getch();
       c |= 0x0100; // identify this key as special
       return c;
    }
    return c;
#endif

#if (DOS_UTIL)
    ULONG c;

    c = getch();
    if ((!c) || ((c & 0xFF) == 0xE0))
    {
       c = getch();
       c |= 0x0100; // identify this key as special
       return c;
    }
    return c;
#endif

#if (LINUX_UTIL)
    ULONG c;
    struct timespec ts = { 0, 10000000L };
    static ULONG sscount = 0;

    refresh_screen();
    while (!_kbhit())
    {
       fflush(stdout);
       nanosleep(&ts, NULL);
       // convert ns to seconds
       if ((sscount++ / 100) > sstime)
       {
          if (screensaver == FALSE)
	  {
	     screensaver = TRUE;
             wclear(stdscr);
	     disable_cursor();
	     refresh_screen();
             cworthy_netware_screensaver();
	  }
	  sscount = 0;
       }
    }
    // read buffered key
    c = getch();

    if (screensaver == TRUE) {
       screensaver = FALSE;
       restore_screen();
       refresh_screen();
       // if screensaver was active swallow the key
       c = 0;
    }
    refresh_screen();
    return c;
#endif

}

void set_xy(NWSCREEN *screen, ULONG row, ULONG col)
{
    screen->crnt_row = row;
    screen->crnt_column = col;
    hard_xy(row, col);
    return;
}

void get_xy(NWSCREEN *screen, ULONG *row, ULONG *col)
{
    *row = screen->crnt_row;
    *col = screen->crnt_column;
    return;
}

void clear_screen(NWSCREEN *screen)
{
#if (WINDOWS_NT_UTIL)
   HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
   COORD coordScreen = { 0, 0 };
   BOOL bSuccess;
   DWORD cCharsWritten;
   CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */
   DWORD dwConSize;                 /* number of character cells in
				       the current buffer */

   /* get the number of character cells in the current buffer */

   bSuccess = GetConsoleScreenBufferInfo( hConsole, &csbi );
   dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

   /* fill the entire screen with blanks */

   bSuccess = FillConsoleOutputCharacter( hConsole, (TCHAR) ' ',
      dwConSize, coordScreen, &cCharsWritten );

   /* get the current text attribute */

   bSuccess = GetConsoleScreenBufferInfo( hConsole, &csbi );

   /* now set the buffer's attributes accordingly */

   bSuccess = FillConsoleOutputAttribute( hConsole, csbi.wAttributes,
      dwConSize, coordScreen, &cCharsWritten );

   /* put the cursor at (0, 0) */

   bSuccess = SetConsoleCursorPosition( hConsole, coordScreen );

   return;
#endif

#if (DOS_UTIL | LINUX_UTIL)
   register ULONG fill;
   register BYTE ch;


#if LINUX_UTIL
   if (pthread_mutex_lock(&vidmem_mutex))
      return;
#endif
   ch = (BYTE)(screen->norm_vid & 0xFF);
   fill = 0x00200020;  //  ' ' space
   fill = fill | (ULONG)(ch << 24);
   fill = fill | (ULONG)(ch << 8);
   set_data((ULONG *)screen->p_vidmem, fill,
	    (screen->ncols * screen->nlines * 2));

#if (DOS_UTIL)
   screen_write(console_screen.p_vidmem);
#endif

#if (LINUX_UTIL)
   wclear(stdscr);
   pthread_mutex_unlock(&vidmem_mutex);
#endif

#endif

   return;
}

void move_string(NWSCREEN *screen,
		 ULONG srcRow, ULONG srcCol,
		 ULONG destRow, ULONG destCol,
		 ULONG length)
{
    register ULONG i;
    register BYTE *src_v, *dest_v, c;
    int attr = 0;

#if LINUX_UTIL
   if (pthread_mutex_lock(&vidmem_mutex))
      return;
#endif
    src_v = screen->p_vidmem;
    src_v += (srcRow * (screen->ncols * 2)) + srcCol * 2;

    dest_v = screen->p_vidmem;
    dest_v += (destRow * (screen->ncols * 2)) + destCol * 2;

    for (i=0; i < length; i++)
    {
       c = *src_v;
       attr = *(src_v + 1);
       *dest_v++ = *src_v++;
       *dest_v++ = *src_v++;

#if (LINUX_UTIL)
       set_color(attr);
       mvputc(destRow, destCol++, c);
       clear_color();
#endif
#if (DOS_UTIL)
       ScreenPutChar(c, attr, destCol++, destRow);
#endif
    }
#if (LINUX_UTIL)
    refresh();
    pthread_mutex_unlock(&vidmem_mutex);
#endif

}


void put_string(NWSCREEN *screen, const char *s, BYTE *attr_array,
		ULONG row, ULONG col, ULONG attr)
{
#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten;
    COORD coordScreen;

    coordScreen.X = (short)col;
    coordScreen.Y = (short)row;

    bSuccess = WriteConsoleOutputCharacter( hConsole, (LPSTR) s,
				lstrlen(s), coordScreen, &cCharsWritten );

    bSuccess = FillConsoleOutputAttribute( hConsole, (WORD)attr,
				lstrlen(s), coordScreen, &cCharsWritten );
#endif

#if (DOS_UTIL | LINUX_UTIL)
    register BYTE *v, c;
    register ULONG len = strlen((const char *)s), count;

#if LINUX_UTIL
   if (pthread_mutex_lock(&vidmem_mutex))
      return;
#endif
    count = 0;
    v = screen->p_vidmem;
    v += (row * (screen->ncols * 2)) + col * 2;
    while (*s)
    {
       if (count++ >
          ((len <= (screen->ncols - col)) ? len : (screen->ncols - col)))
	  break;

       c = *s;
       *v++ = *s++;
       *v++ = attr;

#if (LINUX_UTIL)
       set_color(attr);
       mvputc(row, col++, c);
       clear_color();
#endif
#if (DOS_UTIL)
       ScreenPutChar(c, attr, col++, row);
#endif
    }
#if (LINUX_UTIL)
    pthread_mutex_unlock(&vidmem_mutex);
#endif

#endif

}

void put_string_transparent(NWSCREEN *screen, const char *s, BYTE *attr_array,
			    ULONG row, ULONG col, ULONG attr)
{

#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten;
    COORD coordScreen;

    coordScreen.X = (short)col;
    coordScreen.Y = (short)row;

    bSuccess = WriteConsoleOutputCharacter( hConsole, (LPSTR) s,
				    lstrlen(s), coordScreen, &cCharsWritten );

    if (attr)
       bSuccess = FillConsoleOutputAttribute( hConsole, (WORD)attr,
				    lstrlen(s), coordScreen, &cCharsWritten );
#endif

#if (DOS_UTIL | LINUX_UTIL)
    register BYTE *v, c;
    register ULONG len = strlen((const char *)s), count, color;

#if LINUX_UTIL
   if (pthread_mutex_lock(&vidmem_mutex))
      return;
#endif
    count = 0;
    v = screen->p_vidmem;
    v += (row * (screen->ncols * 2)) + col * 2;
    while (*s)
    {
       if (count++ >
	  ((len <= (screen->ncols - col)) ? len : (screen->ncols - col)))
	  break;

       c = *s;
       *v++ = *s++;
       if (attr)
	  *v |= attr;
       color = *v++;

#if (LINUX_UTIL)
       set_color(color);
       mvputc(row, col++, c);
       clear_color();
#endif
#if (DOS_UTIL)
       ScreenPutChar(c, color,
		     col++, row);
#endif
    }
#if (LINUX_UTIL)
    pthread_mutex_unlock(&vidmem_mutex);
#endif

#endif

}

void put_string_cleol(NWSCREEN *screen, const char *s, BYTE *attr_array,
		      ULONG row, ULONG attr)
{

#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten;
    COORD coordScreen;
    ULONG len = lstrlen(s);
    ULONG left = 0;

    coordScreen.X = (short)0;
    coordScreen.Y = (short)row;

    bSuccess = WriteConsoleOutputCharacter( hConsole, (LPSTR) s,
		       len, coordScreen, &cCharsWritten );

    bSuccess = FillConsoleOutputAttribute( hConsole, (WORD)attr,
		       len, coordScreen, &cCharsWritten );

    if (screen->ncols >= len)
       left = (screen->ncols - len);

    if (left)
    {
       coordScreen.X = (short)len;
       coordScreen.Y = (short)row;

       bSuccess = FillConsoleOutputCharacter( hConsole, (TCHAR) ' ',
				    left, coordScreen, &cCharsWritten );

       bSuccess = FillConsoleOutputAttribute( hConsole, (WORD)attr,
				    left, coordScreen, &cCharsWritten );
    }

#endif

#if (DOS_UTIL | LINUX_UTIL)
    register ULONG i;
    register BYTE *v, c;

#if LINUX_UTIL
   if (pthread_mutex_lock(&vidmem_mutex))
      return;
#endif
    v = screen->p_vidmem;
    v += (row * (screen->ncols * 2)) + 0 * 2;
    for (i = 0; i < screen->ncols; i++)
    {
       if (*s == '\0')
       {
          c = ' ';
	  *v++ = ' ';
	  *v++ = attr;
       }
       else
       {
	  c = *s;
	  *v++ = *s++;
	  *v++ = attr;
       }
#if (LINUX_UTIL)
       set_color(attr_array && attr_array[i] && attr != bar_attribute
		 ? attr_array[i] : attr);
       mvputc(row, i, c);
       clear_color();
#endif
#if (DOS_UTIL)
       ScreenPutChar(c, attr_array && attr_array[i] && attr != bar_attribute
		     ? attr_array[i] : attr,
		     i, row);
#endif
    }
#if (LINUX_UTIL)
    pthread_mutex_unlock(&vidmem_mutex);
#endif

#endif

}

void put_string_to_length(NWSCREEN *screen, const char *s, BYTE *attr_array,
			  ULONG row, ULONG col, ULONG attr, ULONG len)
{

#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten;
    COORD coordScreen;
    ULONG string_len = lstrlen(s);
    ULONG left = 0;

    if (len > string_len)
       left = (len - string_len);
    else
       string_len = len;

    coordScreen.X = (short)col;
    coordScreen.Y = (short)row;

    bSuccess = WriteConsoleOutputCharacter( hConsole, (LPSTR) s,
				    string_len, coordScreen, &cCharsWritten );

    bSuccess = FillConsoleOutputAttribute( hConsole, (WORD)attr,
				    string_len, coordScreen, &cCharsWritten );

    if (left)
    {
       coordScreen.X = (short)(col + string_len);
       coordScreen.Y = (short)row;

       bSuccess = FillConsoleOutputCharacter( hConsole, (TCHAR) ' ',
				    left, coordScreen, &cCharsWritten );

       bSuccess = FillConsoleOutputAttribute( hConsole, (WORD)attr,
				    left, coordScreen, &cCharsWritten );
    }

#endif

#if (DOS_UTIL | LINUX_UTIL)
    register ULONG i, j;
    register BYTE *v, c;

#if LINUX_UTIL
   if (pthread_mutex_lock(&vidmem_mutex))
      return;
#endif
    v = screen->p_vidmem;
    v += (row * (screen->ncols * 2)) + col * 2;
    for (j=col,i=0; i < len && i < (screen->ncols - j); i++)
    {
       if (*s == '\0')
       {
	  c = ' ';
	  *v++ = ' ';
	  *v++ = attr;
       }
       else
       {
	  c = *s;
	  *v++ = *s++;
	  *v++ = attr;
       }
#if (LINUX_UTIL)
       set_color(attr_array && attr_array[i] && attr != bar_attribute
		 ? attr_array[i] : attr);
       mvputc(row, col++, c);
       clear_color();
#endif
#if (DOS_UTIL)
       ScreenPutChar(c, attr_array && attr_array[i] && attr != bar_attribute
		     ? attr_array[i] : attr,
		     col++, row);
#endif
    }
#if (LINUX_UTIL)
    pthread_mutex_unlock(&vidmem_mutex);
#endif

#endif

}

void put_char_direct(NWSCREEN *screen, int c, ULONG row, ULONG col, ULONG attr)
{
#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten;
    COORD coordScreen;

    coordScreen.X = (short)col;
    coordScreen.Y = (short)row;

    bSuccess = FillConsoleOutputCharacter( hConsole, (TCHAR) c,
				    1, coordScreen, &cCharsWritten );

    bSuccess = FillConsoleOutputAttribute( hConsole, (WORD)attr,
				    1, coordScreen, &cCharsWritten );
#endif

#if (DOS_UTIL | LINUX_UTIL)

#if LINUX_UTIL
   if (pthread_mutex_lock(&vidmem_mutex))
      return;
#endif

#if (DOS_UTIL)
    ScreenPutChar(c, attr, col, row);
#endif

#if (LINUX_UTIL)
    set_color(attr);
    mvputc(row, col, c);
    clear_color();
    pthread_mutex_unlock(&vidmem_mutex);
#endif

#endif

    return;
}

void put_char(NWSCREEN *screen, int c, ULONG row, ULONG col, ULONG attr)
{
#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten;
    COORD coordScreen;

    coordScreen.X = (short)col;
    coordScreen.Y = (short)row;

    bSuccess = FillConsoleOutputCharacter( hConsole, (TCHAR) c,
				    1, coordScreen, &cCharsWritten );

    bSuccess = FillConsoleOutputAttribute( hConsole, (WORD)attr,
				    1, coordScreen, &cCharsWritten );

#endif

#if (DOS_UTIL | LINUX_UTIL)
    register BYTE *v;

    if (col >= screen->ncols)
       return;

    if (row >= screen->nlines)
       return;

#if LINUX_UTIL
   if (pthread_mutex_lock(&vidmem_mutex))
      return;
#endif
    v = screen->p_vidmem;
    v += (row * (screen->ncols * 2)) + col * 2;
    *v++ = c;
    *v = attr;

#if (DOS_UTIL)
    ScreenPutChar(c, attr, col, row);
#endif

#if (LINUX_UTIL)
    set_color(attr);
    mvputc(row, col, c);
    clear_color();
    pthread_mutex_unlock(&vidmem_mutex);
#endif

#endif

    return;
}

int get_char(NWSCREEN *screen, ULONG row, ULONG col)
{
#if (WINDOWS_NT_UTIL)
    BYTE v[2];
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten;
    COORD coordScreen;

    coordScreen.X = (short)col;
    coordScreen.Y = (short)row;

    bSuccess = ReadConsoleOutputCharacter( hConsole, (LPSTR) &v,
				    1, coordScreen, &cCharsWritten );
    return (BYTE) v[0];

#endif

#if (DOS_UTIL | LINUX_UTIL)
   register BYTE *v;

#if LINUX_UTIL
   if (pthread_mutex_lock(&vidmem_mutex))
      return ' ';
#endif
   v = screen->p_vidmem;
   v += (row * (screen->ncols * 2)) + col * 2;
#if LINUX_UTIL
   pthread_mutex_unlock(&vidmem_mutex);
#endif

   return (BYTE)(*v);
#endif

}

int get_char_attribute(NWSCREEN *screen, ULONG row, ULONG col)
{
#if (WINDOWS_NT_UTIL)
    int v[2];
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten;
    COORD coordScreen;

    coordScreen.X = (short)col;
    coordScreen.Y = (short)row;

    bSuccess = ReadConsoleOutputAttribute( hConsole, (LPWORD) &v,
				    1, coordScreen, &cCharsWritten );
    return (BYTE) v[0];

#endif

#if (DOS_UTIL | LINUX_UTIL)
   register BYTE *v;

#if LINUX_UTIL
   if (pthread_mutex_lock(&vidmem_mutex))
      return 0;
#endif
   v = screen->p_vidmem;
   v += (row * (screen->ncols * 2)) + col * 2;
   v++;
#if LINUX_UTIL
   pthread_mutex_unlock(&vidmem_mutex);
#endif

   return (BYTE)(*v);
#endif

}

void put_char_cleol(NWSCREEN *screen, int c, ULONG row, ULONG attr)
{
#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten;
    COORD coordScreen;

    coordScreen.X = (short)0;
    coordScreen.Y = (short)row;

    bSuccess = FillConsoleOutputCharacter( hConsole, (TCHAR) c,
				    screen->ncols, coordScreen,
				    &cCharsWritten );

    bSuccess = FillConsoleOutputAttribute( hConsole, (WORD)attr,
				    screen->ncols, coordScreen,
				    &cCharsWritten );
#endif

#if (DOS_UTIL | LINUX_UTIL)
    register ULONG i;
    for (i=0; i < screen->ncols; i++)
       put_char(screen, c, row, i, attr);

#endif
}

void put_char_length(NWSCREEN *screen, int c, ULONG row, ULONG col, ULONG attr,
		     ULONG len)
{
#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten;
    COORD coordScreen;

    coordScreen.X = (short)0;
    coordScreen.Y = (short)row;

    bSuccess = FillConsoleOutputCharacter( hConsole, (TCHAR) c,
				    len, coordScreen, &cCharsWritten );

    bSuccess = FillConsoleOutputAttribute( hConsole, (WORD)attr,
				    len, coordScreen, &cCharsWritten );
#endif

#if (DOS_UTIL | LINUX_UTIL)
    register ULONG i;
    for (i=0; i < len && (i + col) < screen->ncols; i++)
       put_char(screen, c, row, col + i, attr);
#endif
}

ULONG scroll_display(NWSCREEN *screen, ULONG row, ULONG col,
		     ULONG lines, ULONG cols, ULONG up)
{
#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL fSuccess;
    SMALL_RECT srctScrollRect, srctClipRect;
    CHAR_INFO chiFill;
    COORD coordDest;

    if (!cols || !lines)
       return -1;

    if (col > screen->ncols - 1)
       return -1;

    if (cols > screen->ncols)
       return -1;

    if (row > screen->nlines - 1)
       return -1;

    if (lines > screen->nlines)
       return -1;

    srctScrollRect.Top = (short)row;
    srctScrollRect.Bottom = (short)(row + lines - 1);
    srctScrollRect.Left = (short)col;
    srctScrollRect.Right = (short)(col + cols - 1);

    /* The destination for the scroll rectangle is one row up. */

    if (up)
    {
       coordDest.Y = (short)(row - 1);
       coordDest.X = (short)col;
    }
    else
    {
       coordDest.Y = (short)(row + 1);
       coordDest.X = (short)col;
    }

    /*
     * The clipping rectangle is the same as the scrolling rectangle.
     * The destination row is left unchanged.
     */

    srctClipRect = srctScrollRect;

    chiFill.Attributes = (unsigned short)screen->norm_vid;
    chiFill.Char.AsciiChar = ' ';

    fSuccess = ScrollConsoleScreenBuffer(
	hConsole,        /* screen buffer handle     */
	&srctScrollRect, /* scrolling rectangle      */
	&srctClipRect,   /* clipping rectangle       */
	coordDest,       /* top left destination cell*/
	&chiFill);       /* fill character and color */

    if (!fSuccess)
       return -1;

    return 0;
#endif

#if (DOS_UTIL | LINUX_UTIL)
    register ULONG i, tCol, tRow;

    if (!cols || !lines)
       return -1;

    if (col > screen->ncols - 1)
       return -1;

    if (cols > screen->ncols)
       return -1;

    if (row > screen->nlines - 1)
       return -1;

    if (lines > screen->nlines)
       return -1;

    if (up)
    {
       tCol = col;
       tRow = row;
       for (i=1; i < lines; i++)
       {
	  move_string(screen,
			tRow + 1,
			tCol,
			tRow,
			tCol,
			cols);
	  tRow++;
       }
       for (i=0; i < cols; i++)
       {
	  put_char(screen,
		     ' ',
		     tRow,
		     tCol + i,
		     screen->norm_vid);
       }
    }
    else
    {
       tCol = col;
       tRow = row + (lines - 1);
       for (i=1; i < lines; i++)
       {
	  move_string(screen,
			tRow - 1,
			tCol,
			tRow,
			tCol,
			cols);
	  tRow--;
       }
       for (i=0; i < cols; i++)
       {
	  put_char(screen,
		     ' ',
		     row,
		     tCol + i,
		     screen->norm_vid);
       }
    }
#endif
    return 0;
}

ULONG field_set_xy(ULONG num, ULONG row, ULONG col)
{
    char display_buffer[COLS + 1];

    if (!frame[num].owner)
       return -1;

    if (!frame[num].screen)
       return -1;

    if (row >= (ULONG)frame[num].el_count)
       return -1;

    if (row < (ULONG)frame[num].top) {
       frame[num].top = row;
       frame[num].bottom = frame[num].top + frame[num].window_size;
    } else if (row >= (frame[num].top + frame[num].window_size)) {
       frame[num].top = row - frame[num].top;
       frame[num].bottom = frame[num].top + frame[num].window_size;
    }
#if VERBOSE
    printw("row: %d  col: %d  top: %d  bottom: %d win: %d\n", row, col,
	   frame[num].top, frame[num].bottom, frame[num].window_size);
#endif
    snprintf((char *)display_buffer, sizeof(display_buffer),
             "row: %lu  col: %lu  top: %ld  bottom: %ld "
	     "win: %lu  x: %ld y: %ld",  row, col, frame[num].top,
	     frame[num].bottom, frame[num].window_size,
	     row - frame[num].top, col);
    write_screen_comment_line(get_console_screen(),
			      (const char *)display_buffer,
			      BLUE | BGWHITE);

    update_static_portal(num);
    return (frame_set_xy(num, row - frame[num].top, col));

}

ULONG frame_set_xy(ULONG num, ULONG row, ULONG col)
{
    if (!frame[num].owner)
       return -1;

    if (!frame[num].screen)
       return -1;

    if (col >= frame[num].screen->ncols)
       return -1;

    if (row >= frame[num].screen->nlines)
       return -1;

    if ((row + frame[num].start_row) >= frame[num].screen->nlines)
       return -1;

    if ((col + frame[num].start_column) >= frame[num].screen->ncols)
       return -1;

    frame[num].pcur_row = row;
    frame[num].pcur_column = col;

    if (strlen((const char *)frame[num].header))
       set_xy(frame[num].screen,
	   frame[num].start_row + 3 + row,
	   frame[num].start_column + 2 + col);
    else
       set_xy(frame[num].screen,
	   frame[num].start_row + 1 + row,
	   frame[num].start_column + 2 + col);
    return 0;

}

ULONG frame_get_xy(ULONG num, ULONG *row, ULONG *col)
{
    if (!frame[num].owner)
       return -1;

    *row = frame[num].pcur_row;
    *col = frame[num].pcur_column;
    return 0;
}

void scroll_menu(ULONG num, ULONG up)
{
    register ULONG row, col;

    if (strlen((const char *)frame[num].header))
       row = frame[num].start_row + 3;
    else
       row = frame[num].start_row + 1;

    col = frame[num].start_column + 1;

    scroll_display(frame[num].screen,
		  row,
		  col,
		  frame[num].end_row - row,
		  frame[num].end_column - col,
		  up);
    return;
}

ULONG get_resp(ULONG num)
{
    register ULONG key, row, col, width, temp;
    register ULONG i, ccode;

    if (strlen((const char *)frame[num].header))
       row = frame[num].start_row + 3;
    else
       row = frame[num].start_row + 1;

    col = frame[num].start_column + 1;
    width = frame[num].end_column - frame[num].start_column;
    if (width >= 1)
       width -= 1;
    frame[num].top = 0;
    frame[num].bottom = frame[num].top + frame[num].window_size;

    temp = frame[num].choice;
    frame[num].choice = 0;
    frame[num].index = 0;
    for (i=0; i < temp; i++)
    {
       frame[num].choice++;
       frame[num].index++;

       if (frame[num].index >= (long)frame[num].el_count)
	  frame[num].index--;

       if (frame[num].index >= (long)frame[num].window_size)
       {
	  frame[num].index--;
	  if (frame[num].choice < (long)frame[num].el_count)
	  {
	     frame[num].top++;
	     frame[num].bottom = frame[num].top +
		     frame[num].window_size;
	     scroll_menu(num, 1);
	  }
       }

       if (frame[num].choice >= (long)frame[num].el_count)
	  frame[num].choice--;
    }

    for (;;)
    {
       if (frame[num].el_strings[frame[num].choice])
       {
	  if (frame[num].scroll_frame)
          {
	     put_char(frame[num].screen, ' ',
		      row + frame[num].index, col,
		      bar_attribute);

	     put_char(frame[num].screen,
		      frame[num].scroll_frame,
		      row + frame[num].index, col + 1,
		      bar_attribute);

	     put_string_to_length(frame[num].screen,
		    (const char *)frame[num].el_strings[frame[num].choice],
	            frame[num].el_attr[frame[num].choice],
		    row + frame[num].index, col + 2,
		    bar_attribute,
		    width - 2);
	  }
	  else
	  {
	     put_string_to_length(frame[num].screen,
		    (const char *)frame[num].el_strings[frame[num].choice],
                    frame[num].el_attr[frame[num].choice],
		    row + frame[num].index, col,
		    bar_attribute, width);
          }
       }

       if (frame[num].el_count > frame[num].window_size &&
                                          frame[num].top)
       {
	  put_char(frame[num].screen,
		  frame[num].up_char,
		  row,
		  col,
		  get_char_attribute(frame[num].screen, row, col));
       }
       else
       {
	  put_char(frame[num].screen,
		  ' ',
		  row,
		  col,
		  get_char_attribute(frame[num].screen, row, col));
       }

       if (frame[num].el_count > frame[num].window_size
	  && frame[num].bottom < (long)frame[num].el_count)
       {
	  put_char(frame[num].screen,
		  frame[num].down_char,
		  row + frame[num].window_size - 1,
		  col,
		  get_char_attribute(frame[num].screen,
				     row + frame[num].window_size - 1,
				     col));
       }
       else
       {
	  put_char(frame[num].screen,
		  ' ',
		  row + frame[num].window_size - 1,
		  col,
		  get_char_attribute(frame[num].screen,
				     row + frame[num].window_size - 1,
				     col));
       }

       key = get_key();
       if (frame[num].key_mask)
	  continue;

       if (frame[num].el_strings[frame[num].choice])
       {
	  if (frame[num].scroll_frame)
	  {
	     put_char(frame[num].screen,
	              ' ',
		      row + frame[num].index, col,
		      frame[num].fill_color |
		      frame[num].text_color);

	     put_char(frame[num].screen,
	              frame[num].scroll_frame,
		      row + frame[num].index, col + 1,
		      frame[num].fill_color |
		      frame[num].text_color);

	     put_string_to_length(frame[num].screen,
		    (const char *)frame[num].el_strings[frame[num].choice],
                    frame[num].el_attr[frame[num].choice],
		    row + frame[num].index, col + 2,
		    frame[num].fill_color |
		    frame[num].text_color,
		    width - 2);
	  }
	  else
	  {
	     put_string_to_length(frame[num].screen,
		    (const char *)frame[num].el_strings[frame[num].choice],
                    frame[num].el_attr[frame[num].choice],
		    row + frame[num].index, col,
		    frame[num].fill_color |
		    frame[num].text_color, width);
	  }
       }

       switch (key)
       {
          // screensaver return key
          case 0:
             break;

          // repaint screen
          case ' ':
             restore_screen();
             break;

	  case ENTER:
	     if (frame[num].el_func)
	     {
		ccode = (frame[num].el_func)
			(frame[num].screen,
			 frame[num].el_values[frame[num].choice],
			 frame[num].el_strings[frame[num].choice],
			 frame[num].choice);
		if (ccode)
		   return ccode;
	     }
	     else
	     {
		if (frame[num].choice >=
		    (long)frame[num].el_count)
		   return (ULONG) -1;
		else
		   return
		      (frame[num].el_values[frame[num].choice]);
	     }
	     break;

#if (LINUX_UTIL)
	  case F3:
#else
	  case ESC:
#endif
	     if (frame[num].warn_func)
	     {
		register ULONG retCode;

		retCode = (frame[num].warn_func)
			(frame[num].screen, frame[num].choice);
		if (retCode)
		   return retCode;
		else
		   break;
	     }
	     else
		return -1;

	  case PG_UP:
	     for (i=0; i < frame[num].window_size - 1; i++)
	     {
		frame[num].choice--;
		frame[num].index--;

		if (frame[num].index < 0)
		{
		   frame[num].index = 0;
		   if (frame[num].choice >= 0)
		   {
		      if (frame[num].top)
			 frame[num].top--;
		      frame[num].bottom = frame[num].top +
                                               frame[num].window_size;
		   }
		}
		if (frame[num].choice < 0)
		   frame[num].choice = 0;
	     }

             for (i=0; i < frame[num].window_size; i++)
             {
               if (i < frame[num].el_count)
               {
	          if (frame[num].el_strings[frame[num].top + i])
	          {
	             if (frame[num].scroll_frame)
                     {
			put_char(frame[num].screen,
			    ' ',
			    row + i, col,
			    ((row + i == row + frame[num].index) &&
                             frame[num].focus)
			    ? bar_attribute : frame[num].fill_color |
			    frame[num].text_color);

			put_char(frame[num].screen,
			    frame[num].scroll_frame,
			    row + i, col + 1,
			    ((row + i == row + frame[num].index) &&
                             frame[num].focus)
			    ? bar_attribute : frame[num].fill_color |
			    frame[num].text_color);

			put_string_to_length(frame[num].screen,
			    (const char *)
			    frame[num].el_strings[frame[num].top + i],
			    frame[num].el_attr[frame[num].top + i],
			    row + i, col + 2,
			    ((row + i == row + frame[num].index) &&
                             frame[num].focus)
			    ? bar_attribute
			    : frame[num].fill_color |
			    frame[num].text_color,
			    width - 2);
                     }
	             else
		     {
			put_string_to_length(frame[num].screen,
			    (const char *)
			    frame[num].el_strings[frame[num].top + i],
			    frame[num].el_attr[frame[num].top + i],
			    row + i, col,
			    ((row + i == row + frame[num].index) &&
                             frame[num].focus)
			    ? bar_attribute
			    : frame[num].fill_color |
			    frame[num].text_color,
			    width);
                     }
	          }
               }
            }

            if (frame[num].el_count > frame[num].window_size &&
	       frame[num].top)
            {
	       put_char(frame[num].screen,
		  frame[num].up_char,
		  row,
		  col,
		  get_char_attribute(frame[num].screen, row, col));
            }
            else
            {
	      put_char(frame[num].screen,
		  ' ',
		  row,
		  col,
		  get_char_attribute(frame[num].screen, row, col));
            }

            if ((frame[num].el_count > frame[num].window_size) &&
               (frame[num].bottom < (long)(frame[num].el_count)))
            {
	       put_char(frame[num].screen,
		  frame[num].down_char,
		  row + frame[num].window_size - 1,
		  col,
		  get_char_attribute(frame[num].screen,
				     row + frame[num].window_size - 1,
				     col));
            }
            else
            {
	       put_char(frame[num].screen,
		  ' ',
		  row + frame[num].window_size - 1,
		  col,
		  get_char_attribute(frame[num].screen,
				     row + frame[num].window_size - 1,
				     col));
             }
	     break;

	  case PG_DOWN:
	     for (i=0; i < frame[num].window_size - 1; i++)
	     {
		frame[num].choice++;
		frame[num].index++;

		if (frame[num].index >= (long)frame[num].el_count)
		   frame[num].index--;

		if (frame[num].index >= (long)frame[num].window_size)
		{
		   frame[num].index--;
		   if (frame[num].choice <
		       (long)frame[num].el_count)
		   {
		      frame[num].top++;
		      frame[num].bottom = frame[num].top +
			      frame[num].window_size;
		   }
		}
		if (frame[num].choice >=
                    (long)frame[num].el_count)
		   frame[num].choice--;
	     }

             for (i=0; i < frame[num].window_size; i++)
             {
               if (i < frame[num].el_count)
               {
	          if (frame[num].el_strings[frame[num].top + i])
	          {
	             if (frame[num].scroll_frame)
                     {
			put_char(frame[num].screen,
			    ' ',
			    row + i, col,
			    ((row + i == row + frame[num].index) &&
                             frame[num].focus)
			    ? bar_attribute : frame[num].fill_color |
			    frame[num].text_color);

			put_char(frame[num].screen,
			    frame[num].scroll_frame,
			    row + i, col + 1,
			    ((row + i == row + frame[num].index) &&
                             frame[num].focus)
			    ? bar_attribute : frame[num].fill_color |
			    frame[num].text_color);

		        put_string_to_length(frame[num].screen,
			    (const char *)
			    frame[num].el_strings[frame[num].top + i],
			    frame[num].el_attr[frame[num].top + i],
			    row + i, col + 2,
			    ((row + i == row + frame[num].index) &&
                             frame[num].focus)
			    ? bar_attribute
			    : frame[num].fill_color |
			    frame[num].text_color,
			    width - 2);
                     }
	             else
		     {
		        put_string_to_length(frame[num].screen,
			    (const char *)
			    frame[num].el_strings[frame[num].top + i],
			    frame[num].el_attr[frame[num].top + i],
			    row + i, col,
			    ((row + i == row + frame[num].index) &&
                             frame[num].focus)
			    ? bar_attribute
			    : frame[num].fill_color |
			    frame[num].text_color,
			    width);
		     }
	          }
               }
            }

            if (frame[num].el_count > frame[num].window_size &&
	       frame[num].top)
            {
	       put_char(frame[num].screen,
		  frame[num].up_char,
		  row,
		  col,
		  get_char_attribute(frame[num].screen, row, col));
            }
            else
            {
	      put_char(frame[num].screen,
		  ' ',
		  row,
		  col,
		  get_char_attribute(frame[num].screen, row, col));
            }

            if ((frame[num].el_count > frame[num].window_size) &&
               (frame[num].bottom < (long)(frame[num].el_count)))
            {
	       put_char(frame[num].screen,
		  frame[num].down_char,
		  row + frame[num].window_size - 1,
		  col,
		  get_char_attribute(frame[num].screen,
				     row + frame[num].window_size - 1,
				     col));
            }
            else
            {
	       put_char(frame[num].screen,
		  ' ',
		  row + frame[num].window_size - 1,
		  col,
		  get_char_attribute(frame[num].screen,
				     row + frame[num].window_size - 1,
				     col));
             }
	     break;

	  case UP_ARROW:
	     frame[num].choice--;
	     frame[num].index--;

	     if (frame[num].index < 0)
	     {
		frame[num].index = 0;
		if (frame[num].choice >= 0)
		{
		   if (frame[num].top)
		      frame[num].top--;
		   frame[num].bottom = frame[num].top +
			   frame[num].window_size;
		   scroll_menu(num, 0);
		}
	     }

	     if (frame[num].choice < 0)
		frame[num].choice = 0;

	     break;

	  case DOWN_ARROW:
	     frame[num].choice++;
	     frame[num].index++;

	     if (frame[num].index >= (long)frame[num].el_count)
		frame[num].index--;

	     if (frame[num].index >= (long)frame[num].window_size)
	     {
		frame[num].index--;
		if (frame[num].choice < (long)frame[num].el_count)
		{
		   frame[num].top++;
		   frame[num].bottom = frame[num].top +
			   frame[num].window_size;
		   scroll_menu(num, 1);
		}
	     }

	     if (frame[num].choice >= (long)frame[num].el_count)
		frame[num].choice--;

	     break;

	  default:
	     if (frame[num].key_handler)
	     {
		register ULONG retCode;

		retCode = (frame[num].key_handler)
			(frame[num].screen, key, frame[num].choice);
		if (retCode)
		   return (retCode);
	     }
	     break;
       }

    }

}


ULONG fill_menu(ULONG num, ULONG ch, ULONG attr)
{
#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten, i;
    COORD coordScreen;

    for (i=frame[num].start_row; i < frame[num].end_row + 1; i++)
    {
       coordScreen.X = (short)frame[num].start_column;
       coordScreen.Y = (short)i;

       bSuccess = FillConsoleOutputCharacter( hConsole, (TCHAR) ch,
				   (frame[num].end_column + 1) -
				    frame[num].start_column,
				    coordScreen, &cCharsWritten );

       bSuccess = FillConsoleOutputAttribute( hConsole, (WORD)attr,
				   (frame[num].end_column + 1) -
				    frame[num].start_column,
				    coordScreen, &cCharsWritten );
    }
    return 0;

#endif

#if (DOS_UTIL | LINUX_UTIL)
   register ULONG i, j;

   for (i=frame[num].start_column; i < frame[num].end_column + 1; i++)
   {
      for (j=frame[num].start_row; j < frame[num].end_row + 1; j++)
      {
	 put_char(frame[num].screen, ch, j, i, attr);
      }
   }
   return 0;

#endif

}

ULONG save_menu(ULONG num)
{
#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten, i, len;
    COORD coordScreen;
    BYTE *buf_ptr;

    buf_ptr = (BYTE *) frame[num].p;
    for (i=frame[num].start_row; i < frame[num].end_row + 1; i++)
    {
       coordScreen.X = (short)frame[num].start_column;
       coordScreen.Y = (short)i;

       len = frame[num].end_column - frame[num].start_column + 1;
       bSuccess = ReadConsoleOutputCharacter(hConsole,
					    (LPSTR)buf_ptr,
					     len,
					     coordScreen,
					     &cCharsWritten);
       buf_ptr += len;
       bSuccess = ReadConsoleOutputAttribute(hConsole,
					    (LPWORD)buf_ptr,
					     len,
					     coordScreen,
					     &cCharsWritten);
       buf_ptr += len * 2;
    }
    frame[num].saved = 1;
    return 0;
#endif

#if (DOS_UTIL | LINUX_UTIL)
   register ULONG i, j;
   BYTE *buf_ptr;
   BYTE *v;
   BYTE *t;

#if LINUX_UTIL
   if (pthread_mutex_lock(&vidmem_mutex))
      return -1;
#endif
   buf_ptr = (BYTE *) frame[num].p;
   v = frame[num].screen->p_vidmem;
   for (i=frame[num].start_column; i < frame[num].end_column + 1; i++)
   {
      for (j=frame[num].start_row; j < frame[num].end_row + 1; j++)
      {
	 t = (v + (j * frame[num].screen->ncols * 2) + i * 2);
	 *buf_ptr++ = *t++;
	 *buf_ptr++ = *t;
	 *(t - 1) = ' ';  // fill window
      }
   }
   frame[num].saved = 1;
#if LINUX_UTIL
   pthread_mutex_unlock(&vidmem_mutex);
#endif
   return 0;

#endif

}

ULONG restore_screen(void)
{
#if (WINDOWS_NT_UTIL)
    return 0;
#endif


#if (DOS_UTIL | LINUX_UTIL)
    register ULONG i, j;
    BYTE *buf_ptr;
    NWSCREEN *screen = &console_screen;

    if (!screen)
       return -1;

    buf_ptr = (BYTE *) screen->p_vidmem;
    for (j=0; j < screen->nlines; j++)
    {
       for (i=0; i < screen->ncols; i++)
       {
	  put_char_direct(screen, *buf_ptr, j, i, *(buf_ptr + 1));
          buf_ptr += 2;
       }
    }
#if (LINUX_UTIL)
    redrawwin(stdscr);
    refresh_screen();
#endif
    return 0;

#endif

}

ULONG restore_menu(ULONG num)
{
#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten, i, len;
    COORD coordScreen;
    BYTE *buf_ptr;

    if (!frame[num].saved)
       return -1;

    buf_ptr = (BYTE *) frame[num].p;
    for (i=frame[num].start_row; i < frame[num].end_row + 1; i++)
    {
       coordScreen.X = (short)frame[num].start_column;
       coordScreen.Y = (short)i;

       len = frame[num].end_column - frame[num].start_column + 1;
       bSuccess = WriteConsoleOutputCharacter(hConsole,
					    (LPSTR)buf_ptr,
					     len,
					     coordScreen,
					     &cCharsWritten);

       buf_ptr += len;
       bSuccess = WriteConsoleOutputAttribute(hConsole,
					    (LPWORD)buf_ptr,
					     len,
					     coordScreen,
					     &cCharsWritten);
       buf_ptr += len * 2;
    }
    return 0;
#endif

#if (DOS_UTIL | LINUX_UTIL)
    register ULONG i, j;
    BYTE *buf_ptr;

    if (!frame[num].saved)
       return -1;

    buf_ptr = (BYTE *) frame[num].p;
    for (i=frame[num].start_column; i < frame[num].end_column + 1;
	 i++)
    {
       for (j=frame[num].start_row; j < frame[num].end_row + 1; j++)
       {
#if (LINUX_UTIL)
	  put_char(frame[num].screen, *buf_ptr, j, i, *(buf_ptr + 1));
#else
	  put_char(frame[num].screen, *buf_ptr, j, i, *(buf_ptr + 1));
#endif
          buf_ptr += 2;
       }
       frame[num].active = 0;
    }
#if (LINUX_UTIL)
    refresh_screen();
#endif
    return 0;

#endif

}

void free_elements(ULONG num)
{
   if (frame[num].el_attr_storage)
      free((void *) frame[num].el_attr_storage);
   frame[num].el_attr_storage = 0;

   if (frame[num].el_attr)
      free((void *) frame[num].el_attr);
   frame[num].el_attr = 0;

   if (frame[num].el_storage)
      free((void *) frame[num].el_storage);
   frame[num].el_storage = 0;

   if (frame[num].el_values)
      free((void *) frame[num].el_values);
   frame[num].el_values = 0;

   if (frame[num].el_strings)
      free((void *) frame[num].el_strings);
   frame[num].el_strings = 0;

   if (frame[num].p)
      free((void *) frame[num].p);
   frame[num].p = 0;

   return;
}

ULONG free_menu(ULONG num)
{
   register FIELD_LIST *fl;

#if LINUX_UTIL
   pthread_mutex_destroy(&frame[num].mutex);
#endif

   frame[num].cur_row = 0;
   frame[num].cur_column = 0;
   frame[num].choice = 0;
   frame[num].index = 0;

   restore_menu(num);
   free_elements(num);

   frame[num].el_count = 0;
   frame[num].el_func = 0;
   frame[num].warn_func = 0;
   frame[num].owner = 0;

   while (frame[num].head)
   {
      fl = frame[num].head;
      frame[num].head = fl->next;
      free(fl);
   }
   frame[num].head = frame[num].tail = 0;
   frame[num].field_count = 0;

   return 0;

}

ULONG display_menu_header(ULONG num)
{

   register ULONG col, len, i;

   if (!frame[num].header[0])
      return -1;

   col = frame[num].start_column;
   len = strlen((const char *) &frame[num].header[0]);
   len = (frame[num].end_column - col - len) / 2;
   if (len < 0)
      return -1;

   col = col + len;

      for (i=0; i < frame[num].end_column - frame[num].start_column;
           i++)
      {
         put_char(frame[num].screen,
		 frame[num].horizontal_frame,
		 frame[num].start_row + 2,
		 frame[num].start_column + i,
		 frame[num].border_color);
      }

      put_char(frame[num].screen,
	      frame[num].left_frame,
	      frame[num].start_row + 2,
	      frame[num].start_column,
	      frame[num].border_color);

      put_char(frame[num].screen,
	      frame[num].right_frame,
	      frame[num].start_row + 2,
	      frame[num].end_column,
	      frame[num].border_color);

   put_string(frame[num].screen,
	      (const char *)frame[num].header, NULL,
		frame[num].start_row + 1,
		col,
		frame[num].header_color);

   return 0;


}

ULONG draw_menu_border(ULONG num)
{
   register ULONG i;

   for (i=frame[num].start_row + 1; i < frame[num].end_row; i++)
   {
#if (WINDOWS_NT_UTIL | LINUX_UTIL | DOS_UTIL)
      put_char(frame[num].screen, frame[num].vertical_frame,
	       i,
	       frame[num].start_column,
	       frame[num].border_color);
      put_char(frame[num].screen, frame[num].vertical_frame,
	       i,
	       frame[num].end_column,
	       frame[num].border_color);
#endif
   }

   for (i=frame[num].start_column + 1; i < frame[num].end_column; i++)
   {
#if (WINDOWS_NT_UTIL | LINUX_UTIL | DOS_UTIL)
      put_char(frame[num].screen, frame[num].horizontal_frame,
	       frame[num].start_row,
	       i,
	       frame[num].border_color);
      put_char(frame[num].screen, frame[num].horizontal_frame,
	       frame[num].end_row,
	       i,
	       frame[num].border_color);
#endif
   }

   put_char(frame[num].screen, frame[num].upper_left,
	    frame[num].start_row,
	    frame[num].start_column,
	    frame[num].border_color);

   put_char(frame[num].screen, frame[num].lower_left,
	    frame[num].end_row,
	    frame[num].start_column,
	    frame[num].border_color);

   put_char(frame[num].screen, frame[num].upper_right,
	    frame[num].start_row,
	    frame[num].end_column,
	    frame[num].border_color);

   put_char(frame[num].screen, frame[num].lower_right,
	    frame[num].end_row,
	    frame[num].end_column,
	    frame[num].border_color);

   return 0;


}

void display_menu(ULONG num)
{
    register ULONG i, row, col, count, width;

    if (strlen((const char *)frame[num].header))
       row = frame[num].start_row + 3;
    else
       row = frame[num].start_row + 1;

    count = frame[num].window_size;
    col = frame[num].start_column + 1;
    width = frame[num].end_column - frame[num].start_column;
    if (width >= 1)
       width -= 1;

    for (i=0; i < count; i++)
    {
       if ((i < frame[num].el_count) &&
	   frame[num].el_strings &&
	   frame[num].el_strings[i])
       {
	  if (frame[num].scroll_frame)
	  {
	     put_char(frame[num].screen,
		    ' ',
		    row + i, col,
		    frame[num].fill_color |
		    frame[num].text_color);

	     put_char(frame[num].screen,
		    frame[num].scroll_frame,
		    row + i, col + 1,
		    frame[num].fill_color |
		    frame[num].text_color);

             put_string_to_length(frame[num].screen,
		    (const char *)frame[num].el_strings[i],
		    frame[num].el_attr[i],
		    row + i, col + 2,
		    frame[num].fill_color |
		    frame[num].text_color,
		    width - 2);
	  }
	  else
	  {
             put_string_to_length(frame[num].screen,
		    (const char *)frame[num].el_strings[i],
		    frame[num].el_attr[i],
		    row + i, col,
		    frame[num].fill_color | frame[num].text_color,
		    width);
	  }

       }
       else
       {
	  if (frame[num].scroll_frame)
	  {
	     put_char(frame[num].screen,
		     ' ',
		     row + i, col,
		     frame[num].fill_color |
		     frame[num].text_color);

	     put_char(frame[num].screen,
		     frame[num].scroll_frame,
		     row + i, col + 1,
		     frame[num].fill_color |
		     frame[num].text_color);
	  }
       }
    }
}

ULONG add_item_to_menu(ULONG num, const char *p, ULONG value)
{
   register ULONG i;
   register BYTE *v;

    if (frame[num].owner && frame[num].el_strings &&
        frame[num].el_count < frame[num].el_limit)
    {
       v = frame[num].el_strings[frame[num].el_count];
       if (!v)
	  return -1;

       for (i=0; *p && (i < frame[num].screen->ncols); i++)
	  *v++ = *p++;

       frame[num].el_strings[frame[num].el_count]
                                [frame[num].screen->ncols - 1] = '\0';
       frame[num].el_values[frame[num].el_count++] = value;

       return 0;
    }
    return -1;
}

ULONG activate_menu(ULONG num)
{

   register ULONG len;
   register ULONG i, retCode;

   if (!frame[num].screen)
      return -1;

   get_xy(frame[num].screen, (ULONG *)&frame[num].pcur_row,
	  (ULONG *)&frame[num].pcur_column);

   len = 0;
   for (i=0; i < frame[num].el_count; i++)
   {
      if (strlen((const char *)frame[num].el_strings[i]) > len)
	 len = strlen((const char *)frame[num].el_strings[i]);
   }

   if (frame[num].header)
   {
      if (strlen((const char *)frame[num].header) > len)
	 len = strlen((const char *)frame[num].header);
   }

   frame[num].end_column = len + 3 + frame[num].start_column;

   if (strlen((const char *)frame[num].header))
   {
      if (frame[num].window_size)
	 frame[num].end_row = frame[num].window_size + 3 +
				frame[num].start_row;
      else
	 frame[num].end_row = frame[num].el_count + 3 +
				frame[num].start_row;
   }
   else
   {
      if (frame[num].window_size)
	 frame[num].end_row = frame[num].window_size + 1 +
				frame[num].start_row;
      else
	 frame[num].end_row = frame[num].el_count + 1 +
				frame[num].start_row;

   }

   if (frame[num].end_row + 1 > frame[num].screen->nlines - 1 ||
      frame[num].end_column + 1 > frame[num].screen->ncols - 1)
   {
      return -1;
   }

   if (!frame[num].active)
   {
      frame[num].active = TRUE;
      save_menu(num);
      fill_menu(num, ' ', frame[num].fill_color);
   }

   if (frame[num].border)
   {
      draw_menu_border(num);
      display_menu_header(num);
   }

   display_menu(num);

   retCode = get_resp(num);

   restore_menu(num);

   return retCode;

}

ULONG make_menu(NWSCREEN *screen,
	       const char *header,
	       ULONG start_row,
	       ULONG start_column,
	       ULONG window_size,
	       ULONG border,
	       ULONG hcolor,
	       ULONG bcolor,
	       ULONG fcolor,
	       ULONG tcolor,
	       ULONG (*el_func)(NWSCREEN *, ULONG, BYTE *, ULONG),
	       ULONG (*warn_func)(NWSCREEN *, ULONG),
	       ULONG (*key_handler)(NWSCREEN *, ULONG, ULONG),
	       ULONG scroll_barPresent,
	       ULONG max_lines)
{

   register ULONG i;
   register ULONG num;

   if ((max_lines == (ULONG) -1) || (!max_lines))
      max_lines = 100;

   for (num=1; num < MAX_MENU; num++)
   {
      if (!frame[num].owner)
	 break;
   }

   if (!num || num > MAX_MENU)
      return 0;

   if (start_row > screen->nlines - 1 || start_row < 0 ||
       start_column > screen->ncols - 2 || start_column < 0)
      return 0;

   frame[num].p = (BYTE *)malloc(screen->nlines * screen->ncols * 2);
   if (!frame[num].p)
   {
      free_elements(num);
      return 0;
   }
   set_data((ULONG *)frame[num].p, 0, screen->nlines * screen->ncols * 2);

   frame[num].el_storage =
                     (BYTE *)malloc(max_lines * screen->ncols);
   if (!frame[num].el_storage)
   {
      free_elements(num);
      return 0;
   }
   set_data_b((BYTE *) frame[num].el_storage, 0,
	      max_lines * screen->ncols);

   frame[num].el_strings =
                  (BYTE **)malloc(max_lines * sizeof(BYTE *));
   if (!frame[num].el_strings)
   {
      free_elements(num);
      return 0;
   }
   set_data((ULONG *) frame[num].el_strings, 0,
                max_lines * sizeof(BYTE *));

   frame[num].el_values =
              (ULONG *)malloc(max_lines * sizeof(ULONG));
   if (!frame[num].el_values)
   {
      free_elements(num);
      return 0;
   }
   set_data((ULONG *) frame[num].el_values, 0,
                        max_lines * sizeof(ULONG));

   frame[num].el_attr_storage =
                     (BYTE *)malloc(max_lines * screen->ncols);
   if (!frame[num].el_attr_storage)
   {
      free_elements(num);
      return 0;
   }
   set_data_b((BYTE *) frame[num].el_attr_storage, 0,
                       max_lines * screen->ncols);

   frame[num].el_attr =
                  (BYTE **)malloc(max_lines * sizeof(BYTE *));
   if (!frame[num].el_attr)
   {
      free_elements(num);
      return 0;
   }
   set_data((ULONG *) frame[num].el_attr, 0,
	    max_lines * sizeof(BYTE *));

   for (i=0; i < max_lines; i++)
   {
      register BYTE *p = &frame[num].el_storage[i * screen->ncols];
      add_item_to_portal(num, frame[num].el_strings, p, i);
      p[screen->ncols - 1] = '\0';
   }

   for (i=0; i < max_lines; i++)
   {
      register BYTE *p = &frame[num].el_attr_storage[i * screen->ncols];
      add_item_to_portal(num, frame[num].el_attr, p, i);
      p[screen->ncols - 1] = '\0';
   }

   for (i=0; i < (HEADER_LEN - 1); i++)
   {
      if (!header || !header[i])
	 break;
      frame[num].header[i] = header[i];
   }
   frame[num].header[i] = 0x00;   // null terminate string

   frame[num].start_row = start_row;
   frame[num].end_row = start_row + 1;
   frame[num].start_column = start_column;
   frame[num].end_column = start_column + 1;
   frame[num].screen = screen;
   frame[num].border = border;
   frame[num].num = num;
   frame[num].active = 0;
   frame[num].cur_row = 0;
   frame[num].cur_column = 0;
   frame[num].header_color = BRITEWHITE;
   frame[num].border_color = BRITEWHITE;
   frame[num].fill_color = BRITEWHITE;
   frame[num].text_color = BRITEWHITE;
   frame[num].window_size = window_size;
   frame[num].el_func = el_func;
   frame[num].el_count = 0;
   frame[num].warn_func = warn_func;
   frame[num].key_handler = key_handler;
   frame[num].el_limit = max_lines;
   frame[num].head = frame[num].tail = 0;
   frame[num].field_count = 0;
   frame[num].choice = 0;
   frame[num].index = 0;
   frame[num].top = 0;
   frame[num].bottom = 0;
   frame[num].focus = 0;
   frame[num].sleep_count = 0;
   frame[num].enable_focus = 0;
   frame[num].mask = 0;
   frame[num].saved = 0;

#if LINUX_UTIL
   // Unicode UTF8 Box Characters
   if (border == BORDER_SINGLE)
   {
      frame[num].w_upper_left       = "\u250c";
      frame[num].w_upper_right      = "\u2510";
      frame[num].w_lower_left       = "\u2514";
      frame[num].w_lower_right      = "\u2518";
      frame[num].w_left_frame       = "\u251c";
      frame[num].w_right_frame      = "\u2524";
      frame[num].w_vertical_frame   = "\u2500";
      frame[num].w_horizontal_frame = "\u2502";
      frame[num].w_up_char          = "\u25b3";
      frame[num].w_down_char        = "\u25bd";
   }
   else
   if (border == BORDER_DOUBLE)
   {
      frame[num].w_upper_left       = "\u2554";
      frame[num].w_upper_right      = "\u2557";
      frame[num].w_lower_left       = "\u255a";
      frame[num].w_lower_right      = "\u255d";
      frame[num].w_left_frame       = "\u2560";
      frame[num].w_right_frame      = "\u2563";
      frame[num].w_vertical_frame   = "\u2550";
      frame[num].w_horizontal_frame = "\u2551";
      frame[num].w_up_char          = "\u25b3";
      frame[num].w_down_char        = "\u25bd";
   }
   else
   {
      frame[num].w_upper_left       = 0;
      frame[num].w_upper_right      = 0;
      frame[num].w_lower_left       = 0;
      frame[num].w_lower_right      = 0;
      frame[num].w_left_frame       = 0;
      frame[num].w_right_frame      = 0;
      frame[num].w_vertical_frame   = 0;
      frame[num].w_horizontal_frame = 0;
      frame[num].w_up_char          = 0;
      frame[num].w_down_char        = 0;
   }
   if (scroll_barPresent)
      frame[num].w_scroll_frame     = "\u2502";
   else
      frame[num].w_scroll_frame     = 0;
#endif

#if (DOS_UTIL | WINDOWS_NT_UTIL)
   if (border == BORDER_SINGLE)
   {
      frame[num].upper_left       = 218;
      frame[num].upper_right      = 191;
      frame[num].lower_left       = 192;
      frame[num].lower_right      = 217;
      frame[num].left_frame       = 195;
      frame[num].right_frame      = 180;
      frame[num].vertical_frame   = 179;
      frame[num].horizontal_frame = 196;
      frame[num].up_char          = 0x1E;
      frame[num].down_char        = 0x1F;
   }
   else
   if (border == BORDER_DOUBLE)
   {
      frame[num].upper_left       = 201;
      frame[num].upper_right      = 187;
      frame[num].lower_left       = 200;
      frame[num].lower_right      = 188;
      frame[num].left_frame       = 204;
      frame[num].right_frame      = 185;
      frame[num].vertical_frame   = 186;
      frame[num].horizontal_frame = 205;
      frame[num].up_char          = 0x1E;
      frame[num].down_char        = 0x1F;
   }
   else
   {
      frame[num].upper_left       = 0;
      frame[num].upper_right      = 0;
      frame[num].lower_left       = 0;
      frame[num].lower_right      = 0;
      frame[num].left_frame       = 0;
      frame[num].right_frame      = 0;
      frame[num].vertical_frame   = 0;
      frame[num].horizontal_frame = 0;
      frame[num].up_char          = 0;
      frame[num].down_char        = 0;
   }
   if (scroll_barPresent)
      frame[num].scroll_frame     = 179;
   else
      frame[num].scroll_frame     = 0;

#endif

#if (LINUX_UTIL)
   if (text_mode)
   {
      if (border == BORDER_SINGLE)
      {
	 frame[num].upper_left       = '+';
	 frame[num].upper_right      = '+';
	 frame[num].lower_left       = '+';
	 frame[num].lower_right      = '+';
	 frame[num].left_frame       = '+';
	 frame[num].right_frame      = '+';
	 frame[num].vertical_frame   = '|';
	 frame[num].horizontal_frame = '-';
	 frame[num].up_char          = '*';
	 frame[num].down_char        = '*';
      }
      else
      if (border == BORDER_DOUBLE)
      {
	 frame[num].upper_left       = '*';
	 frame[num].upper_right      = '*';
	 frame[num].lower_left       = '*';
	 frame[num].lower_right      = '*';
	 frame[num].left_frame       = '*';
	 frame[num].right_frame      = '*';
	 frame[num].vertical_frame   = '|';
	 frame[num].horizontal_frame = '=';
	 frame[num].up_char          = '*';
	 frame[num].down_char        = '*';
      }
      else
      {
	 frame[num].upper_left       = 0;
	 frame[num].upper_right      = 0;
	 frame[num].lower_left       = 0;
	 frame[num].lower_right      = 0;
	 frame[num].left_frame       = 0;
	 frame[num].right_frame      = 0;
	 frame[num].vertical_frame   = 0;
	 frame[num].horizontal_frame = 0;
	 frame[num].up_char          = 0;
	 frame[num].down_char        = 0;
      }
      if (scroll_barPresent)
	 frame[num].scroll_frame     = '|';
      else
	 frame[num].scroll_frame     = 0;
   }
   else
   {
      if (border == BORDER_SINGLE)
      {
	 frame[num].upper_left       = 218 | A_ALTCHARSET;
	 frame[num].upper_right      = 191 | A_ALTCHARSET;
	 frame[num].lower_left       = 192 | A_ALTCHARSET;
	 frame[num].lower_right      = 217 | A_ALTCHARSET;
	 frame[num].left_frame       = 195 | A_ALTCHARSET;
	 frame[num].right_frame      = 180 | A_ALTCHARSET;
	 frame[num].vertical_frame   = 179 | A_ALTCHARSET;
	 frame[num].horizontal_frame = 196 | A_ALTCHARSET;
#if 0
	 frame[num].up_char          = 0x1E | A_ALTCHARSET;
	 frame[num].down_char        = 0x1F | A_ALTCHARSET;
#endif
	 frame[num].up_char          = '*';
	 frame[num].down_char        = '*';
      }
      else
      if (border == BORDER_DOUBLE)
      {
	 frame[num].upper_left       = 201 | A_ALTCHARSET;
	 frame[num].upper_right      = 187 | A_ALTCHARSET;
	 frame[num].lower_left       = 200 | A_ALTCHARSET;
	 frame[num].lower_right      = 188 | A_ALTCHARSET;
	 frame[num].left_frame       = 204 | A_ALTCHARSET;
	 frame[num].right_frame      = 185 | A_ALTCHARSET;
	 frame[num].vertical_frame   = 186 | A_ALTCHARSET;
	 frame[num].horizontal_frame = 205 | A_ALTCHARSET;
#if 0
	 frame[num].up_char          = 0x1E | A_ALTCHARSET;
         frame[num].down_char        = 0x1F | A_ALTCHARSET;
#endif
	 frame[num].up_char          = '*';
	 frame[num].down_char        = '*';
      }
      else
      {
	 frame[num].upper_left       = 0;
	 frame[num].upper_right      = 0;
	 frame[num].lower_left       = 0;
	 frame[num].lower_right      = 0;
	 frame[num].left_frame       = 0;
	 frame[num].right_frame      = 0;
	 frame[num].vertical_frame   = 0;
	 frame[num].horizontal_frame = 0;
	 frame[num].up_char          = 0;
	 frame[num].down_char        = 0;
      }
      if (scroll_barPresent)
	frame[num].scroll_frame     = 179 | A_ALTCHARSET;
      else
	 frame[num].scroll_frame     = 0;
   }
#endif

   if (hcolor)
      frame[num].header_color = hcolor;
   if (bcolor)
      frame[num].border_color = bcolor;
   if (fcolor)
      frame[num].fill_color = fcolor;
   if (tcolor)
      frame[num].text_color = tcolor;

   frame[num].owner = 1;

#if LINUX_UTIL
   pthread_mutex_init(&frame[num].mutex, NULL);
#endif

   return num;

}

ULONG menu_write_string(ULONG num, BYTE *p, ULONG row, ULONG col, ULONG attr)
{

   if (!frame[num].active)
      return -1;

   put_string(frame[num].screen,
	        (const char *)p, NULL,
		frame[num].start_row + 1 + row,
		frame[num].start_column + 1 + col,
		attr);

   return 0;

}

void scroll_portal(ULONG num, ULONG up)
{

    register ULONG row, col;

    if (strlen((const char *)frame[num].header) ||
        strlen((const char *)frame[num].subheader))
    {
       row = frame[num].start_row + 3;
       if (strlen((const char *)frame[num].subheader))
	  row++;
    }
    else
       row = frame[num].start_row + 1;

    if (frame[num].scroll_frame)
       col = frame[num].start_column + 3;
    else
       col = frame[num].start_column + 1;

    scroll_display(frame[num].screen,
		  row,
		  col,
		  frame[num].end_row - row,
		  frame[num].end_column - col,
		  up);
    return;

}

void enable_portal_focus(ULONG num, ULONG interval)
{
    frame[num].enable_focus = 1;
    frame[num].focus_interval = interval;
    return;
}

void disable_portal_focus(ULONG num)
{
    frame[num].enable_focus = 0;
    frame[num].focus_interval = 0;
    return;
}

void set_portal_focus(ULONG num)
{
    if (frame[num].enable_focus)
    {
       frame[num].focus = 1;
       frame[num].sleep_count = 0;
    }
    return;
}

void clear_portal_focus(ULONG num)
{
    if (frame[num].enable_focus)
    {
       frame[num].focus = 0;
       frame[num].sleep_count = 0;
    }
    return;
}

int get_sleep_count(ULONG num)
{
    if (frame[num].enable_focus)
    {
       if (frame[num].focus_interval == -1)
          return 1;

       frame[num].sleep_count++;
       if (frame[num].sleep_count > frame[num].focus_interval)
       {
          frame[num].sleep_count = 0;
          return 0;
       }
       return 1;
    }
    return -1;
}

int get_portal_focus(ULONG num)
{
    if (frame[num].enable_focus)
    {
      return frame[num].focus;
    }
    return -1;
}

ULONG get_portal_resp(ULONG num)
{
    register ULONG key, row, col, width;
    register ULONG i;
    register ULONG retCode;

#if LINUX_UTIL
    if (pthread_mutex_lock(&frame[num].mutex))
       return -1;
#endif

    set_portal_focus(num);
    if (strlen((const char *)frame[num].header) ||
        strlen((const char *)frame[num].subheader))
    {
       row = frame[num].start_row + 3;
       if (strlen((const char *)frame[num].subheader))
	  row++;
    }
    else
       row = frame[num].start_row + 1;

    col = frame[num].start_column + 1;
    width = frame[num].end_column - frame[num].start_column;
    if (width >= 1)
       width -= 1;

    if (!frame[num].choice)
    {
       frame[num].index = 0;
       frame[num].top = 0;
       frame[num].bottom = frame[num].top +
	       frame[num].window_size;
    }
    frame[num].selected = 1;

    for (;;)
    {
       if (frame[num].el_strings[frame[num].choice])
       {
	  if (frame[num].scroll_frame)
	  {
	     put_char(frame[num].screen,
		    ' ',
		    row + frame[num].index, col,
		    bar_attribute);

	     put_char(frame[num].screen,
		    frame[num].scroll_frame,
		    row + frame[num].index, col + 1,
		    bar_attribute);

	     put_string_to_length(frame[num].screen,
		    (const char *)
	            frame[num].el_strings[frame[num].choice],
	            frame[num].el_attr[frame[num].choice],
		    row + frame[num].index, col + 2,
		    bar_attribute, width - 2);
	  }
	  else
	  {
	     put_string_to_length(frame[num].screen,
		    (const char *)
	            frame[num].el_strings[frame[num].choice],
	            frame[num].el_attr[frame[num].choice],
		    row + frame[num].index, col,
		    bar_attribute, width);
	  }
       }

       if (frame[num].el_limit > frame[num].window_size &&
           frame[num].top)
       {
	  put_char(frame[num].screen,
		  frame[num].up_char,
		  row,
		  col,
		  get_char_attribute(frame[num].screen, row, col));
       }
       else
       {
	  put_char(frame[num].screen,
		  ' ',
		  row,
		  col,
		  get_char_attribute(frame[num].screen, row, col));
       }

       if ((frame[num].el_limit > frame[num].window_size) &&
	   (frame[num].bottom < (long)(frame[num].el_limit)))
       {
	  put_char(frame[num].screen,
		  frame[num].down_char,
		  row + frame[num].window_size - 1,
		  col,
		  get_char_attribute(frame[num].screen,
				     row + frame[num].window_size - 1,
				     col));
       }
       else
       {
	  put_char(frame[num].screen,
		  ' ',
		  row + frame[num].window_size - 1,
		  col,
		  get_char_attribute(frame[num].screen,
				     row + frame[num].window_size - 1,
				     col));
       }

#if LINUX_UTIL
       pthread_mutex_unlock(&frame[num].mutex);
#endif
       key = get_key();

       set_portal_focus(num);

#if LINUX_UTIL
       if (pthread_mutex_lock(&frame[num].mutex))
       {
          clear_portal_focus(num);
          return -1;
       }
#endif
       if (frame[num].key_mask)
	  continue;

       if (frame[num].el_strings[frame[num].choice])
       {
	  if (frame[num].scroll_frame)
	  {
	     put_char(frame[num].screen,
		    ' ',
		    row + frame[num].index, col,
		    frame[num].fill_color |
		    frame[num].text_color);

	     put_char(frame[num].screen,
		    frame[num].scroll_frame,
		    row + frame[num].index, col + 1,
		    frame[num].fill_color |
		    frame[num].text_color);

	     put_string_to_length(frame[num].screen,
		    (const char *)
		    frame[num].el_strings[frame[num].choice],
	            frame[num].el_attr[frame[num].choice],
		    row + frame[num].index, col + 2,
		    frame[num].fill_color |
		    frame[num].text_color, width - 2);
	  }
	  else
	  {
	     put_string_to_length(frame[num].screen,
		    (const char *)
		    frame[num].el_strings[frame[num].choice],
	            frame[num].el_attr[frame[num].choice],
		    row + frame[num].index, col,
		    frame[num].fill_color |
		    frame[num].text_color, width);
	  }
       }

       if (frame[num].el_limit > frame[num].window_size &&
           frame[num].top)
       {
	  put_char(frame[num].screen,
		  frame[num].up_char,
		  row,
		  col,
		  get_char_attribute(frame[num].screen, row, col));
       }
       else
       {
	  put_char(frame[num].screen,
		  ' ',
		  row,
		  col,
		  get_char_attribute(frame[num].screen, row, col));
       }

       if ((frame[num].el_limit > frame[num].window_size) &&
	   (frame[num].bottom < (long)(frame[num].el_limit)))
       {
	  put_char(frame[num].screen,
		  frame[num].down_char,
		  row + frame[num].window_size - 1,
		  col,
		  get_char_attribute(frame[num].screen,
				     row + frame[num].window_size - 1,
				     col));
       }
       else
       {
	  put_char(frame[num].screen,
		  ' ',
		  row + frame[num].window_size - 1,
		  col,
		  get_char_attribute(frame[num].screen,
				     row + frame[num].window_size - 1,
				     col));
       }

       switch (key)
       {
          // screensaver return key
          case 0:
             break;

          // repaint screen
          case ' ':
             restore_screen();
             break;

#if (LINUX_UTIL)
	  case F3:
#else
	  case ESC:
#endif
	     if (frame[num].warn_func)
	     {

		retCode = (frame[num].warn_func)(frame[num].screen,
					 frame[num].choice);
		if (retCode)
		{
		   frame[num].selected = 0;
                   goto UnlockMutex;
		}
		else
		   break;
	     }
	     else
	     {
		frame[num].selected = 0;
		retCode =  -1;
                goto UnlockMutex;
	     };

	  case PG_UP:
	     for (i=0; i < frame[num].window_size - 1; i++)
	     {
		frame[num].choice--;
		frame[num].index--;

		if (frame[num].index < 0)
		{
		   frame[num].index = 0;
		   if (frame[num].choice >= 0)
		   {
		      if (frame[num].top)
			 frame[num].top--;
		      frame[num].bottom = frame[num].top +
                                               frame[num].window_size;
		   }
		}
		if (frame[num].choice < 0)
		   frame[num].choice = 0;
	     }

             for (i=0; i < frame[num].window_size; i++)
             {
               if (i < frame[num].el_limit)
               {
	          if (frame[num].el_strings[frame[num].top + i])
	          {
	             if (frame[num].scroll_frame)
                     {
			put_char(frame[num].screen,
			    ' ',
			    row + i, col,
			    ((row + i == row + frame[num].index) &&
                             frame[num].focus)
			    ? bar_attribute
			    : frame[num].fill_color |
			    frame[num].text_color);

			put_char(frame[num].screen,
			    frame[num].scroll_frame,
			    row + i, col + 1,
			    ((row + i == row + frame[num].index) &&
                             frame[num].focus)
			    ? bar_attribute
			    : frame[num].fill_color |
			    frame[num].text_color);

			put_string_to_length(frame[num].screen,
			    (const char *)
			    frame[num].el_strings[frame[num].top + i],
			    frame[num].el_attr[frame[num].top + i],
			    row + i, col + 2,
			    ((row + i == row + frame[num].index) &&
                             frame[num].focus)
			    ? bar_attribute
			    : frame[num].fill_color |
			    frame[num].text_color,
			    width - 2);
                     }
	             else
		     {
			put_string_to_length(frame[num].screen,
			    (const char *)
			    frame[num].el_strings[frame[num].top + i],
			    frame[num].el_attr[frame[num].top + i],
			    row + i,
			    col,
			    ((row + i == row + frame[num].index) &&
                             frame[num].focus)
			    ? bar_attribute
			    : frame[num].fill_color |
			    frame[num].text_color,
			    width);
	             }
	          }
               }
            }

            if (frame[num].el_limit > frame[num].window_size &&
	       frame[num].top)
            {
	       put_char(frame[num].screen,
		  frame[num].up_char,
		  row,
		  col,
		  get_char_attribute(frame[num].screen, row, col));
            }
            else
            {
	      put_char(frame[num].screen,
		  ' ',
		  row,
		  col,
		  get_char_attribute(frame[num].screen, row, col));
            }

            if ((frame[num].el_limit > frame[num].window_size) &&
               (frame[num].bottom < (long)(frame[num].el_limit)))
            {
	       put_char(frame[num].screen,
		  frame[num].down_char,
		  row + frame[num].window_size - 1,
		  col,
		  get_char_attribute(frame[num].screen,
				     row + frame[num].window_size - 1,
				     col));
            }
            else
            {
	       put_char(frame[num].screen,
		  ' ',
		  row + frame[num].window_size - 1,
		  col,
		  get_char_attribute(frame[num].screen,
				     row + frame[num].window_size - 1,
				     col));
             }

             frame[num].selected = 1;
             break;

	  case PG_DOWN:
	     for (i=0; i < frame[num].window_size - 1; i++)
	     {
		if (frame[num].choice >=
                    (long)frame[num].el_limit)
		   break;

		frame[num].choice++;
		frame[num].index++;

		if (frame[num].index >= (long)frame[num].el_limit)
		   frame[num].index--;

		if (frame[num].index >= (long)frame[num].window_size)
		{
		   frame[num].index--;
		   if (frame[num].choice <
                      (long)frame[num].el_limit)
		   {
		      frame[num].top++;
		      frame[num].bottom = frame[num].top +
                                               frame[num].window_size;
		   }
		}
		if (frame[num].choice >=
                   (long)frame[num].el_limit)
		   frame[num].choice--;
	     }

             for (i=0; i < frame[num].window_size; i++)
             {
               if (i < frame[num].el_limit)
               {
	          if (frame[num].el_strings[frame[num].top + i])
	          {
	             if (frame[num].scroll_frame)
                     {
			put_char(frame[num].screen,
			    ' ',
			    row + i, col,
			    ((row + i == row + frame[num].index) &&
                            frame[num].focus)
			    ? bar_attribute
			    : frame[num].fill_color |
			    frame[num].text_color);

			put_char(frame[num].screen,
			    frame[num].scroll_frame,
			    row + i, col + 1,
			    ((row + i == row + frame[num].index) &&
                            frame[num].focus)
			    ? bar_attribute
			    : frame[num].fill_color |
			    frame[num].text_color);

			put_string_to_length(frame[num].screen,
			    (const char *)
			    frame[num].el_strings[frame[num].top + i],
			    frame[num].el_attr[frame[num].top + i],
			    row + i, col + 2,
			    ((row + i == row + frame[num].index) &&
                            frame[num].focus)
			    ? bar_attribute
			    : frame[num].fill_color |
			    frame[num].text_color, width - 2);
                     }
	             else
		     {
			put_string_to_length(frame[num].screen,
			    (const char *)
			    frame[num].el_strings[frame[num].top + i],
			    frame[num].el_attr[frame[num].top + i],
			    row + i, col,
			    ((row + i == row + frame[num].index) &&
                            frame[num].focus)
			    ? bar_attribute
			    : frame[num].fill_color |
			    frame[num].text_color, width);
		     }
	          }
               }
            }

            if (frame[num].el_limit > frame[num].window_size &&
	       frame[num].top)
            {
	       put_char(frame[num].screen,
		  frame[num].up_char,
		  row,
		  col,
		  get_char_attribute(frame[num].screen, row, col));
            }
            else
            {
	      put_char(frame[num].screen,
		  ' ',
		  row,
		  col,
		  get_char_attribute(frame[num].screen, row, col));
            }

            if ((frame[num].el_limit > frame[num].window_size) &&
               (frame[num].bottom < (long)(frame[num].el_limit)))
            {
	       put_char(frame[num].screen,
		  frame[num].down_char,
		  row + frame[num].window_size - 1,
		  col,
		  get_char_attribute(frame[num].screen,
				     row + frame[num].window_size - 1,
				     col));
            }
            else
            {
	       put_char(frame[num].screen,
		  ' ',
		  row + frame[num].window_size - 1,
		  col,
		  get_char_attribute(frame[num].screen,
				     row + frame[num].window_size - 1,
				     col));
             }

             frame[num].selected = 1;
             break;
#if LINUX_UTIL
	  case VT220_HOME:
#endif
	  case HOME:
             frame[num].choice = 0;
             frame[num].index = 0;
             frame[num].top = 0;
             frame[num].bottom = frame[num].top +
                                      frame[num].window_size;
	     for (i=0; i < frame[num].window_size; i++)
	     {
		if (frame[num].choice >=
		    (long)frame[num].el_limit)
		   break;

		if (frame[num].el_strings[frame[num].choice])
		{
		   if (frame[num].scroll_frame)
		   {
		      put_char(frame[num].screen,
			     ' ',
			     row + frame[num].index, col,
			     frame[num].fill_color |
			     frame[num].text_color);

		      put_char(frame[num].screen,
			     frame[num].scroll_frame,
			     row + frame[num].index, col + 1,
			     frame[num].fill_color |
			     frame[num].text_color);

		      put_string_to_length(frame[num].screen,
			     (const char *)
		             frame[num].el_strings[frame[num].choice],
		             frame[num].el_attr[frame[num].choice],
			     row + frame[num].index, col + 2,
			     frame[num].fill_color |
			     frame[num].text_color, width - 2);
		   }
		   else
		   {
		      put_string_to_length(frame[num].screen,
			     (const char *)
		             frame[num].el_strings[frame[num].choice],
		             frame[num].el_attr[frame[num].choice],
			     row + frame[num].index, col,
			     frame[num].fill_color |
			     frame[num].text_color, width);
		   }
		}

		frame[num].choice++;
		frame[num].index++;

		if (frame[num].index >= (long)frame[num].el_limit)
		   frame[num].index--;

		if (frame[num].index >= (long)frame[num].window_size)
		{
		   frame[num].index--;
		}

		if (frame[num].choice >=
		    (long)frame[num].el_limit)
		   frame[num].choice--;
	     }
             frame[num].choice = 0;
             frame[num].index = 0;
             frame[num].top = 0;
             frame[num].bottom = frame[num].top +
                                      frame[num].window_size;
             frame[num].selected = 1;
             break;

#if LINUX_UTIL
	  case VT220_END:
#endif
	  case END:
	     if (frame[num].el_limit)
             {
	        frame[num].choice = frame[num].el_limit - 1;
	        frame[num].index = frame[num].window_size - 1;
	        frame[num].top = frame[num].el_limit -
	                              frame[num].window_size;
                frame[num].bottom = frame[num].top +
                                      frame[num].window_size;
             }
             else
             {
                frame[num].choice = 0;
                frame[num].index = 0;
                frame[num].top = 0;
                frame[num].bottom = frame[num].top +
                                         frame[num].window_size;
             }

	     for (i=0; i < frame[num].window_size; i++)
	     {
		if (frame[num].choice >=
		    (long)frame[num].el_limit)
		   break;

		if (frame[num].el_strings[frame[num].choice])
		{
		   if (frame[num].scroll_frame)
		   {
		      put_char(frame[num].screen,
			     ' ',
			     row + frame[num].index, col,
			     frame[num].fill_color |
			     frame[num].text_color);

		      put_char(frame[num].screen,
			     frame[num].scroll_frame,
			     row + frame[num].index, col + 1,
			     frame[num].fill_color |
			     frame[num].text_color);

		      put_string_to_length(frame[num].screen,
			     (const char *)
		             frame[num].el_strings[frame[num].choice],
	                     frame[num].el_attr[frame[num].choice],
			     row + frame[num].index, col + 2,
			     frame[num].fill_color |
			     frame[num].text_color, width - 2);
		   }
		   else
		   {
		      put_string_to_length(frame[num].screen,
			     (const char *)
		             frame[num].el_strings[frame[num].choice],
		             frame[num].el_attr[frame[num].choice],
			     row + frame[num].index, col,
			     frame[num].fill_color |
			     frame[num].text_color, width);
		   }
		}

		frame[num].choice++;
		frame[num].index++;

		if (frame[num].index >= (long)frame[num].el_limit)
		   frame[num].index--;

		if (frame[num].index >= (long)frame[num].window_size)
		{
		   frame[num].index--;
		}

		if (frame[num].choice >=
		    (long)frame[num].el_limit)
		   frame[num].choice--;
	     }


             for (i=0; i < frame[num].window_size; i++)
             {
               if (i < frame[num].el_limit)
               {
	          if (frame[num].el_strings[frame[num].top + i])
	          {
	             if (frame[num].scroll_frame)
                     {
			put_char(frame[num].screen,
			    ' ',
			    row + i, col,
			    ((row + i == row + frame[num].index) &&
                            frame[num].focus)
			    ? bar_attribute
			    : frame[num].fill_color |
			    frame[num].text_color);

			put_char(frame[num].screen,
			    frame[num].scroll_frame,
			    row + i, col + 1,
			    ((row + i == row + frame[num].index) &&
                            frame[num].focus)
			    ? bar_attribute
			    : frame[num].fill_color |
			    frame[num].text_color);

			put_string_to_length(frame[num].screen,
			    (const char *)
			    frame[num].el_strings[frame[num].top + i],
			    frame[num].el_attr[frame[num].top + i],
			    row + i, col + 2,
			    ((row + i == row + frame[num].index) &&
                            frame[num].focus)
			    ? bar_attribute
			    : frame[num].fill_color |
			    frame[num].text_color, width - 2);
                     }
	             else
		     {
			put_string_to_length(frame[num].screen,
			    (const char *)
			    frame[num].el_strings[frame[num].top + i],
			    frame[num].el_attr[frame[num].top + i],
			    row + i, col,
			    ((row + i == row + frame[num].index) &&
                            frame[num].focus)
			    ? bar_attribute
			    : frame[num].fill_color |
			    frame[num].text_color, width);
		     }
	          }
               }
            }

            if (frame[num].el_limit > frame[num].window_size &&
	       frame[num].top)
            {
	       put_char(frame[num].screen,
		  frame[num].up_char,
		  row,
		  col,
		  get_char_attribute(frame[num].screen, row, col));
            }
            else
            {
	      put_char(frame[num].screen,
		  ' ',
		  row,
		  col,
		  get_char_attribute(frame[num].screen, row, col));
            }

            if ((frame[num].el_limit > frame[num].window_size) &&
               (frame[num].bottom < (long)(frame[num].el_limit)))
            {
	       put_char(frame[num].screen,
		  frame[num].down_char,
		  row + frame[num].window_size - 1,
		  col,
		  get_char_attribute(frame[num].screen,
				     row + frame[num].window_size - 1,
				     col));
            }
            else
            {
	       put_char(frame[num].screen,
		  ' ',
		  row + frame[num].window_size - 1,
		  col,
		  get_char_attribute(frame[num].screen,
				     row + frame[num].window_size - 1,
				     col));
             }

             frame[num].selected = 1;
             break;

	  case UP_ARROW:
	     frame[num].choice--;
	     frame[num].index--;

	     if (frame[num].index < 0)
	     {
		frame[num].index = 0;
		if (frame[num].choice >= 0)
		{
		   if (frame[num].top)
		      frame[num].top--;
		   frame[num].bottom = frame[num].top +
			   frame[num].window_size;
		   scroll_portal(num, 0);
		}
	     }

	     if (frame[num].choice < 0)
		frame[num].choice = 0;

	     break;

	  case DOWN_ARROW:
	     if (frame[num].choice >= (long)frame[num].el_limit)
		break;

	     frame[num].choice++;
	     frame[num].index++;

	     if (frame[num].index >= (long)frame[num].el_limit)
		frame[num].index--;

	     if (frame[num].index >= (long)frame[num].window_size)
	     {
		frame[num].index--;
		if (frame[num].choice < (long)frame[num].el_limit)
		{
		   frame[num].top++;
		   frame[num].bottom = frame[num].top +
			   frame[num].window_size;
		   scroll_portal(num, 1);
		}
	     }

	     if (frame[num].choice >= (long)frame[num].el_limit)
		frame[num].choice--;

	     break;

	  case ENTER:
	     if (frame[num].el_func)
	     {
		retCode = (frame[num].el_func)
			(frame[num].screen,
			 frame[num].el_values[frame[num].choice],
			 frame[num].el_strings[frame[num].choice],
			 frame[num].choice);
		if (retCode)
                   goto UnlockMutex;
	     }
	     else
             {
		retCode =
                  (frame[num].el_values[frame[num].choice]);
                goto UnlockMutex;
             }
	     break;

	  default:
#ifdef KEYBOARD_DEBUG
	     mvprintw(0, 0, "cworthy:  got key %lu                    ", key);
#endif
	     if (frame[num].key_handler)
	     {
		register ULONG retCode;

		retCode = (frame[num].key_handler)
			(frame[num].screen, key, frame[num].choice);
		if (retCode)
                   goto UnlockMutex;
	     }
	     break;
       }

    }

UnlockMutex:;
    clear_portal_focus(num);
#if (LINUX_UTIL)
    pthread_mutex_unlock(&frame[num].mutex);
#endif
    return retCode;

}

ULONG free_portal(ULONG num)
{
   register FIELD_LIST *fl;

#if (LINUX_UTIL)
   pthread_mutex_destroy(&frame[num].mutex);
#endif

   frame[num].cur_row = 0;
   frame[num].cur_column = 0;
   frame[num].selected = 0;
   frame[num].choice = 0;
   frame[num].index = 0;

   restore_menu(num);
   free_elements(num);

   frame[num].el_count = 0;
   frame[num].el_func = 0;
   frame[num].warn_func = 0;
   frame[num].owner = 0;

   while (frame[num].head)
   {
      fl = frame[num].head;
      frame[num].head = fl->next;
      free(fl);
   }
   frame[num].head = frame[num].tail = 0;
   frame[num].field_count = 0;

   return 0;

}

ULONG add_item_to_portal(ULONG num, BYTE **list, BYTE *item, ULONG index)
{
    if (list)
    {
       list[index] = item;
       return 0;
    }
    return -1;

}

ULONG display_portal_header(ULONG num)
{
   register ULONG col, len, i, adjust;

   if (!frame[num].header[0])
      return -1;

   if (frame[num].subheader[0])
   {
      adjust = 3;
      col = frame[num].start_column + 1;
   }
   else
   {
      adjust = 2;
      col = frame[num].start_column + 1;
      len = strlen((const char *) &frame[num].header[0]);
      len = (frame[num].end_column - col - len) / 2;
      if (len < 0)
	 return -1;
      col = col + len;
   }

   for (i=0; i < frame[num].end_column - frame[num].start_column; i++)
   {
      put_char(frame[num].screen,
		 frame[num].horizontal_frame,
		 frame[num].start_row + adjust,
		 frame[num].start_column + i,
		 frame[num].border_color);
   }

   put_char(frame[num].screen,
	      frame[num].left_frame,
	      frame[num].start_row + adjust,
	      frame[num].start_column,
	      frame[num].border_color);

   put_char(frame[num].screen,
	      frame[num].right_frame,
	      frame[num].start_row + adjust,
	      frame[num].end_column,
	      frame[num].border_color);

   put_string(frame[num].screen,
		(const char *)frame[num].header, NULL,
		frame[num].start_row + 1,
		col,
		frame[num].header_color);

   if (frame[num].subheader[0])
      put_string(frame[num].screen,
		(const char *)frame[num].subheader, NULL,
		frame[num].start_row + 2,
		col,
		frame[num].header_color);

   return 0;


}

ULONG draw_portal_border(ULONG num)
{
   register ULONG i;

   for (i=frame[num].start_row + 1; i < frame[num].end_row; i++)
   {
#if (WINDOWS_NT_UTIL | LINUX_UTIL | DOS_UTIL)
      put_char(frame[num].screen, frame[num].vertical_frame,
	       i,
	       frame[num].start_column,
	       frame[num].border_color);

      put_char(frame[num].screen, frame[num].vertical_frame,
	       i,
	       frame[num].end_column,
	       frame[num].border_color);
#endif
   }

   for (i=frame[num].start_column + 1; i < frame[num].end_column; i++)
   {
#if (WINDOWS_NT_UTIL | LINUX_UTIL | DOS_UTIL)
      put_char(frame[num].screen, frame[num].horizontal_frame,
	       frame[num].start_row,
	       i,
	       frame[num].border_color);

      put_char(frame[num].screen, frame[num].horizontal_frame,
	       frame[num].end_row,
	       i,
	       frame[num].border_color);
#endif
   }

   put_char(frame[num].screen, frame[num].upper_left,
	    frame[num].start_row,
	    frame[num].start_column,
	    frame[num].border_color);

   put_char(frame[num].screen, frame[num].lower_left,
	    frame[num].end_row,
	    frame[num].start_column,
	    frame[num].border_color);

   put_char(frame[num].screen, frame[num].upper_right,
	    frame[num].start_row,
	    frame[num].end_column,
	    frame[num].border_color);

   put_char(frame[num].screen, frame[num].lower_right,
	    frame[num].end_row,
	    frame[num].end_column,
	    frame[num].border_color);

   return 0;

}

ULONG activate_portal(ULONG num)
{
   register ULONG retCode;

   get_xy(frame[num].screen, (ULONG *)&frame[num].pcur_row,
	  (ULONG *)&frame[num].pcur_column);

   if (!frame[num].active)
   {
      frame[num].active = TRUE;
      save_menu(num);
      fill_menu(num, ' ', frame[num].fill_color);
      if (frame[num].border)
      {
	 draw_portal_border(num);
	 display_portal_header(num);
      }
   }

   display_portal(num);

   retCode = get_portal_resp(num);

   restore_menu(num);

   return retCode;

}

ULONG activate_static_portal(ULONG num)
{
   get_xy(frame[num].screen, (ULONG *)&frame[num].pcur_row,
	  (ULONG *)&frame[num].pcur_column);

   if (!frame[num].active)
   {
      frame[num].active = TRUE;
      save_menu(num);
      fill_menu(num, ' ', frame[num].fill_color);
      if (frame[num].border)
      {
	 draw_portal_border(num);
	 display_portal_header(num);
      }
   }
   display_portal(num);
   return 0;

}

ULONG mask_portal(ULONG num)
{
   frame[num].mask = TRUE;
   return 0;
}

ULONG unmask_portal(ULONG num)
{
   frame[num].mask = 0;
   return 0;
}

ULONG deactivate_static_portal(ULONG num)
{
   if (!frame[num].active)
      return -1;

   restore_menu(num);
   return 0;
}

ULONG make_portal(NWSCREEN *screen,
		 const char *header,
		 const char *subheader,
		 ULONG start_row,
		 ULONG start_column,
		 ULONG end_row,
		 ULONG end_column,
		 ULONG num_lines,
		 ULONG border,
		 ULONG hcolor,
		 ULONG bcolor,
		 ULONG fcolor,
		 ULONG tcolor,
		 ULONG (*lineFunction)(NWSCREEN *, ULONG, BYTE *, ULONG),
		 ULONG (*warn_func)(NWSCREEN *, ULONG),
		 ULONG (*key_handler)(NWSCREEN *, ULONG, ULONG),
		 ULONG scroll_barPresent)
{

   register ULONG i;
   register ULONG num;

   for (num=1; num < MAX_MENU; num++)
   {
      if (!frame[num].owner)
	 break;
   }

   if (!num || num > MAX_MENU)
      return 0;

   if (start_row > screen->nlines - 1 || start_row < 0 ||
       start_column > screen->ncols - 2 || start_column < 0)
      return 0;

   frame[num].p = (BYTE *)malloc(screen->nlines * screen->ncols * 2);
   if (!frame[num].p)
   {
      free_elements(num);
      return 0;
   }
   set_data((ULONG *) frame[num].p, 0, screen->nlines * screen->ncols * 2);

   frame[num].el_storage =
	   (BYTE *)malloc(num_lines * screen->ncols);
   if (!frame[num].el_storage)
   {
      free_elements(num);
      return 0;
   }
   set_data_b((BYTE *) frame[num].el_storage, 0x20,
	      num_lines * screen->ncols);

   frame[num].el_strings =
	   (BYTE **)malloc(num_lines * sizeof(BYTE *));
   if (!frame[num].el_strings)
   {
      free_elements(num);
      return 0;
   }
   set_data((ULONG *) frame[num].el_strings, 0,
	    num_lines * sizeof(BYTE *));

   frame[num].el_values =
	   (ULONG *)malloc(num_lines * sizeof(ULONG));
   if (!frame[num].el_values)
   {
      free_elements(num);
      return 0;
   }
   set_data((ULONG *) frame[num].el_values, 0,
	    num_lines * sizeof(ULONG));

   frame[num].el_attr_storage =
                     (BYTE *)malloc(num_lines * screen->ncols);
   if (!frame[num].el_attr_storage)
   {
      free_elements(num);
      return 0;
   }
   set_data_b((BYTE *) frame[num].el_attr_storage, 0,
                       num_lines * screen->ncols);

   frame[num].el_attr =
                  (BYTE **)malloc(num_lines * sizeof(BYTE *));
   if (!frame[num].el_attr)
   {
      free_elements(num);
      return 0;
   }
   set_data((ULONG *) frame[num].el_attr, 0,
                num_lines * sizeof(BYTE *));

   for (i=0; i < num_lines; i++)
   {
      register BYTE *p = &frame[num].el_storage[i * screen->ncols];
      add_item_to_portal(num, frame[num].el_strings, p, i);
      p[screen->ncols - 1] = '\0';
   }

   for (i=0; i < num_lines; i++)
   {
      register BYTE *p = &frame[num].el_attr_storage[i * screen->ncols];
      add_item_to_portal(num, frame[num].el_attr, p, i);
      p[screen->ncols - 1] = '\0';
   }

   for (i=0; i < (HEADER_LEN - 1); i++)
   {
      if (!header || !header[i])
	 break;
      frame[num].header[i] = header[i];
   }
   frame[num].header[i] = 0x00;   // null terminate string

   for (i=0; i < (HEADER_LEN - 1); i++)
   {
      if (!subheader || !subheader[i])
	 break;
      frame[num].subheader[i] = subheader[i];
   }
   frame[num].subheader[i] = 0x00;   // null terminate string

   frame[num].start_row = start_row;
   frame[num].end_row = end_row;
   frame[num].start_column = start_column;
   frame[num].end_column = end_column;
   frame[num].screen = screen;
   frame[num].border = border;
   frame[num].num = num;
   frame[num].active = 0;
   frame[num].cur_row = 0;
   frame[num].cur_column = 0;
   frame[num].selected = 0;
   frame[num].header_color = BRITEWHITE;
   frame[num].border_color = BRITEWHITE;
   frame[num].fill_color = BRITEWHITE;
   frame[num].text_color = BRITEWHITE;
   frame[num].el_func = lineFunction;
   frame[num].warn_func = warn_func;
   frame[num].key_handler = key_handler;
   frame[num].el_limit = 0;
   frame[num].head = frame[num].tail = 0;
   frame[num].field_count = 0;
   frame[num].choice = 0;
   frame[num].index = 0;
   frame[num].top = 0;
   frame[num].bottom = 0;
   frame[num].focus = 0;
   frame[num].sleep_count = 0;
   frame[num].enable_focus = 0;
   frame[num].mask = 0;
   frame[num].saved = 0;

#if LINUX_UTIL
   // Unicode UTF8 Box Characters
   if (border == BORDER_SINGLE)
   {
      frame[num].w_upper_left       = "\u250c";
      frame[num].w_upper_right      = "\u2510";
      frame[num].w_lower_left       = "\u2514";
      frame[num].w_lower_right      = "\u2518";
      frame[num].w_left_frame       = "\u251c";
      frame[num].w_right_frame      = "\u2524";
      frame[num].w_vertical_frame   = "\u2502";
      frame[num].w_horizontal_frame = "\u2500";
      frame[num].w_up_char          = "\u25b3";
      frame[num].w_down_char        = "\u25bd";
   }
   else
   if (border == BORDER_DOUBLE)
   {
      frame[num].w_upper_left       = "\u2554";
      frame[num].w_upper_right      = "\u2557";
      frame[num].w_lower_left       = "\u255a";
      frame[num].w_lower_right      = "\u255d";
      frame[num].w_left_frame       = "\u2560";
      frame[num].w_right_frame      = "\u2563";
      frame[num].w_vertical_frame   = "\u2551";
      frame[num].w_horizontal_frame = "\u2550";
      frame[num].w_up_char          = "\u25b3";
      frame[num].w_down_char        = "\u25bd";
   }
   else
   {
      frame[num].w_upper_left       = 0;
      frame[num].w_upper_right      = 0;
      frame[num].w_lower_left       = 0;
      frame[num].w_lower_right      = 0;
      frame[num].w_left_frame       = 0;
      frame[num].w_right_frame      = 0;
      frame[num].w_vertical_frame   = 0;
      frame[num].w_horizontal_frame = 0;
      frame[num].w_up_char          = 0;
      frame[num].w_down_char        = 0;
   }
   if (scroll_barPresent)
      frame[num].w_scroll_frame     = "\u2502";
   else
      frame[num].w_scroll_frame     = 0;
#endif

#if (DOS_UTIL | WINDOWS_NT_UTIL)
   if (border == BORDER_SINGLE)
   {
      frame[num].upper_left       = 218;
      frame[num].upper_right      = 191;
      frame[num].lower_left       = 192;
      frame[num].lower_right      = 217;
      frame[num].left_frame       = 195;
      frame[num].right_frame      = 180;
      frame[num].vertical_frame   = 179;
      frame[num].horizontal_frame = 196;
      frame[num].up_char          = 0x1E;
      frame[num].down_char        = 0x1F;
   }
   else
   if (border == BORDER_DOUBLE)
   {
      frame[num].upper_left       = 201;
      frame[num].upper_right      = 187;
      frame[num].lower_left       = 200;
      frame[num].lower_right      = 188;
      frame[num].left_frame       = 204;
      frame[num].right_frame      = 185;
      frame[num].vertical_frame   = 186;
      frame[num].horizontal_frame = 205;
      frame[num].up_char          = 0x1E;
      frame[num].down_char        = 0x1F;
   }
   else
   {
      frame[num].upper_left       = 0;
      frame[num].upper_right      = 0;
      frame[num].lower_left       = 0;
      frame[num].lower_right      = 0;
      frame[num].left_frame       = 0;
      frame[num].right_frame      = 0;
      frame[num].vertical_frame   = 0;
      frame[num].horizontal_frame = 0;
      frame[num].up_char          = 0;
      frame[num].down_char        = 0;
   }
   if (scroll_barPresent)
      frame[num].scroll_frame     = 179;
   else
      frame[num].scroll_frame     = 0;

#endif

#if (LINUX_UTIL)
   if (text_mode)
   {
      if (border == BORDER_SINGLE)
      {
	 frame[num].upper_left       = '+';
	 frame[num].upper_right      = '+';
	 frame[num].lower_left       = '+';
	 frame[num].lower_right      = '+';
	 frame[num].left_frame       = '+';
	 frame[num].right_frame      = '+';
	 frame[num].vertical_frame   = '|';
	 frame[num].horizontal_frame = '-';
	 frame[num].up_char          = '*';
	 frame[num].down_char        = '*';
      }
      else
      if (border == BORDER_DOUBLE)
      {
	 frame[num].upper_left       = '*';
	 frame[num].upper_right      = '*';
	 frame[num].lower_left       = '*';
	 frame[num].lower_right      = '*';
	 frame[num].left_frame       = '*';
	 frame[num].right_frame      = '*';
	 frame[num].vertical_frame   = '|';
	 frame[num].horizontal_frame = '=';
	 frame[num].up_char          = '*';
	 frame[num].down_char        = '*';
      }
      else
      {
	 frame[num].upper_left       = 0;
	 frame[num].upper_right      = 0;
	 frame[num].lower_left       = 0;
	 frame[num].lower_right      = 0;
	 frame[num].left_frame       = 0;
	 frame[num].right_frame      = 0;
	 frame[num].vertical_frame   = 0;
	 frame[num].horizontal_frame = 0;
	 frame[num].up_char          = 0;
	 frame[num].down_char        = 0;
      }
      if (scroll_barPresent)
	 frame[num].scroll_frame     = '|';
      else
	 frame[num].scroll_frame     = 0;
   }
   else
   {
      if (border == BORDER_SINGLE)
      {
	 frame[num].upper_left       = 218 | A_ALTCHARSET;
	 frame[num].upper_right      = 191 | A_ALTCHARSET;
	 frame[num].lower_left       = 192 | A_ALTCHARSET;
	 frame[num].lower_right      = 217 | A_ALTCHARSET;
	 frame[num].left_frame       = 195 | A_ALTCHARSET;
	 frame[num].right_frame      = 180 | A_ALTCHARSET;
	 frame[num].vertical_frame   = 179 | A_ALTCHARSET;
	 frame[num].horizontal_frame = 196 | A_ALTCHARSET;
	 frame[num].up_char          = 0x1E | A_ALTCHARSET;
	 frame[num].down_char        = 0x1F | A_ALTCHARSET;
      }
      else
      if (border == BORDER_DOUBLE)
      {
	 frame[num].upper_left       = 201 | A_ALTCHARSET;
	 frame[num].upper_right      = 187 | A_ALTCHARSET;
	 frame[num].lower_left       = 200 | A_ALTCHARSET;
	 frame[num].lower_right      = 188 | A_ALTCHARSET;
	 frame[num].left_frame       = 204 | A_ALTCHARSET;
	 frame[num].right_frame      = 185 | A_ALTCHARSET;
	 frame[num].vertical_frame   = 186 | A_ALTCHARSET;
	 frame[num].horizontal_frame = 205 | A_ALTCHARSET;
	 frame[num].up_char          = 0x1E | A_ALTCHARSET;
	 frame[num].down_char        = 0x1F | A_ALTCHARSET;
      }
      else
      {
	 frame[num].upper_left       = 0;
	 frame[num].upper_right      = 0;
	 frame[num].lower_left       = 0;
	 frame[num].lower_right      = 0;
	 frame[num].left_frame       = 0;
	 frame[num].right_frame      = 0;
	 frame[num].vertical_frame   = 0;
	 frame[num].horizontal_frame = 0;
	 frame[num].up_char          = 0;
	 frame[num].down_char        = 0;
      }
      if (scroll_barPresent)
	frame[num].scroll_frame     = 179 | A_ALTCHARSET;
      else
	 frame[num].scroll_frame     = 0;
   }
#endif

   if (frame[num].header[0] || frame[num].subheader[0])
   {
      frame[num].window_size = end_row - start_row - 3;
      if (frame[num].subheader[0] && frame[num].window_size)
	 frame[num].window_size--;
   }
   else
      frame[num].window_size = end_row - start_row - 1;

   frame[num].el_count = num_lines;
   if (hcolor)
      frame[num].header_color = hcolor;
   if (bcolor)
      frame[num].border_color = bcolor;
   if (fcolor)
      frame[num].fill_color = fcolor;
   if (tcolor)
      frame[num].text_color = tcolor;

   frame[num].owner = 1;

#if (LINUX_UTIL)
   pthread_mutex_init(&frame[num].mutex, NULL);
#endif

   return num;


}

ULONG set_portal_limit(ULONG num, ULONG limit)
{
   if (!frame[num].owner)
      return -1;

   if (limit < frame[num].el_limit)
      frame[num].el_limit = limit;

   return 0;
}

ULONG write_portal_header1(ULONG num, BYTE *p, ULONG attr)
{
   register ULONG col, len, i;

   for (i=0; i < (HEADER_LEN - 1); i++)
   {
      if (!p || !p[i])
	 break;
      frame[num].header[i] = p[i];
   }
   frame[num].header[i] = 0x00;   // null terminate string

   if (frame[num].subheader[0])
   {
      col = frame[num].start_column + 1;
   }
   else
   {
      col = frame[num].start_column + 1;
      len = strlen((const char *) &frame[num].header[0]);
      len = (frame[num].end_column - col - len) / 2;
      if (len < 0)
	 return -1;
      col = col + len;
   }

   put_string(frame[num].screen, (const char *)p, NULL,
	      frame[num].start_row + 1, col,
	      ((attr & 0x0F) | frame[num].header_color));
   return 0;

}

ULONG write_portal_subheader(ULONG num, BYTE *p, ULONG attr)
{
   register ULONG col, len, i;

   for (i=0; i < (HEADER_LEN - 1); i++)
   {
      if (!p || !p[i])
	 break;
      frame[num].subheader[i] = p[i];
   }
   frame[num].subheader[i] = 0x00;   // null terminate string

   if (frame[num].subheader[0])
   {
      col = frame[num].start_column + 1;
   }
   else
   {
      col = frame[num].start_column + 1;
      len = strlen((const char *) &frame[num].subheader[0]);
      len = (frame[num].end_column - col - len) / 2;
      if (len < 0)
	 return -1;
      col = col + len;
   }

   put_string(frame[num].screen, (const char *)p, NULL,
	      frame[num].start_row + 2, col,
	      ((attr & 0x0F) | frame[num].header_color));
   return 0;

}

ULONG write_portal_line(ULONG num, ULONG row, ULONG attr)
{
   register ULONG i;
   register BYTE *v, *a;

   if (!frame[num].owner)
      return -1;

   if (row > frame[num].el_count)
      return -1;

   if (frame[num].el_strings)
   {
      v = frame[num].el_strings[row];
      if (!v)
	 return -1;

      a = frame[num].el_attr[row];
      if (!a)
	 return -1;

#if LINUX_UTIL
      if (pthread_mutex_lock(&frame[num].mutex))
         return -1;
#endif
      for (i=0; i < frame[num].screen->ncols; i++)
      {
	 v[i] = frame[num].horizontal_frame;
	 a[i] = (BYTE)(attr & 0xFF);
      }
      frame[num].el_strings[row][frame[num].screen->ncols - 1] = '\0';
      if ((row + 1) > frame[num].el_limit)
	 frame[num].el_limit = (row + 1);

#if LINUX_UTIL
      pthread_mutex_unlock(&frame[num].mutex);
#endif
      return 0;
   }
   return -1;

}

ULONG write_portal(ULONG num, const char *p, ULONG row, ULONG col, ULONG attr)
{

   register ULONG i;
   register BYTE *v, *a;

   if (!frame[num].owner)
      return -1;

   if (attr) {};

   if (row > frame[num].el_count)
      return -1;

   if (col > frame[num].screen->ncols || !*p)
      return -1;

   if (frame[num].el_strings)
   {
#if LINUX_UTIL
      if (pthread_mutex_lock(&frame[num].mutex))
         return -1;
#endif
      v = frame[num].el_strings[row];
      if (!v)
      {
#if LINUX_UTIL
         pthread_mutex_unlock(&frame[num].mutex);
#endif
	 return -1;
      }

      a = frame[num].el_attr[row];
      if (!a)
      {
#if LINUX_UTIL
         pthread_mutex_unlock(&frame[num].mutex);
#endif
	 return -1;
      }

      for (i=0; i < frame[num].screen->ncols; i++)
      {
	 if (*p && i >= col)
	 {
	    v[i] = *p++;
	    a[i] = (BYTE)(attr & 0xFF);
	 }
	 if (*p == '\0')
            break;
      }
      frame[num].el_strings[row][frame[num].screen->ncols - 1] = '\0';
      if ((row + 1) > frame[num].el_limit)
	 frame[num].el_limit = (row + 1);

#if LINUX_UTIL
      pthread_mutex_unlock(&frame[num].mutex);
#endif
      return 0;
   }
   return -1;

}

ULONG write_portal_char(ULONG num, BYTE p, ULONG row, ULONG col, ULONG attr)
{
   register BYTE *v, *a;

   if (!frame[num].owner)
      return -1;

   if (attr) {};

   if (row > frame[num].el_count)
      return -1;

   if (col > frame[num].screen->ncols)
      return -1;

   if (frame[num].el_strings)
   {
      v = frame[num].el_strings[row];
      if (!v)
	 return -1;

      a = frame[num].el_attr[row];
      if (!a)
	 return -1;

#if LINUX_UTIL
      if (pthread_mutex_lock(&frame[num].mutex))
         return -1;
#endif
      v[col] = p;
      a[col] = (BYTE)(attr & 0xFF);

      if ((row + 1) > frame[num].el_limit)
	 frame[num].el_limit = (row + 1);

#if LINUX_UTIL
      pthread_mutex_unlock(&frame[num].mutex);
#endif
      return 0;
   }
   return -1;

}

ULONG write_portal_cleol(ULONG num, const char *p, ULONG row, ULONG col,
			 ULONG attr)
{

   register ULONG i;
   register BYTE *v, *a;

   if (!frame[num].owner)
      return -1;

   if (attr) {};

   if (row > frame[num].el_count)
      return -1;

   if (col > frame[num].screen->ncols || !*p)
      return -1;

   if (frame[num].el_strings)
   {
#if LINUX_UTIL
      if (pthread_mutex_lock(&frame[num].mutex))
         return -1;
#endif
      v = frame[num].el_strings[row];
      if (!v)
      {
#if LINUX_UTIL
         pthread_mutex_unlock(&frame[num].mutex);
#endif
	 return -1;
      }

      a = frame[num].el_attr[row];
      if (!a)
      {
#if LINUX_UTIL
         pthread_mutex_unlock(&frame[num].mutex);
#endif
	 return -1;
      }

      for (i=0; i < frame[num].screen->ncols; i++)
      {
	 if (*p && i >= col)
	 {
	    v[i] = *p++;
	    a[i] = (BYTE)(attr & 0xFF);
	 }
	 else if (i >= col)
	    v[i] = ' ';
      }
      frame[num].el_strings[row][frame[num].screen->ncols - 1] = '\0';
      if ((row + 1) > frame[num].el_limit)
	 frame[num].el_limit = (row + 1);

#if LINUX_UTIL
      pthread_mutex_unlock(&frame[num].mutex);
#endif
      return 0;
   }
   return -1;

}

ULONG write_screen_comment_line(NWSCREEN *screen, const char *p, ULONG attr)
{
    put_string_cleol(screen, (const char *)p, NULL, screen->nlines - 1, attr);
    return 0;
}

void display_portal(ULONG num)
{
    register ULONG i, row, col, count, width;

    if (!frame[num].choice)
    {
       frame[num].index = 0;
       frame[num].top = 0;
       frame[num].bottom = frame[num].top +
	       frame[num].window_size;
    }

    if (strlen((const char *)frame[num].header) ||
        strlen((const char *)frame[num].subheader))
    {
       row = frame[num].start_row + 3;
       if (strlen((const char *)frame[num].subheader))
	  row++;
    }
    else
       row = frame[num].start_row + 1;

    count = frame[num].window_size;
    col =  frame[num].start_column + 1;
    width = frame[num].end_column - frame[num].start_column;
    if (width >= 1)
       width -= 1;

    for (i=0; i < count; i++)
    {
       if ((i < frame[num].el_count) &&
	   frame[num].el_strings &&
	   frame[num].el_strings[i])
       {
	  if (frame[num].scroll_frame)
	  {
	     put_char(frame[num].screen,
		    ' ',
		    row + i, col,
                    frame[num].fill_color |
		    frame[num].text_color);

	     put_char(frame[num].screen,
		    frame[num].scroll_frame,
		    row + i, col + 1,
                    frame[num].fill_color |
		    frame[num].text_color);

	     put_string_to_length(frame[num].screen,
	            (const char *)frame[num].el_strings[i],
		    frame[num].el_attr[i],
		    row + i, col + 2,
                    frame[num].fill_color |
		    frame[num].text_color,
		    width - 2);
	  }
	  else
	  {
	     put_string_to_length(frame[num].screen,
	            (const char *)frame[num].el_strings[i],
	            frame[num].el_attr[i],
		    row + i, col,
                    frame[num].fill_color |
		    frame[num].text_color,
		    width);
	  }
       }
       else
       {
	  if (frame[num].scroll_frame)
	  {
	     put_char(frame[num].screen,
		    ' ',
		    row + i, col,
                    frame[num].fill_color |
		    frame[num].text_color);

	     put_char(frame[num].screen,
		    frame[num].scroll_frame,
		    row + i, col + 1,
                    frame[num].fill_color |
		    frame[num].text_color);

	  }
       }
    }
#if (LINUX_UTIL)
    refresh_screen();
#endif
}

ULONG update_portal(ULONG num)
{

    register ULONG i, row, col, width;

#if (LINUX_UTIL)
    if (screensaver)
       return -1;
#endif

    if (frame[num].mask)
       return -1;

    if (!frame[num].active)
       return -1;

    if (!frame[num].selected)
       return -1;

#if (LINUX_UTIL)
    if (pthread_mutex_lock(&frame[num].mutex))
       return -1;
#endif

    if (strlen((const char *)frame[num].header) ||
        strlen((const char *)frame[num].subheader))
    {
       row = frame[num].start_row + 3;
       if (strlen((const char *)frame[num].subheader))
	  row++;
    }
    else
       row = frame[num].start_row + 1;

    col = frame[num].start_column + 1;
    width = frame[num].end_column - frame[num].start_column;
    if (width >= 1)
       width -= 1;

    for (i=0; i < frame[num].window_size; i++)
    {
       if (i < frame[num].el_limit)
       {
	  if (frame[num].el_strings[frame[num].top + i])
	  {
	     if (frame[num].scroll_frame)
	     {
		put_char(frame[num].screen,
			 ' ',
			 row + i, col,
			 (row + i == row + frame[num].index)
			 ? bar_attribute :
			 frame[num].fill_color |
			 frame[num].text_color);

		put_char(frame[num].screen,
			 frame[num].scroll_frame,
			 row + i, col + 1,
			 (row + i == row + frame[num].index)
			 ? bar_attribute :
			 frame[num].fill_color |
			 frame[num].text_color);

		put_string_to_length(frame[num].screen,
		    (const char *)frame[num].el_strings[frame[num].top + i],
		    frame[num].el_attr[frame[num].top + i],
		    row + i, col + 2,
		    (row + i == row + frame[num].index)
		    ? bar_attribute : frame[num].fill_color |
		    frame[num].text_color, width - 2);
	     }
	     else
	     {
		put_string_to_length(frame[num].screen,
		    (const char *)frame[num].el_strings[frame[num].top + i],
		    frame[num].el_attr[frame[num].top + i],
		    row + i, col,
		    (row + i == row + frame[num].index) ? bar_attribute :
		    frame[num].fill_color | frame[num].text_color, width);
	     }
	  }
       }
    }

    if (frame[num].el_limit > frame[num].window_size &&
	frame[num].top)
    {
	  put_char(frame[num].screen,
		  frame[num].up_char,
		  row,
		  col,
		  get_char_attribute(frame[num].screen, row, col));
    }
    else
    {
	  put_char(frame[num].screen,
		  ' ',
		  row,
		  col,
		  get_char_attribute(frame[num].screen, row, col));
    }

    if (frame[num].el_limit > frame[num].window_size
        && frame[num].bottom < (long)frame[num].el_limit)
    {
	  put_char(frame[num].screen,
		  frame[num].down_char,
		  row + frame[num].window_size - 1,
		  col,
		  get_char_attribute(frame[num].screen,
				     row + frame[num].window_size - 1,
				     col));
    }
    else
    {
	  put_char(frame[num].screen,
		  ' ',
		  row + frame[num].window_size - 1,
		  col,
		  get_char_attribute(frame[num].screen,
				     row + frame[num].window_size - 1,
				     col));
    }

#if (LINUX_UTIL)
    pthread_mutex_unlock(&frame[num].mutex);
    refresh_screen();
#endif
    return 0;

}

ULONG update_static_portal(ULONG num)
{

    register ULONG i, row, col, width;

#if (LINUX_UTIL)
    if (screensaver)
       return -1;
#endif

    if (frame[num].mask)
       return -1;

#if (LINUX_UTIL)
    if (pthread_mutex_lock(&frame[num].mutex))
       return -1;
#endif

    if (strlen((const char *)frame[num].header) ||
        strlen((const char *)frame[num].subheader))
    {
       row = frame[num].start_row + 3;
       if (strlen((const char *)frame[num].subheader))
	  row++;
    }
    else
       row = frame[num].start_row + 1;

    col = frame[num].start_column + 1;
    width = frame[num].end_column - frame[num].start_column;
    if (width >= 1)
       width -= 1;

    for (i=0; i < frame[num].window_size; i++)
    {
       if (i < frame[num].el_limit)
       {
	  if (frame[num].el_strings[frame[num].top + i])
	  {
	     if (frame[num].scroll_frame)
             {
		put_char(frame[num].screen,
		    ' ',
		    row + i, col,
		    ((row + i == row + frame[num].index) &&
		     frame[num].focus)
		    ? bar_attribute :
		    frame[num].fill_color |
		    frame[num].text_color);

		put_char(frame[num].screen,
		    frame[num].scroll_frame,
		    row + i, col + 1,
		    ((row + i == row + frame[num].index) &&
		     frame[num].focus)
		    ? bar_attribute :
		    frame[num].fill_color |
		    frame[num].text_color);

		put_string_to_length(frame[num].screen,
		    (const char *)frame[num].el_strings[frame[num].top + i],
	            frame[num].el_attr[frame[num].top + i],
		    row + i, col + 2,
		    ((row + i == row + frame[num].index) &&
		     frame[num].focus) ? bar_attribute :
		    frame[num].fill_color | frame[num].text_color,
		    width - 2);
	     }
	     else
	     {
		put_string_to_length(frame[num].screen,
		    (const char *)frame[num].el_strings[frame[num].top + i],
	            frame[num].el_attr[frame[num].top + i],
		    row + i, col,
		    ((row + i == row + frame[num].index) &&
		     frame[num].focus) ? bar_attribute
		    : frame[num].fill_color | frame[num].text_color,
		    width);
	     }
	  }
       }
    }

    if (frame[num].el_limit > frame[num].window_size &&
        frame[num].top)
    {
	  put_char(frame[num].screen,
		  frame[num].up_char,
		  row,
		  col,
		  get_char_attribute(frame[num].screen, row, col));
    }
    else
    {
	  put_char(frame[num].screen,
		  ' ',
		  row,
		  col,
		  get_char_attribute(frame[num].screen, row, col));
    }

    if ((frame[num].el_limit > frame[num].window_size)
	&& (frame[num].bottom < (long)(frame[num].el_limit)))
    {
	  put_char(frame[num].screen,
		  frame[num].down_char,
		  row + frame[num].window_size - 1,
		  col,
		  get_char_attribute(frame[num].screen,
				     row + frame[num].window_size - 1,
				     col));
    }
    else
    {
	  put_char(frame[num].screen,
		  ' ',
		  row + frame[num].window_size - 1,
		  col,
		  get_char_attribute(frame[num].screen,
				     row + frame[num].window_size - 1,
				     col));
    }

#if (LINUX_UTIL)
    pthread_mutex_unlock(&frame[num].mutex);
    refresh_screen();
#endif
    return 0;

}

ULONG clear_portal_storage(ULONG num)
{
   register ULONG i, j;
   register BYTE *v, *a;

   if (!frame[num].owner)
      return -1;

#if LINUX_UTIL
   if (pthread_mutex_lock(&frame[num].mutex))
      return -1;
#endif

   for (i=0; i < frame[num].el_count; i++)
   {
      v = frame[num].el_strings[i];
      if (!v)
      {
#if LINUX_UTIL
         pthread_mutex_unlock(&frame[num].mutex);
#endif
	 return -1;
      }

      a = frame[num].el_attr[i];
      if (!a)
      {
#if LINUX_UTIL
         pthread_mutex_unlock(&frame[num].mutex);
#endif
	 return -1;
      }

      for (j=0; (j < frame[num].screen->ncols); j++)
      {
	 *v++ = ' ';
	 *a++ = '\0';
      }
      frame[num].el_strings[i][frame[num].screen->ncols - 1] = '\0';

   }
#if LINUX_UTIL
   pthread_mutex_unlock(&frame[num].mutex);
#endif

   return 0;

}

ULONG clear_portal(ULONG num)
{
   register ULONG i, j;
   register BYTE *v, *a;

   if (!frame[num].owner)
      return -1;

   for (i=0; i < frame[num].el_count; i++)
   {
#if LINUX_UTIL
      if (pthread_mutex_lock(&frame[num].mutex))
         return -1;
#endif
      v = frame[num].el_strings[i];
      if (!v)
      {
#if LINUX_UTIL
         pthread_mutex_unlock(&frame[num].mutex);
#endif
	 return -1;
      }

      a = frame[num].el_attr[i];
      if (!a)
      {
#if LINUX_UTIL
         pthread_mutex_unlock(&frame[num].mutex);
#endif
	 return -1;
      }

      for (j=0; (j < frame[num].screen->ncols); j++)
      {
	 *v++ = ' ';
	 *a++ = '\0';
      }
      frame[num].el_strings[i][frame[num].screen->ncols - 1] = '\0';

#if LINUX_UTIL
      pthread_mutex_unlock(&frame[num].mutex);
#endif
   }
   frame[num].el_limit = 0;

   frame[num].choice = 0;
   frame[num].index = 0;
   frame[num].top = 0;
   frame[num].bottom = frame[num].top + frame[num].window_size;

#if (LINUX_UTIL)
   refresh_screen();
#endif
   return 0;

}

ULONG disable_portal_input(ULONG num)
{
   register ULONG retCode = frame[num].key_mask;
   frame[num].key_mask = TRUE;
   return retCode;
}

ULONG enable_portal_input(ULONG num)
{
   register ULONG retCode = frame[num].key_mask;
   frame[num].key_mask = 0;
   return retCode;
}

ULONG error_portal(const char *p, ULONG row)
{
    register ULONG portal;
    register ULONG len, startCol, endCol;

    len = strlen((const char *)p);
    if (!console_screen.ncols || (console_screen.ncols < len))
       return -1;

    startCol = ((console_screen.ncols - len) / 2) - 2;
    endCol = console_screen.ncols - startCol;
    portal = make_portal(&console_screen,
		       0,
		       0,
		       row,
		       startCol,
		       row + 4,
		       endCol,
		       3,
		       BORDER_SINGLE,
		       error_attribute,
		       error_attribute,
		       error_attribute,
		       error_attribute,
		       0,
		       0,
		       0,
		       FALSE);

    if (!portal)
       return -1;

    write_portal(portal, (const char *)p, 1, 1, error_attribute);

    activate_static_portal(portal);

    get_key();

    if (portal)
    {
       deactivate_static_portal(portal);
       free_portal(portal);
    }
    return 0;
}

ULONG message_portal(const char *p, ULONG row, ULONG attr, ULONG wait)
{
    register ULONG portal;
    register ULONG len, startCol, endCol;

    len = strlen((const char *)p);
    if (!console_screen.ncols || (console_screen.ncols < len))
       return -1;

    startCol = ((console_screen.ncols - len) / 2) - 2;
    endCol = console_screen.ncols - startCol;
    portal = make_portal(&console_screen,
		       0,
		       0,
		       row,
		       startCol,
		       row + 4,
		       endCol,
		       3,
		       BORDER_SINGLE,
		       (attr & ~BLINK),
		       (attr & ~BLINK),
		       (attr & ~BLINK),
		       attr,
		       0,
		       0,
		       0,
		       FALSE);

    if (!portal)
       return -1;

    write_portal(portal, (const char *)p, 1, 1, attr);

    activate_static_portal(portal);

    if (wait)
       get_key();

    if (portal)
    {
       deactivate_static_portal(portal);
       free_portal(portal);
    }
    return 0;
}

ULONG create_message_portal(const char *p, ULONG row, ULONG attr)
{
    register ULONG portal;
    register ULONG len, startCol, endCol;

    len = strlen((const char *)p);
    if (!console_screen.ncols || (console_screen.ncols < len))
       return -1;

    startCol = ((console_screen.ncols - len) / 2) - 2;
    endCol = console_screen.ncols - startCol;
    portal = make_portal(&console_screen,
		       0,
		       0,
		       row,
		       startCol,
		       row + 4,
		       endCol,
		       3,
		       BORDER_SINGLE,
		       (attr & ~BLINK),
		       (attr & ~BLINK),
		       (attr & ~BLINK),
		       attr,
		       0,
		       0,
		       0,
		       FALSE);

    if (!portal)
       return -1;

    write_portal(portal, (const char *)p, 1, 1, attr);

    activate_static_portal(portal);

    return portal;
}

ULONG close_message_portal(ULONG portal)
{
    if (portal > MAX_MENU)
       return -1;

    if (portal)
    {
       deactivate_static_portal(portal);
       free_portal(portal);
    }
    return 0;
}

ULONG confirm_menu(const char *confirm, ULONG row, ULONG attr)
{
    register ULONG mNum, retCode, len, startCol;

    len = strlen((const char *)confirm);
    if (!console_screen.ncols || (console_screen.ncols < len))
       return -1;

    startCol = ((console_screen.ncols - len) / 2) - 2;
    mNum = make_menu(&console_screen,
		     (const char *)confirm,
		     row,
		     startCol,
		     2,
		     BORDER_DOUBLE,
		     attr,
		     attr,
		     BRITEWHITE | (attr & 0xF0),
		     BRITEWHITE | (attr & 0xF0),
		     0,
		     0,
		     0,
		     TRUE,
		     0);

    add_item_to_menu(mNum, "No", 0);
    add_item_to_menu(mNum, "Yes", 1);

    retCode = activate_menu(mNum);
    if (retCode == (ULONG) -1)
       retCode = 0;

    free_menu(mNum);

    return retCode;
}

void insert_field_node(ULONG num, FIELD_LIST *fl)
{
    if (!frame[num].head)
    {
       frame[num].head = fl;
       frame[num].tail = fl;
       fl->next = fl->prior = 0;
    }
    else
    {
       frame[num].tail->next = fl;
       fl->next = 0;
       fl->prior = frame[num].tail;
       frame[num].tail = fl;
    }
    frame[num].field_count++;
    return;
}

ULONG add_field_to_portal(ULONG num, ULONG row, ULONG col, ULONG attr,
			BYTE *prompt, ULONG plen,
			BYTE *buffer, ULONG buflen,
			BYTE **menu_strings, ULONG menu_items,
			ULONG menu_default, ULONG *menu_result,
			ULONG flags, int (*hide)(ULONG, FIELD_LIST *),
                        void *priv)
{
    register FIELD_LIST *fl;

    if (!frame[num].owner)
       return -1;

    if (row > frame[num].el_count)
       return -1;

    if (col > (frame[num].screen->ncols - 1))
       return -1;

    if ((col + plen + buflen + 1) > (frame[num].screen->ncols - 1))
       return -1;

    fl = (FIELD_LIST *)malloc(sizeof(FIELD_LIST));
    if (!fl)
       return -1;

    memset(fl, 0, sizeof(FIELD_LIST));

    fl->next = fl->prior = 0;
    fl->portal = num;
    fl->row = row;
    fl->col = col;
    fl->prompt = prompt;
    fl->plen = plen;
    fl->buffer = buffer;
    fl->buflen = buflen;
    fl->flags = flags;
    fl->attr = attr;
    fl->hide = hide;
    fl->priv = priv;
    fl->menu_strings = menu_strings;
    fl->menu_items = menu_items;
    fl->menu_result = menu_result;
    fl->result = menu_default;

    insert_field_node(num, fl);

    return 0;
}

ULONG input_portal_fields(ULONG num)
{
   register ULONG ccode, i, temp, insert = 0;
   register ULONG key, menuRow, len, screenRow, menuCol, adj;
   register FIELD_LIST *fl, *fl_search;
   register BYTE *p;

   if (!frame[num].owner)
      return -1;

   if (strlen((const char *)frame[num].header))
      adj = 3;
   else
      adj = 1;

   fl = frame[num].head;
   while (fl)
   {
      ccode = write_portal(num, (const char *)fl->prompt, fl->row, fl->col,
			   fl->attr);
      if (ccode)
	 return ccode;
      fl->pos = strlen((const char *)fl->buffer);
      fl = fl->next;
   }

   update_static_portal(num);
   enable_cursor(insert);

   fl = frame[num].head;
   while (fl)
   {
      if (fl->menu_items && fl->menu_strings)
      {
	 write_portal(num,
		   (const char *)fl->menu_strings[fl->result],
		   fl->row, fl->col + fl->plen, fl->attr);
      }
      else
      {
	 write_portal(num,
		   (const char *)fl->buffer,
		   fl->row, fl->col + fl->plen,
                   (fl->hide && fl->hide(num, fl)) ? GRAY | BGBLUE : fl->attr);
      }
      fl = fl->next;
   }
   update_static_portal(num);

   // go to first field row,col coordinates
   fl = frame[num].head;
   if (fl->menu_items && fl->menu_strings)
   {
      write_portal(num,
           (const char *)fl->menu_strings[fl->result],
	   fl->row, fl->col + fl->plen, field_attribute);
   }
   else
      write_portal(num,
	   (const char *)fl->buffer,
	   fl->row, fl->col + fl->plen,	field_attribute);

   field_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);

   for (;;)
   {
      key = get_key();
      switch (key)
      {
          // screensaver return key
          case 0:
	     update_static_portal(num);
             break;

#if (LINUX_UTIL)
	 case ESC:
	    break;
#else
	 case F3:
	    break;
#endif

	 case F1: case F2: case F4: case F5: case F6:
	 case F7: case F8: case F9: case F11: case F12:
	    break;

	 case F10:
	    (fl->buflen)
	    ? (fl->buffer[fl->buflen - 1] = '\0')
	    : (fl->buffer[0] = '\0');

            fl = frame[num].head;
            while (fl)
            {
               if (fl->menu_items && fl->menu_strings)
               {
	          write_portal(num,
                           (const char *)fl->menu_strings[fl->result],
			   fl->row, fl->col + fl->plen, fl->attr);
               }
               else
               {
	          write_portal(num, (const char *)fl->buffer,
			   fl->row, fl->col + fl->plen,
                           (fl->hide && fl->hide(num, fl))
			   ? GRAY | BGBLUE : fl->attr);
               }
               fl = fl->next;
            }
	    disable_cursor();
	    update_static_portal(num);
	    return 0;

#if LINUX_UTIL
	 case VT220_HOME:
#endif
	 case HOME:
	    if (fl->menu_items || fl->menu_strings)
	       break;
	    fl->pos = 0;
	    field_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
	    break;

#if LINUX_UTIL
	 case VT220_END:
#endif
	 case END:
	    if (fl->menu_items || fl->menu_strings)
	       break;
	    fl->pos = strlen((const char *)fl->buffer);
	    field_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
	    break;

	 case UP_ARROW:
	    (fl->buflen)
	    ? (fl->buffer[fl->buflen - 1] = '\0')
	    : (fl->buffer[0] = '\0');

	    if (fl->menu_items && fl->menu_strings)
	    {
               write_portal(num,
		   (const char *)fl->menu_strings[fl->result],
		   fl->row, fl->col + fl->plen,
		   fl->attr);
	    }
	    else
               write_portal(num,
			   (const char *)fl->buffer,
			   fl->row, fl->col + fl->plen,
                           (fl->hide && fl->hide(num, fl))
			   ? GRAY | BGBLUE : fl->attr);

	    fl = fl->prior;
            while (fl && fl->hide && fl->hide(num, fl))
	       fl = fl->prior;

	    if (!fl)
	       fl = frame[num].tail;

	    if (!fl)
	       return -1;

	    if (fl)
	    {
	       if (fl->menu_items && fl->menu_strings)
	       {
                  write_portal(num,
			   (const char *)fl->menu_strings[fl->result],
			   fl->row, fl->col + fl->plen,
			   field_attribute);
	       }
	       else
                  write_portal(num,
			   (const char *)fl->buffer,
			   fl->row, fl->col + fl->plen,
			   field_attribute);

	       fl->pos = strlen((const char *)fl->buffer);
	       field_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
	    }
	    break;

	 case DOWN_ARROW:
	    (fl->buflen)
	    ? (fl->buffer[fl->buflen - 1] = '\0')
	    : (fl->buffer[0] = '\0');

	    if (fl->menu_items && fl->menu_strings)
	    {
               write_portal(num,
			   (const char *)fl->menu_strings[fl->result],
			   fl->row, fl->col + fl->plen, fl->attr);
	    }
	    else
               write_portal(num,
			   (const char *)fl->buffer,
			   fl->row, fl->col + fl->plen,
                           (fl->hide && fl->hide(num, fl))
			   ? GRAY | BGBLUE : fl->attr);

	    fl = fl->next;
            while (fl && fl->hide && fl->hide(num, fl))
	       fl = fl->next;

	    if (!fl)
	       fl = frame[num].head;

	    if (!fl)
	       return -1;

            if (fl)
	    {
	       if (fl->menu_items && fl->menu_strings)
	       {
                  write_portal(num,
			   (const char *)fl->menu_strings[fl->result],
			   fl->row, fl->col + fl->plen, field_attribute);
	       }
	       else
                  write_portal(num,
			   (const char *)fl->buffer,
			   fl->row, fl->col + fl->plen, field_attribute);

	       fl->pos = strlen((const char *)fl->buffer);
	       field_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
	    }
	    break;

	 case PG_UP:
	 case PG_DOWN:
            break;

	 case INS:
            if (insert) {
               insert = 0;
	       enable_cursor(insert);
	    } else {
               insert = 1;
	       enable_cursor(insert);
	    }
            break;

	 case DEL:
	    p = (BYTE *) &fl->buffer[fl->pos];
	    temp = fl->pos;
	    p++;
	    while ((*p) && (temp < fl->buflen))
	    {
	       fl->buffer[temp++] = *p++;
	    }
	    fl->buffer[temp] = '\0';

            write_portal(num,
		   (const char *)fl->buffer,
		   fl->row, fl->col + fl->plen, field_attribute);

	    field_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
	    break;

	 case LEFT_ARROW:
	    if (fl->menu_items || fl->menu_strings)
		     break;

	    if (fl->pos)
	    {
	       fl->pos--;
	       field_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
	    }
	    break;

	 case RIGHT_ARROW:
	    if (fl->menu_items || fl->menu_strings)
	       break;

	    if (fl->pos < (fl->buflen - 1))
	    {
	       fl->pos++;
	       field_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
	    }
	    break;


	 case SPACE:
	    if (fl->menu_items && fl->menu_strings)
	    {
	       len = (fl->menu_items + 1) / 2;
	       screenRow = fl->row + adj + frame[fl->portal].start_row;
	       if (screenRow >= len)
		  menuRow = screenRow - len;
	       else
		  menuRow = screenRow;

	       len = 0;
	       for (i=0; i < fl->menu_items; i++)
	       {
		  if (strlen((const char *)fl->menu_strings[i]) > len)
		     len = strlen((const char *)fl->menu_strings[i]);
	       }
	       menuCol = ((console_screen.ncols - len) / 2);

	       fl->menu_portal = make_menu(&console_screen,
						0,
						menuRow,
						menuCol,
						fl->menu_items,
						BORDER_DOUBLE,
						field_popup_highlight_attribute,
						field_popup_highlight_attribute,
						field_popup_normal_attribute,
						field_popup_normal_attribute,
						0,
						0,
						0,
						TRUE,
						0);

	       if (!fl->menu_portal)
		  break;

	       for (i=0; i < fl->menu_items; i++)
		  add_item_to_menu(fl->menu_portal,
			 (const char *)fl->menu_strings[i], i + 1);

	       disable_cursor();
	       frame[fl->menu_portal].choice = fl->result;
	       ccode = activate_menu(fl->menu_portal);
	       enable_cursor(insert);

	       if (ccode == (ULONG) -1)
		  fl->result = 0;
	       else
	       if (ccode)
		  fl->result = ccode - 1;
	       else
		  fl->result = ccode;

	       if (fl->menu_result)
		  *(fl->menu_result) = fl->result;

	       if (fl->menu_portal)
		  free_menu(fl->menu_portal);
	       fl->menu_portal = 0;

               write_portal(num,
		   (const char *)fl->menu_strings[fl->result],
		   fl->row, fl->col + fl->plen, field_attribute);

	       field_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
	       break;
	    }

	    if (fl->pos < (fl->buflen - 1))
	    {
               if (!insert)
               {
	          fl->buffer[fl->pos] = ' ';

		  write_portal_char(num, ' ', fl->row, fl->col + fl->plen +
		       fl->pos, fl->attr);

	          write_portal_char(num, ' ', fl->row, fl->col + fl->plen +
		       fl->pos, field_attribute);

	          fl->pos++;
	          field_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
               }
               else
               {
		  if (strlen((const char *)fl->buffer) < (fl->buflen - 1))
                  {
	             for (i = fl->buflen; i > fl->pos; i--)
		        fl->buffer[i] = fl->buffer[i - 1];

	             fl->buffer[fl->pos] = ' ';
	             write_portal_char(num, ' ', fl->row, fl->col + fl->plen +
				       fl->pos, fl->attr);

	             write_portal_char(num, ' ', fl->row, fl->col + fl->plen +
			 fl->pos, field_attribute);

                     write_portal(num,
			   (const char *)fl->buffer,
			   fl->row, fl->col + fl->plen, field_attribute);

	             fl->pos++;
	             field_set_xy(num, fl->row, fl->col + fl->plen +
				  fl->pos + 1);
                  }
               }
	    }
	    break;

	 case BKSP:
	    if (fl->menu_items || fl->menu_strings)
	       break;

	    if (fl->pos)
	    {
	       fl->pos--;
	       if (!fl->buffer[fl->pos + 1])
               {
	          write_portal_char(num, ' ', fl->row, fl->col + fl->plen +
				       fl->pos, fl->attr);

		  write_portal_char(num, ' ',
			 fl->row, fl->col + fl->plen + fl->pos,
			 field_attribute);

		  fl->buffer[fl->pos] = '\0';
               }
	       else
               {
                  for (i = fl->pos; i < fl->buflen; i++)
	             fl->buffer[i] = fl->buffer[i + 1];
                  fl->buffer[i] = '\0';
               }

               write_portal(num,
			   (const char *)fl->buffer,
			   fl->row, fl->col + fl->plen, field_attribute);

	       field_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
	    }
	    break;

	 case ENTER:
	    if (fl->menu_items && fl->menu_strings)
	    {
	       len = (fl->menu_items + 1) / 2;
	       screenRow = fl->row + adj + frame[fl->portal].start_row;
	       if (screenRow >= len)
		  menuRow = screenRow - len;
	       else
		  menuRow = screenRow;

	       len = 0;
	       for (i=0; i < fl->menu_items; i++)
	       {
		  if (strlen((const char *)fl->menu_strings[i]) > len)
		     len = strlen((const char *)fl->menu_strings[i]);
	       }
	       menuCol = ((console_screen.ncols - len) / 2);

	       fl->menu_portal = make_menu(&console_screen,
						0,
						menuRow,
						menuCol,
						fl->menu_items,
						BORDER_DOUBLE,
						field_popup_highlight_attribute,
						field_popup_highlight_attribute,
						field_popup_normal_attribute,
						field_popup_normal_attribute,
						0,
						0,
						0,
						TRUE,
						0);

	       if (!fl->menu_portal)
		  break;

	       for (i=0; i < fl->menu_items; i++)
		  add_item_to_menu(fl->menu_portal,
                                   (const char *)fl->menu_strings[i], i + 1);

	       disable_cursor();
	       frame[fl->menu_portal].choice = fl->result;
	       ccode = activate_menu(fl->menu_portal);
	       enable_cursor(insert);

	       if (ccode == (ULONG) -1)
		  fl->result = 0;
	       else
	       if (ccode)
		  fl->result = ccode - 1;
	       else
		  fl->result = ccode;

	       if (fl->menu_result)
		  *(fl->menu_result) = fl->result;

	       if (fl->menu_portal)
		  free_menu(fl->menu_portal);
	       fl->menu_portal = 0;

               write_portal(num,
		   (const char *)fl->menu_strings[fl->result],
		   fl->row, fl->col + fl->plen, field_attribute);

	       fl_search = frame[num].head;
               while (fl_search)
               {
                  if (fl_search->menu_items && fl_search->menu_strings)
                  {
	             write_portal(num,
                           (const char *)
			   fl_search->menu_strings[fl_search->result],
			   fl_search->row, fl_search->col + fl_search->plen,
			   fl_search->attr);
                  }
                  else
                  {
	             write_portal(num, (const char *)fl_search->buffer,
			   fl_search->row, fl_search->col + fl_search->plen,
                           (fl_search->hide && fl_search->hide(num, fl_search))
			   ? GRAY | BGBLUE : fl_search->attr);
                  }
                  fl_search = fl_search->next;
               }
	    }

	    (fl->buflen)
	    ? (fl->buffer[fl->buflen - 1] = '\0')
	    : (fl->buffer[0] = '\0');

            // if on the last portal field which is a variable data field, process
	    // portal fields
	    if (fl == frame[num].tail && (fl->flags & FIELD_ENTRY))
	    {
	       disable_cursor();
	       update_static_portal(num);
	       return 0;
	    }
	    else
	    {
	       fl_search = frame[num].head;
               while (fl_search)
               {
                  if (fl_search->menu_items && fl_search->menu_strings)
                  {
	             write_portal(num,
                           (const char *)
			   fl_search->menu_strings[fl_search->result],
			   fl_search->row, fl_search->col + fl_search->plen,
			   fl_search->attr);
                  }
                  else
                  {
	             write_portal(num, (const char *)fl_search->buffer,
			   fl_search->row, fl_search->col + fl_search->plen,
                           (fl_search->hide && fl_search->hide(num, fl_search))
			   ? GRAY | BGBLUE : fl_search->attr);
                  }
                  fl_search = fl_search->next;
               }

	       fl = fl->next;
               while (fl && fl->hide && fl->hide(num, fl))
	          fl = fl->next;

	       if (!fl)
		  fl = frame[num].head;

	       if (!fl)
		   return -1;

	       if (fl->menu_items && fl->menu_strings)
	       {
                  write_portal(num,
			   (const char *)fl->menu_strings[fl->result],
			   fl->row, fl->col + fl->plen,
			   field_attribute);
	       }
	       else
                  write_portal(num,
			   (const char *)fl->buffer,
			   fl->row, fl->col + fl->plen, field_attribute);
			   fl->pos = strlen((const char *)fl->buffer);
	    }
	    field_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
            break;

	 case TAB:
	    (fl->buflen)
	    ? (fl->buffer[fl->buflen - 1] = '\0')
	    : (fl->buffer[0] = '\0');

	    if (fl->menu_items && fl->menu_strings)
	    {
               write_portal(num,
			   (const char *)fl->menu_strings[fl->result],
			   fl->row, fl->col + fl->plen, fl->attr);
	    }
	    else
               write_portal(num,
			   (const char *)fl->buffer,
			   fl->row, fl->col + fl->plen,
                           (fl->hide && fl->hide(num, fl))
			   ? GRAY | BGBLUE : fl->attr);

	    fl = fl->next;
            while (fl && fl->hide && fl->hide(num, fl))
	       fl = fl->next;

	    if (!fl)
	       fl = frame[num].head;

	    if (!fl)
	       return -1;

	    if (fl->menu_items && fl->menu_strings)
	    {
               write_portal(num,
			   (const char *)fl->menu_strings[fl->result],
			   fl->row, fl->col + fl->plen, field_attribute);
	    }
	    else
               write_portal(num,
			   (const char *)fl->buffer,
			   fl->row, fl->col + fl->plen, field_attribute);

			   fl->pos = strlen((const char *)fl->buffer);
	    field_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
	    break;

#if (LINUX_UTIL)
	  case F3:
#else
	  case ESC:
#endif
	    disable_cursor();
            fl = frame[num].head;
            while (fl)
            {
               if (fl->menu_items && fl->menu_strings)
               {
	          write_portal(num, (const char *)fl->menu_strings[fl->result],
			   fl->row, fl->col + fl->plen, fl->attr);
               }
               else
               {
	          write_portal(num, (const char *)fl->buffer,
			   fl->row, fl->col + fl->plen,
                           (fl->hide && fl->hide(num, fl))
			   ? GRAY | BGBLUE : fl->attr);
               }
               fl = fl->next;
            }
	    update_static_portal(num);
	    return 1;

	 default:
	    if (fl->menu_items && fl->menu_strings)
	    {
	       len = (fl->menu_items + 1) / 2;
	       screenRow = fl->row + adj + frame[fl->portal].start_row;
	       if (screenRow >= len)
		  menuRow = screenRow - len;
	       else
		  menuRow = screenRow;

	       len = 0;
	       for (i=0; i < fl->menu_items; i++)
	       {
		  if (strlen((const char *)fl->menu_strings[i]) > len)
		     len = strlen((const char *)fl->menu_strings[i]);
	       }
	       menuCol = ((console_screen.ncols - len) / 2);

	       fl->menu_portal = make_menu(&console_screen,
						0,
						menuRow,
						menuCol,
						fl->menu_items,
						BORDER_DOUBLE,
						field_popup_highlight_attribute,
						field_popup_highlight_attribute,
						field_popup_normal_attribute,
						field_popup_normal_attribute,
						0,
						0,
						0,
						TRUE,
						0);
	       if (!fl->menu_portal)
		  break;

	       for (i=0; i < fl->menu_items; i++)
		  add_item_to_menu(fl->menu_portal,
                                   (const char *)fl->menu_strings[i], i + 1);

	       disable_cursor();
	       frame[fl->menu_portal].choice = fl->result;
	       ccode = activate_menu(fl->menu_portal);
	       enable_cursor(insert);

	       if (ccode == (ULONG) -1)
		  fl->result = 0;
	       else
	       if (ccode)
		  fl->result = ccode - 1;
	       else
		  fl->result = ccode;

	       if (fl->menu_result)
		  *(fl->menu_result) = fl->result;

	       if (fl->menu_portal)
		  free_menu(fl->menu_portal);
	       fl->menu_portal = 0;

               write_portal(num,
			   (const char *)fl->menu_strings[fl->result],
			   fl->row, fl->col + fl->plen, field_attribute);
	       field_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
	       break;
	    }

	    if (fl->pos < (fl->buflen - 1))
	    {
               if (!insert)
               {
	          fl->buffer[fl->pos] = (BYTE)key;
	          write_portal_char(num, (BYTE)key,
			 fl->row, fl->col + fl->plen + fl->pos,
			 fl->attr);

		  write_portal_char(num, (BYTE)key,
			 fl->row, fl->col + fl->plen + fl->pos,
			 field_attribute);

	          fl->pos++;
	          field_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
               }
               else
               {
                  if (strlen((const char *)fl->buffer) < (fl->buflen - 1))
                  {
	             for (i = fl->buflen; i > fl->pos; i--)
		        fl->buffer[i] = fl->buffer[i - 1];

	             fl->buffer[fl->pos] = (BYTE)key;
	             write_portal_char(num, (BYTE)key,
			 fl->row, fl->col + fl->plen + fl->pos,
			 fl->attr);

	             write_portal_char(num, (BYTE)key,
			 fl->row, fl->col + fl->plen + fl->pos,
			 field_attribute);

                     write_portal(num,
			   (const char *)fl->buffer,
			   fl->row, fl->col + fl->plen, field_attribute);

	             fl->pos++;
	             field_set_xy(num, fl->row, fl->col + fl->plen +
				  fl->pos + 1);
                  }
               }
	    }
	    break;
      }
   }
   return 0;

}

