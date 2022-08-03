/***************************************************************************
*
*   Copyright(c) Jeff V. Merkey 1997-2019.  All rights reserved.
*   Open CWorthy Look Alike Terminal Library.
*
*   CW example CWorthy Application
*
****************************************************************************/

#include "cworthy.h"

ULONG mainportal;

ULONG warn_func(NWSCREEN *screen, ULONG index)
{
    ULONG mNum, retCode;

    mask_portal(mainportal);

    mNum = make_menu(screen,
		     " Exit CW ",
		     get_screen_lines() - 12,
		     ((get_screen_cols() - 1) / 2) -
                     ((strlen((const char *)"  Exit CW  ") + 2) / 2),
		     2,
		     BORDER_DOUBLE,
		     YELLOW | BGBLUE,
		     YELLOW | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     0,
		     0,
		     0,
		     TRUE,
		     0);

    add_item_to_menu(mNum, "Yes", 1);
    add_item_to_menu(mNum, "No", 0);

    retCode = activate_menu(mNum);
    if (retCode == (ULONG) -1)
       retCode = 0;

    free_menu(mNum);

    unmask_portal(mainportal);

    return retCode;
}

#define CONFIG_NAME        "  Open CWorthy Example "
#define COPYRIGHT_NOTICE1  "  Copyright (c) 1997-2019 Leaf Linux. All Rights Reserved."
#define COPYRIGHT_NOTICE2  "  "

ULONG menuFunction(NWSCREEN *screen, ULONG value, BYTE *option,
                   ULONG menu_index)
{
    int portal = 0;
    unsigned char displaybuffer[256];

    switch (value)
    {
       case 1:
          portal = make_portal(get_console_screen(),
		       " Summary",
		       0,
		       3,
		       0,
		       get_screen_lines() - 2,
		       get_screen_cols() - 1,
		       25,
		       BORDER_SINGLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       NULL,
		       0,
		       NULL,
		       TRUE);
          if (!portal)
             return 0;

          mask_portal(mainportal);

          SNPRINTF((char *)displaybuffer, sizeof(displaybuffer),
                  "  F1-Help  ESC-Return to Menu  "
		  "SPACE-Refresh  [terminal:%s]",
                  get_term_name());
          write_screen_comment_line(get_console_screen(),
				   (const char *)displaybuffer,
                                   BLUE | BGWHITE);

          activate_static_portal(portal);
          update_static_portal(portal);

          enable_portal_focus(portal, 5);
          get_portal_resp(portal);


          SNPRINTF((char *)displaybuffer, sizeof(displaybuffer),
		  "  F1-Help  ESC-Exit  TAB-View Stats  "
                  "SPACE-Refresh  [terminal:%s]", get_term_name());
          write_screen_comment_line(get_console_screen(),
				    (const char *)displaybuffer,
                                   BLUE | BGWHITE);
          if (portal)
          {
             deactivate_static_portal(portal);
             free_portal(portal);
          }
          unmask_portal(mainportal);
          break;

       case 2:
       case 3:
          break;
    }
    return 0;

}

