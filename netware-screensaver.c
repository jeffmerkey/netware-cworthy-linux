/***************************************************************************
*
*   Copyright(c) Jeff V. Merkey 1997-2022.  All rights reserved.
*
*   Portions adapted from xscreensaver loadsnake program is
*   portions Copyright (c) 2007-2011 Cosimo Streppone <cosimo@cpan.org>
*
*   Licensed under the Lesser GNU Public License (LGPL) v2.1.
*
*   Permission to use, copy, modify, distribute, and sell this software and its
*   documentation for any purpose is hereby granted without fee, provided that
*   the above copyright notice appear in all copies and that both that
*   copyright notice and this permission notice appear in supporting
*   documentation.  No representations are made about the suitability of
*   this software for any purpose.  It is provided "as is" without express or
*   implied warranty.
*
*   NetWare SMP style worm screesnsaver for Linux using ncurses
*
*   USAGE:  netware-worms [cpus|speedup]
*       i.e. worm cpus=<num=8 to 256> speedup=<divisor=1|2|3|4>
*   examples:
*       netware-worms cpus=8;
*          start worm screensaver with 8 worm threads
*       netware-worms speedup=4
*          run worm screensaver at 4X speed
*
**************************************************************************/

#include "cworthy.h"
#include "netware-screensaver.h"

static ULONG worm_chars[] =
{
   (219 | A_ALTCHARSET),
   (178 | A_ALTCHARSET),
   (177 | A_ALTCHARSET),
   (176 | A_ALTCHARSET),
};

static ULONG worm_colors[]=
{
   (LTRED | BGBLACK),
   (BLUE | BGBLACK),
   (LTGREEN | BGBLACK),
   (LTCYAN | BGBLACK),
   (YELLOW | BGBLACK),
   (BRITEWHITE | BGBLACK),
   (MAGENTA | BGBLACK),
   (BROWN | BGBLACK),
   (RED | BGBLACK),
   (LTBLUE | BGBLACK),
   (LTMAGENTA | BGBLACK),
   (GRAY | BGBLACK),
   (LTRED | BGBLACK),
   (WHITE | BGBLACK),
   (GREEN | BGBLACK),
   (CYAN | BGBLACK),
};

int worm_max_length = WORM_MAX_LEN;

static void worm_mvputc(ULONG row, ULONG col, const chtype ch)
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

         // solid block
         case 219:
         // dark shade block
         case 178:
         // medium shade block
         case 177:
         // light shade block
         case 176:
            mvaddch(row, col, ch);
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

      default:
         mvaddch(row, col, ch);
         break;
    }
    return;
}




static void worm_put_char(int c, long row, long col, ULONG attr)
{
    if (col >= COLS)
       return;
    if (row >= LINES)
       return;
    set_color(attr);
    worm_mvputc(row, col, c);
    clear_color();
    return;
}

static int worm_kbhit(void)
{
   int STDIN = 0, bytes = 0;
   struct termios init;
   struct termios term;

   // setup the terminal to check
   // for pending keystrokes
   tcgetattr(STDIN, &init);
   tcgetattr(STDIN, &term);
   term.c_lflag &= ~ICANON;
   tcsetattr(STDIN, TCSANOW, &term);

   setbuf(stdin, NULL);
   ioctl(STDIN, FIONREAD, &bytes);
   // set the terminal to default
   tcsetattr(STDIN, TCSANOW, &init);
   return bytes;
}

static int get_processors(void)
{
    int cpus = 0;
    cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (cpus < 1)
        cpus = 1;
    return (cpus);
}

