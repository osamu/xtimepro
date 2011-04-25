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
#define STAT_FILE "/tmp/.xtimepro"

static int      debug = 0;

struct xtimepro_config {
	struct tm      *chktime_tm;
	time_t          chktime_time;
	char           *output_file;
	char           *stat_file;
};

struct xtimepro_stat {
	int             prev_time, prev_idle, total_idle;
};


struct xtimepro_config *
xtimepro_default_config()
{
	struct xtimepro_config *tc;
	time_t          now = time(NULL);

	tc = (struct xtimepro_config *) calloc(1, sizeof(struct xtimepro_config));
	tc->chktime_tm = localtime(&now);
	tc->chktime_tm->tm_hour = CHECKTIME_HOUR;
	tc->chktime_tm->tm_min = CHECKTIME_MIN;
	tc->chktime_tm->tm_sec = 0;
	tc->chktime_time = mktime(tc->chktime_tm);
	tc->output_file = OUTPUT_FILE;
	tc->stat_file = STAT_FILE;

	return tc;
}

struct xtimepro_stat *
xtimepro_read_stat(char *filename, struct xtimepro_stat *stat)
{
	FILE           *stat_fp = NULL;

	if ((stat_fp = fopen(filename, "r")) == NULL) {
		fprintf(stderr, "Can't open stat file : %s\n", filename);
		fprintf(stderr, "xtimepro initialize with %s\n");

	} else {
		// FIXME: check error;
		// line = (char *)malloc(sizeof(char) * nbytes);
		fscanf(stat_fp, "%d %d %d", &stat->prev_time, &stat->prev_idle,
		       &stat->total_idle);

		if (debug)
			printf("DEBUG: prev_time:%d , prev_idle:%d, total_idle:%d\n",
			       stat->prev_time, stat->prev_idle, stat->total_idle);

		fclose(stat_fp);
	}
	return stat;
}
int
xtimepro_write_stat(char *filename, int last_time, int this_idle, int total_idle)
{
	FILE           *stat_fp = NULL;

	if ((stat_fp = fopen(filename, "w")) == NULL) {
		fprintf(stderr, "Error: Can't open stat file : %s\n", filename);
		return -1;
	}
	fprintf(stat_fp, "%d %d %d\n", last_time, this_idle, total_idle);

	if (debug)
		printf("DEBUG: last = %s", ctime((time_t *) (&last_time)));

	fclose(stat_fp);
	return 0;
}

int
parse_option(struct xtimepro_config *config, int argc, char *argv[])
{
	int             opt;
	int             hour = 0, min = 0;

	while ((opt = getopt(argc, argv, "c:o:s:id")) != -1) {
		switch (opt) {
		case 'c':
			sscanf(optarg, "%d:%d", &hour, &min);
			config->chktime_tm->tm_hour = hour;
			config->chktime_tm->tm_min = min;
			config->chktime_time = mktime(config->chktime_tm);
			break;
		case 'o':
			config->output_file = strdup(optarg);
			break;
		case 's':
			config->stat_file = strdup(optarg);
			break;
		case 'd':
			debug = 1;
			break;
		case 'i':
		default:
			fprintf(stderr,
				"usage: %s [-c time] [-o output_file] [-s status_file] [-d] \n",
				argv[0]);
			fprintf(stderr, "Options\n");
			fprintf(stderr,
				" -c time    : time period for attendance at day (default 00:00)\n");
			fprintf(stderr,
				" -o file    : file path of attendance result\n");
			fprintf(stderr, " -s file    : file path of status\n");
			fprintf(stderr, " -d         : debug mode\n");
			exit(0);
		}
	}

	if (debug) {
		printf("DEBUG: attendance result  file is %s\n",
		       config->output_file);
		printf("DEBUG: status file is %s\n", config->stat_file);
	}


	return 0;
}

time_t
get_idle_time()
{
	time_t          idle_time;
	static XScreenSaverInfo *mit_info;
	Display        *display;
	int             screen;
	mit_info = XScreenSaverAllocInfo();
	if ((display = XOpenDisplay(NULL)) == NULL) {
		return (-1);
	}
	screen = DefaultScreen(display);
	XScreenSaverQueryInfo(display, RootWindow(display, screen), mit_info);
	idle_time = (mit_info->idle) / 1000;
	XFree(mit_info);
	XCloseDisplay(display);
	return idle_time;
}


int
main(int argc, char *argv[])
{
	struct xtimepro_config *config;
	struct xtimepro_stat *stat;
	FILE           *fp;
	time_t          now = 0, last = 0;
	time_t          this_idle = 0;

	config = xtimepro_default_config();
	stat = (struct xtimepro_stat *) malloc(sizeof(struct xtimepro_stat));

	parse_option(config, argc, argv);

	// preper
	now = time(NULL);
	this_idle = get_idle_time();
	if (debug)
		printf("DEBUG: checktime : %s", ctime(&(config->chktime_time)));

	// read status file
	stat = xtimepro_read_stat(config->stat_file, stat);

	// idle time handling
	if (this_idle > stat->prev_idle) {	// continues to be idle 
		last = stat->prev_time;

	} else {		// there was something event 

		if (stat->prev_time == -1) {	// this is first event since
                          			// checktime.
			fp = fopen(config->output_file, "a+");
			fprintf(fp, "in : %s", ctime(&now));
			if (debug)
				printf("in : %s", ctime(&now));

			fclose(fp);
		}
		last = now;
		stat->total_idle += stat->prev_idle;
	}

	// checktime processing 
	if ((last != -1)	// over the checktime
	    && (last < config->chktime_time) && (config->chktime_time < now)) {
		fp = fopen(config->output_file, "a+");
		fprintf(fp, "out : %s", ctime(&last));
		if (debug)
			printf("out : %s", ctime(&last));
		last = -1;	// UNTILL FIRST EVENT 
		stat->total_idle = 0;
		fclose(fp);
	}
	// write status 
	xtimepro_write_stat(config->stat_file, last, this_idle, stat->total_idle);
	
	return 0;
}
/*
Local Variables:
mode: c
tab-width: 8
End:
*/
