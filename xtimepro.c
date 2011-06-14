/*

  xtimepro - Manage attendance of youself at X11 environment.

*/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/scrnsaver.h>

#define CHECKTIME_HOUR 0
#define CHECKTIME_MIN 0
#define OUTPUT_FILE "/tmp/xtimepro-log.txt"
#define STAT_FILE   "/tmp/.xtimepro"
#define DEBUG_FILE  "/tmp/xtimepro.debug"

const char *program_string = "xtimepro";
const char *version_string = "0.9";
static int debug = 0;
time_t x11_get_idle_time (void);

struct xtimepro_config
{
  struct tm *chktime_tm;
  time_t chktime_time;
  char *output_file;
  char *stat_file;
  char *debug_file;
};

struct xtimepro_stat
{
  int prev_time, prev_idle, total_idle;
};

struct xtimepro
{
  FILE *fp, *debugfp;
  time_t t_now;
  time_t t_prev;
  time_t t_idle;
  char timestr[128];
  struct xtimepro_stat *stat;

};

struct xtimepro *
xtimepro_init (struct xtimepro_config *config)
{
  struct xtimepro *tp;

  if ((tp = (struct xtimepro *) malloc (sizeof (struct xtimepro))) == NULL)
    {
      fprintf (stderr, "Initialize xtimepro failed\n");
      exit (1);
    }

  if (debug)
    tp->debugfp = fopen (config->debug_file, "a+");

  tp->stat = (struct xtimepro_stat *) malloc (sizeof (struct xtimepro_stat));
  tp->t_now = time (NULL);
  strftime (tp->timestr, 1024, "%b %d %T", localtime (&tp->t_now));
  tp->t_idle = x11_get_idle_time ();

  return tp;
}

void
xtimepro_finish (struct xtimepro **tp)
{

  if (debug)
    fclose ((*tp)->debugfp);

  if ((*tp)->stat != NULL)
    free ((*tp)->stat);

  free (*tp);
}


struct xtimepro_config *
xtimepro_default_config ()
{
  struct xtimepro_config *tc;
  time_t now = time (NULL);

  tc = (struct xtimepro_config *) calloc (1, sizeof (struct xtimepro_config));
  tc->chktime_tm = localtime (&now);
  tc->chktime_tm->tm_hour = CHECKTIME_HOUR;
  tc->chktime_tm->tm_min = CHECKTIME_MIN;
  tc->chktime_tm->tm_sec = 0;
  tc->chktime_time = mktime (tc->chktime_tm);
  tc->output_file = OUTPUT_FILE;
  tc->stat_file = STAT_FILE;
  tc->debug_file = DEBUG_FILE;

  return tc;
}

int
xtimepro_read_stat (char *filename, struct xtimepro_stat *stat)
{
  FILE *stat_fp = NULL;

  if ((stat_fp = fopen (filename, "r")) == NULL)
    {
      return -1;
    }

  // FIXME: check error;
  // line = (char *)malloc(sizeof(char) * nbytes);
  fscanf (stat_fp, "%d %d %d", &stat->prev_time, &stat->prev_idle,
	  &stat->total_idle);

  if (debug)
    printf ("DEBUG: prev_time:%d , prev_idle:%d, total_idle:%d\n",
	    stat->prev_time, stat->prev_idle, stat->total_idle);

  fclose (stat_fp);
  return 0;
}

int
xtimepro_write_stat (char *filename, int last_time, int this_idle,
		     int total_idle)
{
  FILE *stat_fp = NULL;

  if ((stat_fp = fopen (filename, "w")) == NULL)
    {
      fprintf (stderr, "Error: Can't open stat file : %s\n", filename);
      return -1;
    }

  fprintf (stat_fp, "%d %d %d\n", last_time, this_idle, total_idle);

  if (debug)
    printf ("DEBUG: last = %s", ctime ((time_t *) (&last_time)));

  fclose (stat_fp);
  return 0;
}

