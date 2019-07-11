/***************************************************************************
*
*   Copyright(c) Jeff V. Merkey 1997-2019.  All rights reserved.
*   Open CWorthy Look Alike Terminal Library.
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

#ifndef _NWMENU_
#define _NWMENU_

#define LINUX_UTIL       1
#define DOS_UTIL         0
#define WINDOWS_NT_UTIL  0

#if LINUX_UTIL
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <ctype.h>
#include <wchar.h>
#include <locale.h>
#include <pthread.h>
#include <netdb.h>
#include <netipx/ipx.h>
#include <neteconet/ec.h>
#include <linux/if_slip.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if_arp.h>
#include <neteconet/ec.h>
#include <linux/atalk.h>
#include <linux/netdevice.h>
#include <asm/types.h>
#include <asm/param.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <time.h>
#include <ncurses.h>
#include <linux/hdreg.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <sys/utsname.h>

#ifndef LONGLONG
typedef long long LONGLONG;
#endif

typedef unsigned long  ULONG;
typedef unsigned short WORD;
typedef unsigned char  BYTE;

#define SNPRINTF snprintf
#define MAX_MENU        25
#endif

#if (DOS_UTIL)
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <dos.h>
#include <conio.h>
#include <sys/farptr.h>
#include <sys/movedata.h>
#include <sys/segments.h>
#include <sys/stat.h>
#include <dpmi.h>
#include <time.h>

#ifndef LONGLONG
typedef long long LONGLONG;
#endif
#ifndef ULONG
typedef unsigned long  ULONG;
#ifndef WORD
#endif
typedef unsigned short WORD;
#endif
#ifndef BYTE
typedef unsigned char  BYTE;
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define SNPRINTF snprintf
#define MAX_MENU        10
#endif

#if (WINDOWS_NT_UTIL)
#include <windows.h>
#include <winioctl.h>
#include <winuser.h>
#include <winsock.h>
#include <stdarg.h>

typedef UCHAR BYTE;
typedef USHORT WORD;

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <conio.h>

#define SNPRINTF snprintff
#define MAX_MENU        25
#endif


//
//  keyboard scan code values after translation
//

#if (DOS_UTIL | WINDOWS_NT_UTIL)

#define   F1           0x13B
#define   F2           0x13C
#define   F3           0x13D
#define   F4           0x13E
#define   F5           0x13F
#define   F6           0x140
#define   F7           0x141
#define   F8           0x142
#define   F9           0x143
#define   F10          0x144
#define   F11          0x185
#define   F12          0x186
#define   INS          0x152
#define   DEL          0x153
#define   ENTER        0x0D
#define   ESC          0x1B
#define   BKSP         0x08
#define   SPACE        0x20
#define   TAB          0x09
#define   HOME         0x147
#define   END          0x14F
#define   PG_UP        0x149
#define   PG_DOWN      0x151
#define   UP_ARROW     0x148
#define   DOWN_ARROW   0x150
#define   LEFT_ARROW   0x14B
#define   RIGHT_ARROW  0x14D

#endif

#if (LINUX_UTIL)

#define   F1           0x109
#define   F2           0x10A
#define   F3           0x10B
#define   F4           0x10C
#define   F5           0x10D
#define   F6           0x10E
#define   F7           0x10F
#define   F8           0x110
#define   F9           0x111
#define   F10          0x112
#define   F11          0x113
#define   F12          0x114
#define   INS          0x14B
#define   DEL          0x14A
#define   ENTER        0x0D
#define   ESC          0x1B
#define   BKSP         0x107
#define   SPACE        0x20
#define   TAB          0x09
#define   HOME         0x106
#define   END          0x168
#define   PG_UP        0x153
#define   PG_DOWN      0x152
#define   UP_ARROW     0x103
#define   DOWN_ARROW   0x102
#define   LEFT_ARROW   0x104
#define   RIGHT_ARROW  0x105

#define   VT220_HOME	362
#define   VT220_END	385

#endif

#define BLINK		0x80
#define	BLACK		0x00
#define BLUE		0x01
#define GREEN		0x02
#define CYAN		0x03
#define RED		0x04
#define MAGENTA		0x05
#define BROWN		0x06
#define WHITE		0x07
#define	GRAY		0x08
#define LTBLUE		0x09
#define LTGREEN		0x0A
#define LTCYAN		0x0B
#define LTRED		0x0C
#define LTMAGENTA	0x0D
#define YELLOW		0x0E
#define BRITEWHITE	0x0F
#define	BGBLACK		0x00
#define BGBLUE		0x10
#define BGGREEN		0x20
#define BGCYAN		0x30
#define BGRED		0x40
#define BGMAGENTA	0x50
#define BGBROWN		0x60
#define BGWHITE		0x70

#define UP_CHAR         0x1E
#define DOWN_CHAR       0x1F

#define UP              1
#define DOWN            0

#define BORDER_SINGLE   1
#define BORDER_DOUBLE   2

#define HEADER_LEN     80

typedef struct _NWSCREEN
{
   BYTE *p_vidmem;	 // pointer to crnt video buffer
   BYTE *p_saved;
   ULONG crnt_row;	 // Current cursor position
   ULONG crnt_column;
   ULONG ncols;		 // Virtual Screen Size
   ULONG nlines;
   ULONG vid_mode;	 // 0x00 = 80x25 VGA color text
   ULONG norm_vid;	 // 0x07 = WhiteOnBlack
   ULONG reverse_vid;	 // 0x71 = RevWhiteOnBlack
   ULONG tab_size;
} NWSCREEN;

typedef struct _FIELD_LIST
{
   struct _FIELD_LIST *next;
   struct _FIELD_LIST *prior;
   ULONG portal;
   ULONG row;
   ULONG col;
   BYTE *prompt;
   ULONG plen;
   BYTE *buffer;
   ULONG buflen;
   ULONG flags;
   ULONG attr;
   ULONG pos;
   ULONG result;
   BYTE **menu_strings;
   ULONG menu_items;
   ULONG menu_portal;
   ULONG *menu_result;
   int (*hide)(ULONG num, struct _FIELD_LIST *fl);
   void *priv;
} FIELD_LIST;

typedef struct _CWFRAME
{
   ULONG num;
   BYTE *p;
   BYTE **el_strings;
   BYTE *el_storage;
   BYTE **el_attr;
   BYTE *el_attr_storage;
   ULONG *el_values;
   ULONG el_count;
   ULONG el_limit;
   ULONG start_row;
   ULONG end_row;
   ULONG start_column;
   ULONG end_column;
   ULONG cur_row;
   ULONG cur_column;
   ULONG pcur_row;
   ULONG pcur_column;
   ULONG window_size;
   ULONG border;
   ULONG active;
   ULONG mask;
   ULONG header_color;
   ULONG border_color;
   ULONG fill_color;
   ULONG text_color;
   BYTE header[HEADER_LEN];
   BYTE subheader[HEADER_LEN];
   NWSCREEN *screen;
   ULONG owner;
   ULONG (*el_func)(NWSCREEN *, ULONG, BYTE *, ULONG);
   ULONG (*warn_func)(NWSCREEN *, ULONG);
   long choice;
   long index;
   long top;
   long bottom;
   ULONG selected;
   ULONG (*key_handler)(NWSCREEN *, ULONG, ULONG);
   ULONG key_mask;
   ULONG screen_mode;
   ULONG nlines;
   ULONG scroll_bar;

   // window border characters
   int upper_left;
   int upper_right;
   int lower_left;
   int lower_right;
   int left_frame;
   int right_frame;
   int vertical_frame;
   int horizontal_frame;
   int up_char;
   int down_char;
   int scroll_frame;

   // utf8 window border characters
   const char *w_upper_left;
   const char *w_upper_right;
   const char *w_lower_left;
   const char *w_lower_right;
   const char *w_left_frame;
   const char *w_right_frame;
   const char *w_vertical_frame;
   const char *w_horizontal_frame;
   const char *w_up_char;
   const char *w_down_char;
   const char *w_scroll_frame;

   int focus;
   int sleep_count;
   int enable_focus;
   int focus_interval;
   int saved;

   FIELD_LIST *head;
   FIELD_LIST *tail;
   ULONG field_count;
#if LINUX_UTIL
   pthread_mutex_t mutex;
#endif
} CWFRAME;

extern ULONG bar_attribute;
extern ULONG field_attribute;
extern ULONG field_popup_highlight_attribute;
extern ULONG field_popup_normal_attribute;
extern ULONG error_attribute;

//
//   hal functions
//

ULONG init_cworthy(void);
ULONG release_cworthy(void);
void enable_cursor(void);
void disable_cursor(void);
#if (LINUX_UTIL)
int _kbhit(void);
void refresh_screen(void);
int install_screensaver(void (*ssfunc)(void));
int uninstall_screensaver(void (*ssfunc)(void));
ULONG set_screensaver_interval(ULONG seconds);
#endif

void copy_data(ULONG *src, ULONG *dest, ULONG len);
void set_data(ULONG *dest, ULONG value, ULONG len);
void set_data_b(BYTE *dest, BYTE value, ULONG len);
void hard_xy(ULONG row, ULONG col);
ULONG get_key(void);
void set_xy(NWSCREEN *screen, ULONG row, ULONG col);
void get_xy(NWSCREEN *screen, ULONG *row, ULONG *col);
void screen_write(BYTE *p);
void clear_screen(NWSCREEN *screen);
void move_string(NWSCREEN *screen,
		 ULONG srcRow, ULONG srcCol,
		 ULONG destRow, ULONG destCol,
		 ULONG length);

void put_string(NWSCREEN *screen, const char *s, BYTE *attr_array,
		ULONG row, ULONG col, ULONG attr);
void put_string_transparent(NWSCREEN *screen, const char *s, BYTE *attr_array,
			    ULONG row, ULONG col, ULONG attr);
void put_string_cleol(NWSCREEN *screen, const char *s, BYTE *attr_array,
		      ULONG line, ULONG attr);
void put_string_to_length(NWSCREEN *screen, const char *s, BYTE *attr_array,
			  ULONG row, ULONG col, ULONG attr, ULONG len);

void put_char(NWSCREEN *screen, int c, ULONG row, ULONG col, ULONG attr);
int get_char(NWSCREEN *screen, ULONG row, ULONG col);
int get_char_attribute(NWSCREEN *screen, ULONG row, ULONG col);
void put_char_cleol(NWSCREEN *screen, int c, ULONG line, ULONG attr);
void put_char_length(NWSCREEN *screen, int c, ULONG row, ULONG col, ULONG attr,
		     ULONG len);
ULONG scroll_display(NWSCREEN *screen, ULONG row, ULONG col,
		   ULONG cols, ULONG lines, ULONG up);
void set_color(ULONG attr);
void clear_color(void);
void mvputc(ULONG row, ULONG col, const chtype ch);

//
//  menu functions
//

void scroll_menu(ULONG num, ULONG up);
ULONG get_resp(ULONG num);
ULONG fill_menu(ULONG num, ULONG ch, ULONG attr);
ULONG save_menu(ULONG num);
ULONG restore_menu(ULONG num);
ULONG restore_screen(void);
ULONG free_menu(ULONG num);
ULONG display_menu_header(ULONG num);
ULONG draw_menu_border(ULONG num);
void display_menu(ULONG num);
ULONG add_item_to_menu(ULONG num, const char *item, ULONG value);
ULONG activate_menu(ULONG num);
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
	       ULONG maxlines);
ULONG menu_write_string(ULONG num, BYTE *p, ULONG row, ULONG col, ULONG attr);

//
//  portal functions
//

void scroll_portal(ULONG num, ULONG up);
ULONG get_portal_resp(ULONG num);
ULONG free_portal(ULONG num);
ULONG add_item_to_portal(ULONG num, BYTE **list, BYTE *item, ULONG index);
ULONG display_portal_header(ULONG num);
ULONG draw_portal_border(ULONG num);
void display_portal(ULONG num);
ULONG update_portal(ULONG num);
ULONG update_static_portal(ULONG num);
ULONG activate_portal(ULONG num);
ULONG activate_static_portal(ULONG num);
ULONG deactivate_static_portal(ULONG num);
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
		 ULONG scroll_barPresent);
ULONG set_portal_limit(ULONG num, ULONG limit);
ULONG write_portal(ULONG num, const char *p, ULONG row, ULONG col, ULONG attr);
ULONG write_portal_char(ULONG num, BYTE p, ULONG row, ULONG col, ULONG attr);
ULONG write_portal_cleol(ULONG num, const char *p, ULONG row, ULONG col,
			 ULONG attr);
ULONG write_screen_comment_line(NWSCREEN *screen, const char *p, ULONG attr);
ULONG disable_portal_input(ULONG num);
ULONG enable_portal_input(ULONG num);
ULONG clear_portal(ULONG num);
ULONG clear_portal_storage(ULONG num);
ULONG error_portal(const char *p, ULONG row);
ULONG confirm_menu(const char *confirm, ULONG row, ULONG attr);
ULONG write_portal_line(ULONG num, ULONG row, ULONG attr);

void enable_portal_focus(ULONG num, ULONG interval);
void disable_portal_focus(ULONG num);
void set_portal_focus(ULONG num);
void clear_portal_focus(ULONG num);
int get_sleep_count(ULONG num);
int get_portal_focus(ULONG num);
ULONG mask_portal(ULONG num);
ULONG unmask_portal(ULONG num);

ULONG message_portal(const char *p, ULONG row, ULONG attr, ULONG wait);
ULONG create_message_portal(const char *p, ULONG row, ULONG attr);
ULONG close_message_portal(ULONG portal);
ULONG write_portal_header1(ULONG num, BYTE *p, ULONG attr);
ULONG write_portal_subheader(ULONG num, BYTE *p, ULONG attr);

NWSCREEN *get_console_screen(void);
int get_screen_lines(void);
int get_screen_cols(void);
BYTE *get_term_name(void);
int is_xterm(void);
int is_linux_term(void);
int is_ansi_term(void);
void set_text_mode(int mode);
void set_mono_mode(int mode);
void set_unicode_mode(int mode);

#if WINDOWS_NT_UTIL
int snprintff(char *buf, int size, const char *fmt, ...);
#endif

//
//  field flags menu or data field
//

#define MENU_ENTRY    0x00000000
#define FIELD_ENTRY   0x00000001

ULONG add_field_to_portal(ULONG num, ULONG row, ULONG col, ULONG attr,
			BYTE *prompt, ULONG plen,
			BYTE *buffer, ULONG buflen,
			BYTE **menu_strings, ULONG menu_items,
			ULONG menu_default, ULONG *menu_result,
			ULONG flags, int (*hide)(ULONG num, FIELD_LIST *fl),
                        void *priv);
ULONG input_portal_fields(ULONG num);
#endif

