#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

enum {
	GETOPT_OUTPUT = 0x100,
	GETOPT_API,
	GETOPT_START,
	GETOPT_CALLBACK,
	GETOPT_END,
};

struct option long_options[] = {    
	{ "output",     required_argument,  0,  GETOPT_OUTPUT },
	{ "api",        required_argument, 	0, 	GETOPT_API },
	{ "start",      required_argument,  0,  GETOPT_START },
	{ "callback",   required_argument,  0,  GETOPT_CALLBACK },
	{ "end",        required_argument,  0,  GETOPT_END },
	{ 0,            0,                  0,  0 }
};

static const char file_version[] = "1.0.0";

long current_time_milliseconds();

int main(int argc, char **argv) {
	int lopt_idx = -1;
	int c;
	char *api = NULL;
	char *start = NULL;
	char *end = NULL;
	char *callback = NULL;
	char *outputFile = NULL;

	while((c = getopt_long(argc, argv, "", long_options, &lopt_idx)) != EOF) {
		switch (c) {
			case GETOPT_OUTPUT:
				outputFile = optarg;
				break;
			case GETOPT_API:
				api = optarg;
			case GETOPT_START:
				start = optarg;
				break;
			case GETOPT_CALLBACK:
				callback = optarg;
				break;
			case GETOPT_END:
				end = optarg;
				break;
		}
	}

	if (outputFile == NULL) {
		return 1;
	}

	if (!api && !start && !callback && !end) {
		return 2;
	}

	if (api) {
		FILE *fp = fopen(outputFile, "w");

		if (fp == NULL) {
			return 3;
		}

		fprintf(fp, "{\n");
		fprintf(fp, "\"file_version\": \"%s\",\n", file_version);
		fprintf(fp, "\"api\": %s\n", api);
		fprintf(fp, "}");
		fclose(fp);
		return 0;
	}

	FILE *fp = fopen(outputFile, "r+");

	if (fp == NULL) {
		return 3;
	}

	if (start) {
		static const char end_check[] = "}\n}";

		fseek(fp, -strlen(end_check), SEEK_END);
		char buffer[strlen(end_check)];
		fread(buffer, sizeof(char), strlen(end_check), fp);

		if (strncmp(buffer, "}\n}", strlen(end_check)) != 0) {
			fprintf(stderr, "Output file is not as expected.");
			fclose(fp);
			return 4;
		}

		fseek(fp, -2, SEEK_END);
		fprintf(fp, ",\n");
		fprintf(fp, "\"start_time\": %lld,\n", current_time_milliseconds());
		fprintf(fp, "\"start\": %s\n", start);
		fprintf(fp, "}");
		fclose(fp);
		return 0;
	}

	if (callback) {
		static const char end_check1[] = "}\n}";
		static const char end_check2[] = "\n]\n}";

		const int max_check_size = strlen(end_check1) > strlen(end_check2) ? strlen(end_check1) : strlen(end_check2);

		fseek(fp, -max_check_size, SEEK_END);
		char buffer[max_check_size];
		fread(buffer, sizeof(char), max_check_size, fp);
		bool write_callbacks_line = false;

		if (strncmp(&buffer[max_check_size - strlen(end_check1)], end_check1, strlen(end_check1)) == 0) {
			fseek(fp, -2, SEEK_END);
			write_callbacks_line = true;
		} else if (strncmp(&buffer[max_check_size - strlen(end_check2)], end_check2, strlen(end_check2)) == 0) {
			fseek(fp, -4, SEEK_END);
		} else {
			fprintf(stderr, "Output file is not as expected.");
			fclose(fp);
			return 4;
		}

		fprintf(fp, ",\n");
		if (write_callbacks_line) {
			fprintf(fp, "\"callbacks\": [\n");
		}
		fprintf(fp, "%s\n", callback);
		fprintf(fp, "]\n");
		fprintf(fp, "}");
		fclose(fp);
		return 0;
	}

	if (end) {
		static const char end_check[] = "\n}";

		fseek(fp, -strlen(end_check), SEEK_END);
		char buffer[strlen(end_check)];
		fread(buffer, sizeof(char), strlen(end_check), fp);

		if (strncmp(buffer, end_check, strlen(end_check)) != 0) {
			fprintf(stderr, "Output file is not as expected.");
			fclose(fp);
			return 4;
		}

		fseek(fp, -2, SEEK_END);
		fprintf(fp, ",\n");
		fprintf(fp, "\"end_time\": %lld,\n", current_time_milliseconds());
		fprintf(fp, "\"end\": %s\n", end);
		fprintf(fp, "}");
		fclose(fp);
		return 0;
	}

	fclose(fp);
	return 2;
}

long current_time_milliseconds() {
	struct timeval tp;

	gettimeofday(&tp, NULL);
	return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}