static int get_cpu_load(STATE *st, int cpu)
{
    static char line[100], *s;
    unsigned long long p_usr = 0, p_nice = 0, p_sys  = 0, p_idle = 0;
    unsigned long long load = 0, idle = 0, util = 0, len;
    unsigned long long p_io = 0, p_irq = 0, p_sirq = 0;
    unsigned long long p_steal = 0, p_guest = 0, p_guest_nice = 0;
    FILE *f;
    char src[100] = "\0";

    if (cpu > st->cpus)
        return 0;

    if (cpu > MAX_WORMS)
        return 0;

    // convert cpu num to ascii text number
    // and null terminate
    snprintf(src, 100, "cpu%d", cpu);
    len = strlen(src);

    if (NULL != (f = fopen("/proc/stat", "r")))
    {
        while (!feof(f) && !load)
	{
            s = fgets(line, 98, f);
            if (s && !strncasecmp(src, line, len))
	    {
		p_usr  = st->usr[cpu];
                p_nice = st->nice[cpu];
                p_sys  = st->sys [cpu];
                p_idle = st->idle[cpu];
                p_io = st->io[cpu];
                p_irq = st->irq[cpu];
                p_sirq = st->sirq[cpu];
                p_steal = st->steal[cpu];
                p_guest = st->guest[cpu];
                p_guest_nice = st->guest_nice[cpu];

                if (sscanf(&line[len + 1], "%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
			   &(st->usr[cpu]), &(st->nice[cpu]),
			   &(st->sys[cpu]), &(st->idle[cpu]),
			   &(st->io[cpu]), &(st->irq[cpu]),
			   &(st->sirq[cpu]), &(st->steal[cpu]),
                           &(st->guest[cpu]), &(st->guest_nice[cpu])) == 10)
		{
		    // calculate total cycles
		    unsigned long long user, nice;
	            unsigned long long idletime;
	            unsigned long long deltaidle;
	            unsigned long long systime;
	            unsigned long long virtalltime;
	            unsigned long long totaltime;
	            unsigned long long deltatime;

		    // Guest time is already accounted in usertime
		    user = st->usr[cpu] - st->guest[cpu];     
		    nice = st->nice[cpu] - st->guest_nice[cpu];
		    // io is added in the total idle time
                    idletime = st->idle[cpu] + st->io[cpu];
                    systime = st->sys[cpu] + st->irq[cpu] + st->sirq[cpu];
                    virtalltime = st->guest[cpu] + st->guest_nice[cpu];
                    totaltime = user + nice + systime + idletime + 
                                st->steal[cpu] + virtalltime;

		    // Guest time is already accounted in usertime
		    user = p_usr - p_guest;     
		    nice = p_nice - p_guest_nice;
		    // io is added in the total idle time
                    deltaidle = p_idle + p_io;
                    systime = p_sys + p_irq + p_sirq;
                    virtalltime = p_guest + p_guest_nice;
                    deltatime = user + nice + systime + deltaidle + 
                                  p_steal + virtalltime;

                    load = totaltime - deltatime;
                    idle = idletime - deltaidle;
		    // prevent divide by zero if result is 0
		    if (!load)
                        load = 1;

		    // subtract idle cycles from load and mulitply * 100
		    // to express as percentage
		    util = (load - idle) * 100 / load;
		    idle = idle * 100 / load;
                    break;
                }
                else
		{
	            fclose(f);
                    return 0;
		}
            }
        }
	fclose(f);
    }

    len = util * util * worm_max_length / 10000.0;
    if (len < WORM_MIN_LEN)
        len = WORM_MIN_LEN;
#if VERBOSE
    set_color(WHITE | BGBLACK);
    mvprintw(cpu, 2, "Load on cpu %d is %d%% length %d idle %d%%\n", cpu, util, len, idle);
    clear_color();
#endif
    return (len);
}

static int get_system_load(void)
{
    char load[100], *s;
    float l1 = 0;
    int l2 = 0;
    FILE *f;

    f = fopen("/proc/loadavg", "r");
    if (f != NULL)
    {
        s = fgets(load, 98, f);
        if (s) {
           sscanf(load, "%f", &l1);
#if VERBOSE
           printw("Load from /proc/loadavg is %f\n", l1);
#endif
        }
	fclose(f);
    }
    else
        l1 = 0;

    // convert from float to integer
    l1 *= 1000.0;
    l2 = (int) l1;
    l2 /= 1000;

    return l2;
}

