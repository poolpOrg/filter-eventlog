/*
 * Copyright (c) 2019 Gilles Chehade <gilles@poolp.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define	TIMEFRAME	86400

static time_t curr_timeframe;

extern char *__progname;

int
main(int argc, char *argv[])
{
	char *line = NULL;
	size_t linesize = 0;
	ssize_t linelen;
	FILE *fp = NULL;
	char pathname[PATH_MAX];
	time_t ctime;
	time_t timeframe;
	char *ep;

	if (argc != 2)
		errx(1, "usage: %s /path/to/directory", __progname);

	printf("register|report|smtp-in|*\n");
	printf("register|ready\n");
	fflush(stdout);

	while ((linelen = getline(&line, &linesize, stdin)) != -1) {
		if (strncmp("report|1|", line, 9) != 0)
			errx(1, "received bogus report event");

		errno = 0;
		ctime = strtoul(line+9, &ep, 10);
		if (line[9] == '\0' || *ep != '.')
			errx(1, "received bogus report event");
		if (errno == ERANGE)
			errx(1, "received bogus report event");
		
		timeframe = ctime - (ctime % TIMEFRAME);
		if (curr_timeframe == 0 || curr_timeframe != timeframe) {
			if (fp)
				fclose(fp);
			else {
				(void)snprintf(pathname, sizeof pathname, "%s/%lld",
				    argv[1], timeframe);
				if ((fp = fopen(pathname, "a+")) == NULL)
					err(1, "fopen");
				curr_timeframe = timeframe;
			}
		}

		fwrite(line, linelen, 1, fp);
		fflush(fp);
	}
	free(line);
	if (ferror(stdout))
		err(1, "getline");

	return 0;
}