int
parse_option (struct xtimepro_config *config, int argc, char *argv[])
{
  int opt;
  int hour = 0, min = 0;

  while ((opt = getopt (argc, argv, "c:o:s:id")) != -1)
    {
      switch (opt)
	{
	case 'c':
	  sscanf (optarg, "%d:%d", &hour, &min);
	  config->chktime_tm->tm_hour = hour;
	  config->chktime_tm->tm_min = min;
	  config->chktime_time = mktime (config->chktime_tm);
	  break;
	case 'o':
	  config->output_file = strdup (optarg);
	  break;
	case 's':
	  config->stat_file = strdup (optarg);
	  break;
	case 'd':
	  debug = 1;
	  break;
	case 'i':
	default:
	  fprintf (stderr,
		   "usage: %s [-c time] [-o output_file] [-s status_file] [-d] \n",
		   argv[0]);
	  fprintf (stderr, "Options\n");
	  fprintf (stderr,
		   " -c time    : time period for attendance at day (default 00:00)\n");
	  fprintf (stderr, " -o file    : file path of attendance result\n");
	  fprintf (stderr, " -s file    : file path of status\n");
	  fprintf (stderr, " -d         : debug mode\n");
	  exit (0);
	}
    }

  if (debug)
    {
      printf ("DEBUG: attendance result file is %s\n", config->output_file);
      printf ("DEBUG: debug file is %s\n", config->debug_file);
      printf ("DEBUG: status file is %s\n", config->stat_file);
    }


  return 0;
}

time_t
x11_get_idle_time ()
{
  time_t idle_time;
  static XScreenSaverInfo *mit_info;
  Display *display;
  int screen;
  mit_info = XScreenSaverAllocInfo ();
  if ((display = XOpenDisplay (NULL)) == NULL)
    {
      return (-1);
    }
  screen = DefaultScreen (display);
  XScreenSaverQueryInfo (display, RootWindow (display, screen), mit_info);
  idle_time = (mit_info->idle) / 1000;
  XFree (mit_info);
  XCloseDisplay (display);
  return idle_time;
}


int
main (int argc, char *argv[])
{
  struct xtimepro_config *config;
  struct xtimepro *tp;

  config = xtimepro_default_config ();
  parse_option (config, argc, argv);

  tp = xtimepro_init (config);

  if (debug)
    printf ("DEBUG: checktime : %s", ctime (&(config->chktime_time)));

  // read status file
  if (xtimepro_read_stat (config->stat_file, tp->stat) < 0)
    {
      fprintf (stderr, "Initialize %s\n", config->stat_file);
      xtimepro_write_stat (config->stat_file, -1, tp->t_idle, 0);
      xtimepro_finish (&tp);
      exit (1);
    }

  // idle time handling
  if (tp->t_idle > tp->stat->prev_idle)
    {				// continues to be idle 
      tp->t_prev = tp->stat->prev_time;

      if (debug)
	{
	  fprintf (tp->debugfp, "%s : idle continue idle=%d, last=%d\n",
		   tp->timestr, tp->t_idle, tp->t_prev);
	}

    }
  else
    {				// there was something event 

      if (tp->stat->prev_time == -1)
	{			// this is first event since
	  // checktime.
	  FILE *fp = fopen (config->output_file, "a+");
	  fprintf (fp, "in : %s", ctime (&(tp->t_now)));

	  if (debug)
	    fprintf (tp->debugfp, "%s : checkin\n", tp->timestr);

	  fclose (fp);
	}
      tp->t_prev = tp->t_now;
      tp->stat->total_idle += tp->stat->prev_idle;

      if (debug)
	{
	  fprintf (tp->debugfp, "%s : event occour idle=%d, last=%d\n",
		   tp->timestr, tp->t_idle, tp->t_prev);

	}
    }

  // checktime processing 
  if ((tp->t_prev != -1)	// over the checktime
      && (tp->t_prev < config->chktime_time)
      && (config->chktime_time < tp->t_now))
    {
      FILE *fp = fopen (config->output_file, "a+");
      fprintf (fp, "out : %s", ctime (&(tp->t_prev)));

      if (debug)
	{
	  fprintf (tp->debugfp, "%s : checkout\n", tp->timestr);
	}

      tp->t_prev = -1;		// UNTILL FIRST EVENT 
      tp->stat->total_idle = 0;
      fclose (fp);
    }

  // write status 
  xtimepro_write_stat (config->stat_file, tp->t_prev, tp->t_idle,
		       tp->stat->total_idle);
  xtimepro_finish (&tp);

  return 0;
}

/*
  Local Variables:
  mode: c
  tab-width: 8
  End:
*/
