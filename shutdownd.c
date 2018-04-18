/*
 * Copyright (c) 2012, 2013 joshua stein <jcs@jcs.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/types.h>
#include <machine/apmvar.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#define	DEFAULT_WARN_MINS	25
#define	DEFAULT_WARN_CMD	"yad " \
				"--image=gnome-shutdown " \
				"--fixed " \
				"--button gtk-ok:0 " \
				"--text \"<b>Critical Battery Status</b>\\n" \
				"The system battery currently has " \
				"<b>$battery_minutes</b> remaining \\n" \
				"and will automatically shutdown in " \
				"<b>$shutdown_minutes</b>.\""

#define	DEFAULT_SHUTDOWN_MINS	5
#define	DEFAULT_SHUTDOWN_CMD	"/usr/bin/doas /sbin/halt -p"

extern char *__progname;

void usage(void);
void yell(char *, ...);
void expand_var(char *, char *, int, char *, char *);
void expand_warn_cmd(char *, char *, int, int, struct apm_power_info *);
void execute(char *);

int verbose = 0;
int warned = 0;

void
usage()
{
	fprintf(stderr, "usage: %s [-v] [-s shutdown mins] [-S shutdown cmd] "
	    "[-w warn mins] [-W warn cmd]\n", __progname);
	exit(1);
}

void
yell(char *fmt, ...)
{
	char *s;
	va_list ap;

	if ((s = malloc(512)) == NULL)
		errx(1, "malloc");

	va_start(ap, fmt);
	(void)vsnprintf(s, 512, fmt, ap);
	va_end(ap);

	if (verbose) {
		write(STDERR_FILENO, s, strlen(s));
		write(STDERR_FILENO, "\n", 1);
	}
	else
		syslog(LOG_ALERT, "%s", s);
}

/* dst = src.gsub(var, data) */
void
expand_var(char *src, char *dst, int dstlen, char *var, char *data)
{
	int srclen = strlen(src);
	int varlen = strlen(var);
	int x;

	bzero(dst, dstlen);

	for (x = 0; x < srclen; x++) {
		if (srclen - x > varlen &&
		    memcmp(src + x, var, varlen) == 0) {
			/* found var at x, append data instead */
			strlcat(dst, data, dstlen);
			x += varlen - 1;
		} else {
			int s = strlen(dst);
			dst[s] = src[x];
			dst[s + 1] = '\0';
		}
	}
}

void
expand_warn_cmd(char *warn_cmd_tmpl, char *warn_cmd, int warn_cmd_len,
    int shutdown_mins, struct apm_power_info *apm_info)
{
	char mins[20];
	char tmpl[warn_cmd_len];

	snprintf(mins, sizeof(mins), "%d minute%s", apm_info->minutes_left,
		(apm_info->minutes_left == 1 ? "" : "s"));
	expand_var(warn_cmd_tmpl, warn_cmd, warn_cmd_len, "$battery_minutes",
	    mins);

	/* swap current buffer as template */
	strncpy(tmpl, warn_cmd, warn_cmd_len);
	bzero(warn_cmd, warn_cmd_len);

	snprintf(mins, sizeof(mins), "%d minute%s",
	    apm_info->minutes_left - shutdown_mins,
	    (apm_info->minutes_left - shutdown_mins == 1 ? "" : "s"));
	expand_var(tmpl, warn_cmd, warn_cmd_len, "$shutdown_minutes", mins);
}

void
execute(char *command)
{
	char *argp[] = { "sh", "-c", command, NULL };

	switch (fork()) {
	case -1:
		yell("fork of %s failed", command);
		break;
	case 0:
		execv("/bin/sh", argp);
		_exit(1);
		/* NOTREACHED */
	}
}

int
main(int argc, char *argv[])
{
	struct apm_power_info apm_info;
	int apm_fd;
	int ch;
	int shutdown_mins = DEFAULT_SHUTDOWN_MINS;
	int warn_mins = DEFAULT_WARN_MINS;
	char *shutdown_cmd = DEFAULT_SHUTDOWN_CMD;
	char *warn_cmd_tmpl = DEFAULT_WARN_CMD;
	char warn_cmd[PATH_MAX];
	char *p;

	while ((ch = getopt(argc, argv, "s:S:vw:W:")) != -1) {
		switch (ch) {
		case 's':
			shutdown_mins = strtol(optarg, &p, 10);
			if (*p || shutdown_mins < 1)
				errx(1, "illegal -s value: %s", optarg);
				/* NOTREACHED */
			break;
		case 'S':
			shutdown_cmd = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'w':
			warn_mins = strtol(optarg, &p, 10);
			if (*p || warn_mins < 1)
				errx(1, "illegal -w value: %s", optarg);
				/* NOTREACHED */
			break;
		case 'W':
			warn_cmd_tmpl = optarg;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}

	apm_fd = open("/dev/apm", O_RDONLY);
	if (apm_fd < 0)
		errx(1, "can't open /dev/apm");

	if (!verbose)
		daemon(0, 0);

	for (;;) {
		if (ioctl(apm_fd, APM_IOC_GETPOWER, &apm_info) < 0) {
			yell("can't read apm power");
			exit(1);
		}

		if (apm_info.ac_state == APM_AC_ON)
			goto sleeper;

		if (verbose)
			printf("low battery (%d%%), off ac, %d minute%s "
			    "remaining\n",
			    apm_info.battery_life,
			    apm_info.minutes_left,
			    (apm_info.minutes_left == 1 ? "" : "s"));

		if (apm_info.minutes_left <= warn_mins && !warned) {
			expand_warn_cmd(warn_cmd_tmpl, warn_cmd, PATH_MAX,
			    shutdown_mins, &apm_info);

			yell("minutes remaining below %d, running warn "
			    "command: %s", warn_mins, warn_cmd);

			warned = 1;
			execute(warn_cmd);
		}
		else if (apm_info.minutes_left <= shutdown_mins) {
			yell("minutes remaining below %d, running shutdown "
			    "command: %s\n", shutdown_mins, shutdown_cmd);

			execute(shutdown_cmd);
			exit(0);
		}
		else if (apm_info.minutes_left >= warn_mins + 10)
			warned = 0;

sleeper:
		sleep(5);
	}

	return (0);
}