static void move_worm(STATE *st, WORM *s)
{
    int n = 0, dir = 0;
    int x = 0, y = 0;

    /* worm head position */
    x = s->x[0];
    y = s->y[0];

    /* and direction */
    dir = s->direction;

    /* 0=up, 2=right, 4=down, 6=left */
    switch(dir)
    {
        case 0: y++;      break;
        case 1: y++; x++; break;
        case 2:      x += 2; break;
        case 3: y--; x++; break;
        case 4: y--;      break;
        case 5: y--; x--; break;
        case 6:      x -= 2; break;
        case 7: y++; x--; break;
    }

    /* Check bounds and change direction */
    if (x < 0 && (dir >= 5 && dir <= 7)) {
        x = 1;
        dir -= 4;
    }
    else if (y < 0 && (dir >= 3 && dir <= 5)) {
        y = 1;
        dir -= 4;
    }
    else if (x >= (st->cols - 2) && (dir >= 1 && dir <= 3)) {
        x = st->cols - 2;
        dir += 4;
    }
    else if (y >= st->rows && (dir == 7 || dir == 0 || dir == 1)) {
        y = st->rows - 1;
        dir += 4;
    }
    else if (s->runlength == 0) {
        int rnd;

	rnd = random() % 128;
	if(rnd > 90)
            dir += 2;
	else if(rnd == 1)
            dir++;
        else if(rnd == 2)
            dir--;
        // set this to the current worm length
	s->runlength = s->length;
    }
    else {
        int rnd;

	s->runlength--;
	rnd = random() % 128;
	if(rnd == 1)
            dir++;
        else if(rnd == 2)
            dir--;
    }

    if (dir < 0)
        dir = -dir;
    dir = dir % 8;

    s->direction = dir;

    /* Copy x,y coords in "tail" positions */
    for(n = s->length - 1; n > 0; n--) {
	s->x[n] = s->x[n-1];
        s->y[n] = s->y[n-1];
    }

    /* New head position */
    s->x[0] = x;
    s->y[0] = y;

}

static int grow_worm(STATE *st, WORM *s)
{
    int newlen = get_cpu_load(st, s->cpu);
    int len = s->length;

#if VERBOSE
    printw("grow: cpu %d len %d newlen %d\n", s->cpu, len, newlen);
#endif
    if (newlen > len) {
        int x, y;

        x = s->x[len - 1];
        y = s->y[len - 1];

        switch(s->direction) {
            case 0: y--;      break;
            case 1: y--; x--; break;
            case 2:      x -= 2; break;
            case 3: y++; x--; break;
            case 4: y++;      break;
            case 5: y++; x++; break;
            case 6:      x += 2; break;
            case 7: y--; x++; break;
        }
        len++;

        if (len >= worm_max_length)
            len = worm_max_length - 1;

        s->x[len] = x;
        s->y[len] = y;
    }
    else if (newlen < len) {
        len--;
        if (len < WORM_MIN_LEN)
            len = WORM_MIN_LEN;
        s->x[len + 1] = 0;
        s->y[len + 1] = 0;
    }
    s->length = len;
    return(len);
}

static void clear_worm(STATE *st, WORM *s)
{
    int n;

    for (n = s->length_prev - 1; n >= 0; n--) {
       worm_put_char(' ', s->y_prev[n], s->x_prev[n],
                  WHITE | BGBLACK);
       worm_put_char(' ', s->y_prev[n], s->x_prev[n] + 1,
                  WHITE | BGBLACK);
    }
}