ULONG menuKeyboardHandler(NWSCREEN *screen, ULONG key, ULONG index)
{
    BYTE displaybuffer[256];

    switch (key)
    {
       case F1:
          break;

       case TAB:
	  if (mainportal)
	  {
             SNPRINTF((char *)displaybuffer, sizeof(displaybuffer),
	            "  F1-Help  ESC-Return to Menu  "
                    "SPACE-Refresh  [terminal:%s]", get_term_name());
	     write_screen_comment_line(get_console_screen(),
				       (const char *)displaybuffer,
				       BLUE | BGWHITE);

             enable_portal_focus(mainportal, -1);
	     get_portal_resp(mainportal);

             SNPRINTF((char *)displaybuffer, sizeof(displaybuffer),
		     "  F1-Help  ESC-Exit  TAB-Switch to Stats  "
		     "SPACE-Refresh  [terminal:%s]",
		     get_term_name());
             write_screen_comment_line(get_console_screen(),
				       (const char *)displaybuffer,
				       BLUE | BGWHITE);
	  }
	  break;

       default:
	  break;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int i, retCode = 0;
    BYTE displaybuffer[256];
    int plines, mlines, mlen, menu = 0;

    for (i=0; i < argc; i++)
    {
#if (WINDOWS_NT_UTIL)
       if (!_stricmp(argv[i], "-h"))
       {
          printf("USAGE:  cw.exe text     - disable box line drawing\n");
          printf("        cw.exe -h       - this help screen\n");
          printf("        cw.exe -help    - this help screen\n");
          exit(0);
       }

       if (!_stricmp(argv[i], "-help"))
       {
          printf("USAGE:  cw.exe text     - disable box line drawing\n");
          printf("        cw.exe -h       - this help screen\n");
          printf("        cw.exe -help    - this help screen\n");
          exit(0);
       }

       if (!_stricmp(argv[i], "text"))
          set_text_mode(1);
#else

       if (!strcasecmp(argv[i], "-h"))
       {
          printf("USAGE:  cw.exe text     - disable box line drawing\n");
          printf("        cw.exe -h       - this help screen\n");
          printf("        cw.exe -help    - this help screen\n");
          exit(0);
       }

       if (!strcasecmp(argv[i], "-help"))
       {
          printf("USAGE:  cw.exe text     - disable box line drawing\n");
          printf("        cw.exe -h       - this help screen\n");
          printf("        cw.exe -help    - this help screen\n");
          exit(0);
       }
       if (!strcasecmp(argv[i], "text"))
          set_text_mode(1);
#endif
    }

    if (init_cworthy())
       return 0;

    for (i=0; i < get_screen_lines() - 1; i++)
    {
       put_char_cleol(get_console_screen(), 176, i, CYAN | BGBLUE);
    }

    unsigned long header_attr = BLUE | BGCYAN;
#if LINUX_UTIL
    if (is_xterm())
       header_attr = BRITEWHITE | BGCYAN;
    if (mono_mode)
       header_attr = BLUE | BGWHITE;
#endif
    SNPRINTF((char *)displaybuffer, sizeof(displaybuffer), CONFIG_NAME);
    put_string_cleol(get_console_screen(), (const char *)displaybuffer, NULL, 0, header_attr);

    SNPRINTF((char *)displaybuffer, sizeof(displaybuffer), COPYRIGHT_NOTICE1);
    put_string_cleol(get_console_screen(), (const char *)displaybuffer, NULL, 1, header_attr);

    SNPRINTF((char *)displaybuffer, sizeof(displaybuffer), COPYRIGHT_NOTICE2);
    put_string_cleol(get_console_screen(), (const char *)displaybuffer, NULL, 2, header_attr);

    SNPRINTF((char *)displaybuffer, sizeof(displaybuffer),
	     "  F1-Help  ESC-Exit  TAB-View Stats  "
	    "SPACE-Refresh  [terminal:%s]",
	    get_term_name());
    write_screen_comment_line(get_console_screen(), (const char *)displaybuffer,
			      BLUE | BGWHITE);

   // adjust portal and menu sizes based on screen size
    plines = get_screen_lines() >= 34
	     ? get_screen_lines() - 13
	     : get_screen_lines() - 9;
    mlines = get_screen_lines() >= 34
	     ? get_screen_lines() - 12
	     : get_screen_lines() - 8;
    mlen = 3;

    mainportal = make_portal(get_console_screen(),
		       "",
		       0,
		       3,
		       0,
		       plines,
		       get_screen_cols() - 1,
		       25,
		       BORDER_SINGLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       NULL,
		       0,
		       NULL,
		       TRUE);
    if (!mainportal)
       goto ErrorExit;

    activate_static_portal(mainportal);
    update_static_portal(mainportal);

    menu = make_menu(get_console_screen(),
		     "  Available Options  ",
		     mlines,
		     ((get_screen_cols() - 1) / 2) -
                     ((strlen("  Available Options  ") + 4) / 2),
		     mlen,
		     BORDER_DOUBLE,
		     YELLOW | BGBLUE,
		     YELLOW | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     menuFunction,
		     warn_func,
		     menuKeyboardHandler,
		     TRUE,
		     0);

    if (!menu)
	  goto ErrorExit;

    add_item_to_menu(menu, "Summary", 1);
    add_item_to_menu(menu, "Statistics", 2);
    add_item_to_menu(menu, "Message Log", 3);

    retCode = activate_menu(menu);

ErrorExit:;
    SNPRINTF((char *)displaybuffer, sizeof(displaybuffer),
	     " Exiting ... ");
    write_screen_comment_line(get_console_screen(), (const char *)displaybuffer,
			      BLUE | BGWHITE);

    if (mainportal)
    {
       deactivate_static_portal(mainportal);
       free_portal(mainportal);
    }

    if (menu)
       free_menu(menu);

    release_cworthy();
    return retCode;
}

