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
#include <unistd.h>

#define	TIMEFRAME	86400

static time_t curr_timeframe;

extern char *__progname;
extern int optind;
extern char *optarg;

static void
print_raw(FILE *fp, char *line, size_t linelen)
{
	fwrite(line, linelen, 1, fp);
}

static void
print_kv(FILE *fp, char *line, size_t linelen)
{
	char *save = line;
	char *timestamp;
	char *subsystem;
	char *event;
	char *session;

	if (strncmp(line, "report|0|", 9) != 0)
		return;
	line[strcspn(line, "\n")] = '\0';

	
	line += 9;
	linelen -= 9;

	timestamp = line;
	line[strcspn(line, "|")] = '\0';
	line += strlen(line) + 1;

	subsystem = line;
	line[strcspn(line, "|")] = '\0';
	line += strlen(line) + 1;

	event = line;
	line[strcspn(line, "|")] = '\0';
	line += strlen(line) + 1;

	session = line;
	line[strcspn(line, "|")] = '\0';
	line += strlen(line) + 1;

	fprintf(fp, "timestamp=%s ", timestamp);
	fprintf(fp, "subsystem=%s ", subsystem);
	fprintf(fp, "event=%s ", event);
	fprintf(fp, "session=%s ", session);

	if (strcmp(event, "link-connect") == 0) {
		char *rdns;
		char *fcrdns;
		char *src_addr;
		char *src_port;
		char *dst_addr;
		char *dst_port;

		rdns = line;
		line[strcspn(line, "|")] = '\0';
		line += strlen(line) + 1;

		fcrdns = line;
		line[strcspn(line, "|")] = '\0';
		line += strlen(line) + 1;

		src_addr = line;
		line[strcspn(line, ":")] = '\0';
		line += strlen(line) + 1;

		src_port = line;
		line[strcspn(line, "|")] = '\0';
		line += strlen(line) + 1;

		dst_addr = line;
		line[strcspn(line, ":")] = '\0';
		line += strlen(line) + 1;

		dst_port = line;

		fprintf(fp, "rdns=%s fcrdns=%s src_addr=%s dst_addr=%s",
		    rdns, fcrdns, src_addr, dst_addr);

		if (*dst_port != '/') {
			fprintf(fp, " src_port=%s dst_port=%s",
			    src_port, dst_port);
		} else {
			fprintf(fp, " src_port=\"");
			while (*src_port) {
				if (*src_port == '"')
					fprintf(fp, "\\");
				fprintf(fp, "%c", *src_port);
				src_port++;
			}
			fprintf(fp, "\"");

			fprintf(fp, " dst_port=\"");
			while (*dst_port) {
				if (*dst_port == '"')
					fprintf(fp, "\\");
				fprintf(fp, "%c", *dst_port);
				dst_port++;
			}
			fprintf(fp, "\"");
		}
	}
	else if (strcmp(event, "link-identify") == 0)
		fprintf(fp, "helo-name=%s", line);
	else if (strcmp(event, "link-tls") == 0)
		fprintf(fp, "tls=%s", line);
	else if (strcmp(event, "link-disconnect") == 0)
		/* no params */;

	else if (strcmp(event, "tx-begin") == 0) {
		fprintf(fp, "msgid=%s", line);
	}
	else if (strcmp(event, "tx-mail") == 0 ||
	    strcmp(event, "tx-rcpt") == 0) {
		char *msgid;
		char *address;
		char *status;

		msgid = line;
		line[strcspn(line, "|")] = '\0';
		line += strlen(line) + 1;

		address = line;
		line[strcspn(line, "|")] = '\0';
		line += strlen(line) + 1;

		status = line;

		fprintf(fp, "msgid=%s address=%s status=%s",
		    msgid, address, status);
	}

	else if (strcmp(event, "tx-envelope") == 0) {
		char *msgid;
		char *evpid;

		msgid = line;
		line[strcspn(line, "|")] = '\0';
		line += strlen(line) + 1;

		evpid = line;

		fprintf(fp, "msgid=%s evpid=%s", msgid, evpid);
	}
	else if (strcmp(event, "tx-data") == 0) {
		char *msgid;
		char *status;

		msgid = line;
		line[strcspn(line, "|")] = '\0';
		line += strlen(line) + 1;

		status = line;

		fprintf(fp, "msgid=%s status=%s", msgid, status);
	}
	else if (strcmp(event, "tx-commit") == 0) {
		char *msgid;
		char *size;

		msgid = line;
		line[strcspn(line, "|")] = '\0';
		line += strlen(line) + 1;

		size = line;

		fprintf(fp, "msgid=%s size=%s", msgid, size);
	}
	else if (strcmp(event, "tx-rollback") == 0) {
		char *msgid;

		msgid = line;

		fprintf(fp, "msgid=%s", msgid);
	}


	else if (strcmp(event, "protocol-client") == 0 ||
	    strcmp(event, "protocol-server") == 0) {
		fprintf(fp, "line=\"");
		while (*line) {
			if (*line == '"')
				fprintf(fp, "\\");
			fprintf(fp, "%c", *line);
			line++;
		}
		fprintf(fp, "\"");
	}

	else if (strcmp(event, "filter-response") == 0) {
		char *phase;
		char *response;
		char *param = NULL;

		phase = line;
		line[strcspn(line, "|")] = '\0';
		line += strlen(line) + 1;

		response = line;
		line[strcspn(line, "|")] = '\0';
		line += strlen(line) + 1;

		fprintf(fp, "phase=%s response=%s", phase, response);

		if (line < save + linelen) {
			param = line;
			fprintf(fp, "param=%s", param);
		}
	}
	else if (strcmp(event, "timeout") == 0)
		/* no params*/;

	else {
		fprintf(fp, "line: %s", line);
	}

	fprintf(fp, "\n");

}

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
	int ch;
	void (*print)(FILE *fp, char *line, size_t linelen) = print_raw;

	while ((ch = getopt(argc, argv, "t:")) != -1) {
		switch (ch) {
		case 't':
			if (strcmp(optarg, "raw") == 0)
				print = print_raw;
			else if (strcmp(optarg, "kv") == 0)
				print = print_kv;
			else
				errx(1, "unsupported type: %s", optarg);
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		errx(1, "usage: %s /path/to/directory", __progname);

	while ((linelen = getline(&line, &linesize, stdin)) != -1) {
		line[strcspn(line, "\n")] = '\0';
		if (strcmp("config|ready", line) == 0)
			break;
	}

	printf("register|report|smtp-in|*\n");
	printf("register|report|smtp-out|*\n");
	printf("register|ready\n");
	fflush(stdout);

	while ((linelen = getline(&line, &linesize, stdin)) != -1) {
		if (strncmp("report|0|", line, 9) != 0)
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
			(void)snprintf(pathname, sizeof pathname, "%s/%lld",
			    argv[0], timeframe);
			if ((fp = fopen(pathname, "a+")) == NULL)
				err(1, "fopen: %s", pathname);
			curr_timeframe = timeframe;
		}

		print(fp, line, linelen);

		fflush(fp);
	}
	free(line);
	if (ferror(stdout))
		err(1, "getline");

	return 0;
}