/*

For drawing the worm the following set of equations map the
worm chars to create the effect of the worm moving and expanding.

The logic is non-intuitive but it is described below.  There are
four worm drawing characters in total.  The mapping is defined as:

current char position = n
div = length / 4
mod = length % 4
c = n < (div + 1) * mod ? n / (div + 1) : (n - mod) / div

the above routine produces the following output:

LENGTH    DIV/MOD     WINDOW                CHARACTER MAP
----------------------------------------------------------
length 04 div 1 mod 0 (div + 1) * mod = 00  0 1 2 3
length 05 div 1 mod 1 (div + 1) * mod = 02  0 0 1 2 3
length 06 div 1 mod 2 (div + 1) * mod = 04  0 0 1 1 2 3
length 07 div 1 mod 3 (div + 1) * mod = 06  0 0 1 1 2 2 3
length 08 div 2 mod 0 (div + 1) * mod = 00  0 0 1 1 2 2 3 3
length 09 div 2 mod 1 (div + 1) * mod = 03  0 0 0 1 1 2 2 3 3
length 10 div 2 mod 2 (div + 1) * mod = 06  0 0 0 1 1 1 2 2 3 3
length 11 div 2 mod 3 (div + 1) * mod = 09  0 0 0 1 1 1 2 2 2 3 3
length 12 div 3 mod 0 (div + 1) * mod = 00  0 0 0 1 1 1 2 2 2 3 3 3
length 13 div 3 mod 1 (div + 1) * mod = 04  0 0 0 0 1 1 1 2 2 2 3 3 3
length 14 div 3 mod 2 (div + 1) * mod = 08  0 0 0 0 1 1 1 1 2 2 2 3 3 3
length 15 div 3 mod 3 (div + 1) * mod = 12  0 0 0 0 1 1 1 1 2 2 2 2 3 3 3
length 16 div 4 mod 0 (div + 1) * mod = 00  0 0 0 0 1 1 1 1 2 2 2 2 3 3 3 3
length 17 div 4 mod 1 (div + 1) * mod = 05  0 0 0 0 0 1 1 1 1 2 2 2 2 3 3 3 3
length 18 div 4 mod 2 (div + 1) * mod = 10  0 0 0 0 0 1 1 1 1 1 2 2 2 2 3 3 3 3
length 19 div 4 mod 3 (div + 1) * mod = 15  0 0 0 0 0 1 1 1 1 1 2 2 2 2 2 3 3 3 3
length 20 div 5 mod 0 (div + 1) * mod = 00  0 0 0 0 0 1 1 1 1 1 2 2 2 2 2 3 3 3 3 3
length 21 div 5 mod 1 (div + 1) * mod = 06  0 0 0 0 0 0 1 1 1 1 1 2 2 2 2 2 3 3 3 3 3
length 22 div 5 mod 2 (div + 1) * mod = 12  0 0 0 0 0 0 1 1 1 1 1 1 2 2 2 2 2 3 3 3 3 3
length 23 div 5 mod 3 (div + 1) * mod = 18  0 0 0 0 0 0 1 1 1 1 1 1 2 2 2 2 2 2 3 3 3 3 3
length 24 div 6 mod 0 (div + 1) * mod = 00  0 0 0 0 0 0 1 1 1 1 1 1 2 2 2 2 2 2 3 3 3 3 3 3
length 25 div 6 mod 1 (div + 1) * mod = 07  0 0 0 0 0 0 0 1 1 1 1 1 1 2 2 2 2 2 2 3 3 3 3 3 3
length 26 div 6 mod 2 (div + 1) * mod = 14  0 0 0 0 0 0 0 1 1 1 1 1 1 1 2 2 2 2 2 2 3 3 3 3 3 3
length 27 div 6 mod 3 (div + 1) * mod = 21  0 0 0 0 0 0 0 1 1 1 1 1 1 1 2 2 2 2 2 2 2 3 3 3 3 3 3
length 28 div 7 mod 0 (div + 1) * mod = 00  0 0 0 0 0 0 0 1 1 1 1 1 1 1 2 2 2 2 2 2 2 3 3 3 3 3 3 3
length 29 div 7 mod 1 (div + 1) * mod = 08  0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 2 2 2 2 2 2 2 3 3 3 3 3 3 3
length 30 div 7 mod 2 (div + 1) * mod = 16  0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 3 3 3 3 3 3 3
length 31 div 7 mod 3 (div + 1) * mod = 24  0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 3 3 3 3 3 3 3
length 32 div 8 mod 0 (div + 1) * mod = 00  0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 3 3 3 3 3 3 3 3
length 33 div 8 mod 1 (div + 1) * mod = 09  0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 3 3 3 3 3 3 3 3
length 34 div 8 mod 2 (div + 1) * mod = 18  0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 3 3 3 3 3 3 3 3
length 35 div 8 mod 3 (div + 1) * mod = 27  0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 3 3 3 3 3 3 3 3
length 36 div 9 mod 0 (div + 1) * mod = 00  0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 3 3 3 3 3 3 3 3 3

*/

static void save_worm(WORM *s)
{
    int n;

    // save last worm position and coordinates
    // for clearing later
    for (n = s->length - 1; n >= 0; n--) {
        s->x_prev[n] = s->x[n];
        s->y_prev[n] = s->y[n];
    }
    s->length_prev = s->length;
}

static void draw_worm(STATE *st, WORM *s)
{
    int n, div, mod, c;

    // get character interval and draw worm it is
    // assumed that the minimum worm length is 4
    div = s->length / 4;
    mod = s->length % 4;
    for (n = s->length - 1; n >= 0 && div; n--) {
	   c = n < (div + 1) * mod ? n / (div + 1) : (n - mod) / div;
	   worm_put_char(worm_chars[c % 4],
		    s->y[n], s->x[n], worm_colors[s->cpu % 16]);
	   worm_put_char(worm_chars[c % 4],
		    s->y[n], s->x[n] + 1, worm_colors[s->cpu % 16]);
#if VERBOSE
       printw("cpu %d x[n] = %d y[n] = %d n = %d\n",
	      s->cpu, s->x[n], s->y[n], n);
#endif
    }
#if VERBOSE
    printw("\n");
#endif
}

#define NANOSLEEP

static unsigned long run_worms(STATE *st)
{
    float range, increment;
    int n;

    // reset columns and lines in case the screen was resized
    worm_max_length = AREA_BASE_LEN + AREA_EXT_LEN;
    if (worm_max_length > WORM_MAX_LEN)
       worm_max_length = WORM_MAX_LEN;
    st->cols = COLS;
    st->rows = LINES;

    for (n = 0; n < st->cpus; n++) {
        WORM *s = (WORM *) &st->worms[n];

       if (++s->count >= s->limit) {
           s->count = 0;
           grow_worm(st, s);
           move_worm(st, s);
           clear_worm(st, s);
           s->limit = 4 - (s->length / (worm_max_length / 4));
#if VERBOSE
           printw("length %d limit %d\n", s->length, s->limit);
#endif
	}
        save_worm(s);

	// update all worms even those sleeping to
	// maintain worm overwrite stacking order
	// when one worm overwrites another during
	// display
        draw_worm(st, s);
        refresh();
    }

    // decrease base wait time if system load increases
    // range is 0-100 load average before reaching
    // minimum st->delay wait time
    n = get_system_load();
#ifdef NANOSLEEP
    range = MAX_NANOSEC - MIN_NANOSEC;
    increment = range / MAX_LOADAVG;
    st->delay = MAX_NANOSEC - (n * increment);
    if (st->delay < MIN_NANOSEC)
       st->delay = MIN_NANOSEC;
    st->delay /= st->divisor;
#else
    range = MAX_MICROSEC - MIN_MICROSEC;
    increment = range / MAX_LOADAVG;
    st->delay = MAX_MICROSEC - (n * increment);
    if (st->delay < MIN_MICROSEC)
        st->delay = MIN_MICROSEC;
    st->delay /= st->divisor;
#endif
#if VERBOSE
    printw("delay %d load(n) = %d\n", st->delay, n);
#endif
    return st->delay;
}

#include <sys/time.h>
#include <sys/resource.h>

int cworthy_netware_screensaver(void)
{
    int n, i, prio = 0;
    STATE state, *st = &state;

    memset(st, 0, sizeof(STATE));
    st->cpus = get_processors();
    if (!st->cpus)
       exit(1);

    // set nice value to highest priority
    prio = getpriority(PRIO_PROCESS, 0);
    setpriority(PRIO_PROCESS, 0, -20);

    if (st->cpus > MAX_WORMS)
       st->cpus = MAX_WORMS;
    st->divisor = 1;

    // initialize random number generator
    srand(time(0));

#if VERBOSE
    printw("cols: %d lines: %d base: %d len: %d area: %d"
	   " max: %d min: %d adj: %d divisor: %d\n",
       COLS, LINES, AREA_BASE_LEN, AREA_EXT_LEN, AREA,
       AREA_MAX, AREA_MIN, (AREA) - (AREA_MIN), AREA_DIVISOR);
#endif

    worm_max_length = AREA_BASE_LEN + AREA_EXT_LEN;
    if (worm_max_length > WORM_MAX_LEN)
       worm_max_length = WORM_MAX_LEN;
    st->cols = COLS;
    st->rows = LINES;

#ifdef NANOSLEEP
    st->delay = MAX_NANOSEC / st->divisor;
#else
    st->delay = MAX_MICROSEC / st->divisor;
#endif

    st->worms = (WORM *)calloc(st->cpus, sizeof(WORM));
    if (!st->worms)
       exit(1);
    memset(st->worms, 0, st->cpus * sizeof(WORM));

    for (n = 0; n < st->cpus; n++) {
        WORM *s = (WORM *)&st->worms[n];

	s->cpu  = n;
        s->x[0] = random() % (COLS - 1);
	s->y[0] = random() % LINES;
	for (i=1; i < WORM_MAX_LEN; i++)
	{
           s->x[i] = s->x[0];
           s->y[i] = s->y[0];
	}
	s->direction = ((random() % 9) >> 1) << 1;
	s->length = WORM_MIN_LEN;
        s->runlength = WORM_MIN_LEN;
#if VERBOSE
	printw(stderr, "worm %d starting at %d,%d dir %d length %d\n",
	       s->cpu, s->x[0], s->y[0], s->direction, s->length);
#endif
    }

    while (!worm_kbhit())
    {
#ifdef NANOSLEEP
       struct timespec ts = { 0, st->delay };
#endif
       run_worms(st);
#ifdef NANOSLEEP
       nanosleep(&ts, NULL);
#else
       usleep(st->delay);
#endif
    }

    if (st->worms)
       free(st->worms);

    setpriority(PRIO_PROCESS, 0, prio);
    return 0;
}

