/**
 * ttyqr - QR Code generator for text console
 * Copyright (C) 2012 Ondrej Caletka <ondrej@caletka.cz>
 *
 * based on 
 * QR Code encoding tool, licensed under LGPL 2.0
 * Copyright (C) 2006, 2007, 2008, 2009 Kentaro Fukuchi <fukuchi@megaui.net>
 *
 * Printing QR code on terminal borrowed from Google's Authenticator
 * http://code.google.com/p/google-authenticator/source/browse/libpam/google-authenticator.c
 * Copyright 2010 Google Inc.
 * Author: Markus Gutschke
 * Licensed under the Apache License, Version 2.0
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include <qrencode.h>
//#include "config.h"
#define VERSION "0.1"

static int casesensitive = 1;
static int eightbit = 0;
static int version = 0;
static int structured = 0;
static QRecLevel level = QR_ECLEVEL_L;
static QRencodeMode hint = QR_MODE_8;
static int useBlockElements = 0;

static const struct option options[] = {
	{"help"         , no_argument      , NULL, 'h'},
	{"output"       , required_argument, NULL, 'o'},
	{"level"        , required_argument, NULL, 'l'},
	{"symversion"   , required_argument, NULL, 'v'},
	{"structured"   , no_argument      , NULL, 'S'},
	{"kanji"        , no_argument      , NULL, 'k'},
	{"casesensitive", no_argument      , NULL, 'c'},
	{"ignorecase"   , no_argument      , NULL, 'i'},
	{"8bit"         , no_argument      , NULL, '8'},
	{"version"      , no_argument      , NULL, 'V'},
	{"block"        , no_argument      , NULL, 'b'},
	{NULL, 0, NULL, 0}
};

static char *optstring = "ho:l:v:Skci8Vb";

static void usage(int help, int longopt)
{
	fprintf(stderr,
"ttyqr version %s\n"
"Copyright (C) 2012 Ondrej Caletka\n", VERSION);
	if(help) {
		if(longopt) {
			fprintf(stderr,
"Usage: ttyqr [OPTION]... [STRING]\n"
"Encode input data in a QR Code and display it on terminal.\n\n"
"  -h, --help   display the help message. -h displays only the help of short\n"
"               options.\n\n"
"  -o FILENAME, --output=FILENAME\n"
"               write output to FILENAME. If '-' is specified, the result\n"
"               will be output to standard output (default). If -S is given, structured\n"
"               symbols are written to FILENAME-01.txt, FILENAME-02.txt, ...;\n"
"               if specified, remove a trailing '.txt' from FILENAME.\n\n"
"  -b, --block  Use unicode block elements for more compact codes.\n\n"
"  -l {LMQH}, --level={LMQH}\n"
"               specify error collectin level from L (lowest) to H (highest).\n"
"               (default=L)\n\n"
"  -v NUMBER, --symversion=NUMBER\n"
"               specify the version of the symbol. (default=auto)\n\n"
"  -S, --structured\n"
"               make structured symbols. Version and output file must be specified.\n\n"
"  -k, --kanji  assume that the input text contains kanji (shift-jis).\n\n"
"  -c, --casesensitive\n"
"               encode lower-case alphabet characters in 8-bit mode. (default)\n\n"
"  -i, --ignorecase\n"
"               ignore case distinctions and use only upper-case characters.\n\n"
"  -8, -8bit    encode entire data in 8-bit mode. -k, -c and -i will be ignored.\n\n"
"  -V, --version\n"
"               display the version number and copyrights of the ttyqr.\n\n"
"  [STRING]     input data. If it is not specified, data will be taken from\n"
"               standard input.\n"
			);
		} else {
			fprintf(stderr,
"Usage: ttyqr [OPTION]... [STRING]\n"
"Encode input data in a QR Code and display it on terminal.\n\n"
"  -h           display this message.\n"
"  --help       display the usage of long options.\n"
"  -o FILENAME  write PNG image to FILENAME. If '-' is specified, the result\n"
"               will be output to standard output. If -S is given, structured\n"
"               symbols are written to FILENAME-01.txt, FILENAME-02.txt, ...;\n"
"               if specified, remove a trailing '.txt' from FILENAME.\n"
"  -b           Use unicode block elements for more compact codes.\n"
"  -l {LMQH}    specify error collectin level from L (lowest) to H (highest).\n"
"               (default=L)\n"
"  -v NUMBER    specify the version of the symbol. (default=auto)\n"
"  -S           make structured symbols. Version and output file must be specified.\n"
"  -k           assume that the input text contains kanji (shift-jis).\n"
"  -c           encode lower-case alphabet characters in 8-bit mode. (default)\n"
"  -i           ignore case distinctions and use only upper-case characters.\n"
"  -8           encode entire data in 8-bit mode. -k, -c and -i will be ignored.\n"
"  -V           display the version number and copyrights of the ttyqr.\n"
"  [STRING]     input data. If it is not specified, data will be taken from\n"
"               standard input.\n"
			);
		}
	}
}

#define MAX_DATA_SIZE (7090 * 16) /* from the specification */
static char *readStdin(void)
{
	char *buffer;
	int ret;

	buffer = (char *)malloc(MAX_DATA_SIZE);
	if(buffer == NULL) {
		fprintf(stderr, "Memory allocation failed.\n");
		exit(EXIT_FAILURE);
	}
	ret = fread(buffer, 1, MAX_DATA_SIZE, stdin);
	if(ret == 0) {
		fprintf(stderr, "No input data.\n");
		exit(EXIT_FAILURE);
	}
	if(feof(stdin) == 0) {
		fprintf(stderr, "Input data is too large.\n");
		exit(EXIT_FAILURE);
	}

	buffer[ret] = '\0';

	return buffer;
}


/**
 * Printing QR code on terminal borrowed from Google's
 * Authenticator
 * http://code.google.com/p/google-authenticator/source/browse/libpam/google-authenticator.c
 * Copyright 2010 Google Inc.
 * Author: Markus Gutschke
 *
 * Licensed under the Apache License, Version 2.0
 */

#define ANSI_RESET        "\x1B[0m\n"
#define ANSI_BLACKONGREY  "\x1B[30;47;27m"
#define ANSI_WHITE        "\x1B[27m"
#define ANSI_BLACK        "\x1B[7m"
#define UTF8_BOTH         "\xE2\x96\x88"
#define UTF8_TOPHALF      "\xE2\x96\x80"
#define UTF8_BOTTOMHALF   "\xE2\x96\x84"

static int writeTTY(QRcode *qrcode, const char *outfile) {
	FILE *fp = stdout;
	//
	// Output QRCode using ANSI colors. Instead of black on white, we
	// output black on grey, as that works independently of whether the
	// user runs his terminals in a black on white or white on black color
	// scheme.
	// But this requires that we print a border around the entire QR Code.
	// Otherwise, readers won't be able to recognize it.
	if(outfile[0] == '-' && outfile[1] == '\0') {
		fp = stdout;
	} else {
		fp = fopen(outfile, "w");
		if(fp == NULL) {
			fprintf(stderr, "Failed to create file: %s\n", outfile);
			perror(NULL);
			exit(EXIT_FAILURE);
		}
	}

	char *ptr = (char *)qrcode->data;

	if (!useBlockElements) {
	  for (int i = 0; i < 2; ++i) {
	    fprintf(fp, ANSI_BLACKONGREY);
	    for (int x = 0; x < qrcode->width + 4; ++x) fprintf(fp, "  ");
	    fputs(ANSI_RESET, fp);
	  }
	  for (int y = 0; y < qrcode->width; ++y) {
	    fprintf(fp, ANSI_BLACKONGREY"    ");
	    int isBlack = 0;
	    for (int x = 0; x < qrcode->width; ++x) {
	      if (*ptr++ & 1) {
	        if (!isBlack) {
	          fprintf(fp, ANSI_BLACK);
	        }
	        isBlack = 1;
	      } else {
	        if (isBlack) {
	          fprintf(fp, ANSI_WHITE);
	        }
	        isBlack = 0;
	      }
	      fprintf(fp, "  ");
	    }
	    if (isBlack) {
	      fprintf(fp, ANSI_WHITE);
	    }
	    fputs("    "ANSI_RESET, fp);
	  }
	  for (int i = 0; i < 2; ++i) {
	    fprintf(fp, ANSI_BLACKONGREY);
	    for (int x = 0; x < qrcode->width + 4; ++x) fprintf(fp, "  ");
	    fputs(ANSI_RESET, fp);
	  }
	} else {
	  // Drawing the QRCode with Unicode block elements is desirable as
	  // it makes the code much smaller, which is often easier to scan.
	  // Unfortunately, many terminal emulators do not display these
	  // Unicode characters properly.
	  fprintf(fp, ANSI_BLACKONGREY);
	  for (int i = 0; i < qrcode->width + 4; ++i) {
	    fprintf(fp, " ");
	  }
	  fputs(ANSI_RESET, fp);
	  for (int y = 0; y < qrcode->width; y += 2) {
	    fprintf(fp, ANSI_BLACKONGREY"  ");
	    for (int x = 0; x < qrcode->width; ++x) {
	      int top = qrcode->data[y*qrcode->width + x] & 1;
	      int bottom = 0;
	      if (y+1 < qrcode->width) {
	        bottom = qrcode->data[(y+1)*qrcode->width + x] & 1;
	      }
	      if (top) {
	        if (bottom) {
	          fprintf(fp, UTF8_BOTH);
	        } else {
	          fprintf(fp, UTF8_TOPHALF);
	        }
	      } else {
	        if (bottom) {
	          fprintf(fp, UTF8_BOTTOMHALF);
	        } else {
	          fprintf(fp, " ");
	        }
	      }
	    }
	    fputs("  "ANSI_RESET, fp);
	  }
	  fprintf(fp, ANSI_BLACKONGREY);
	  for (int i = 0; i < qrcode->width + 4; ++i) {
	    fprintf(fp, " ");
	  }
	  fputs(ANSI_RESET, fp);
	}

	fclose(fp);
	return 0;
}

static QRcode *encode(const char *intext)
{
	QRcode *code;

	if(eightbit) {
		code = QRcode_encodeString8bit(intext, version, level);
	} else {
		code = QRcode_encodeString(intext, version, level, hint, casesensitive);
	}

	return code;
}

static void qrencode(const char *intext, const char *outfile)
{
	QRcode *qrcode;
	
	qrcode = encode(intext);
	if(qrcode == NULL) {
		perror("Failed to encode the input data:");
		exit(EXIT_FAILURE);
	}
	writeTTY(qrcode, outfile);
	QRcode_free(qrcode);
}

static QRcode_List *encodeStructured(const char *intext)
{
	QRcode_List *list;

	if(eightbit) {
		list = QRcode_encodeString8bitStructured(intext, version, level);
	} else {
		list = QRcode_encodeStringStructured(intext, version, level, hint, casesensitive);
	}

	return list;
}

static void qrencodeStructured(const char *intext, const char *outfile)
{
	QRcode_List *qrlist, *p;
	char filename[FILENAME_MAX];
	char *base, *q, *suffix = NULL;
	int i = 1;

	base = strdup(outfile);
	if(base == NULL) {
		fprintf(stderr, "Failed to allocate memory.\n");
		exit(EXIT_FAILURE);
	}
	if(strlen(base) > 4) {
		q = base + strlen(base) - 4;
		if(strcasecmp(".txt", q) == 0) {
			suffix = strdup(q);
			*q = '\0';
		}
	}
	
	qrlist = encodeStructured(intext);
	if(qrlist == NULL) {
		perror("Failed to encode the input data:");
		exit(EXIT_FAILURE);
	}

	for(p = qrlist; p != NULL; p = p->next) {
		if(p->code == NULL) {
			fprintf(stderr, "Failed to encode the input data.\n");
			exit(EXIT_FAILURE);
		}
		if(suffix) {
			snprintf(filename, FILENAME_MAX, "%s-%02d%s", base, i, suffix);
		} else {
			snprintf(filename, FILENAME_MAX, "%s-%02d", base, i);
		}
		writeTTY(p->code, filename);
		i++;
	}

	free(base);
	if(suffix) {
		free(suffix);
	}

	QRcode_List_free(qrlist);
}

int main(int argc, char **argv)
{
	int opt, lindex = -1;
	char *outfile = "-";
	char *intext = NULL;

	while((opt = getopt_long(argc, argv, optstring, options, &lindex)) != -1) {
		switch(opt) {
			case 'h':
				if(lindex == 0) {
					usage(1, 1);
				} else {
					usage(1, 0);
				}
				exit(0);
				break;
			case 'o':
				outfile = optarg;
				break;
			case 'v':
				version = atoi(optarg);
				if(version < 0) {
					fprintf(stderr, "Invalid version: %d\n", version);
					exit(EXIT_FAILURE);
				}
				break;
			case 'l':
				switch(*optarg) {
					case 'l':
					case 'L':
						level = QR_ECLEVEL_L;
						break;
					case 'm':
					case 'M':
						level = QR_ECLEVEL_M;
						break;
					case 'q':
					case 'Q':
						level = QR_ECLEVEL_Q;
						break;
					case 'h':
					case 'H':
						level = QR_ECLEVEL_H;
						break;
					default:
						fprintf(stderr, "Invalid level: %s\n", optarg);
						exit(EXIT_FAILURE);
						break;
				}
				break;
			case 'S':
				structured = 1;
			case 'k':
				hint = QR_MODE_KANJI;
				break;
			case 'c':
				casesensitive = 1;
				break;
			case 'i':
				casesensitive = 0;
				break;
			case '8':
				eightbit = 1;
				break;
			case 'V':
				usage(0, 0);
				exit(0);
				break;
			case 'b':
				useBlockElements = 1;
				break;
			default:
				fprintf(stderr, "Try `ttyqr --help' for more information.\n");
				exit(EXIT_FAILURE);
				break;
		}
	}

	if(optind < argc) {
		intext = argv[optind];
	}
	if(intext == NULL) {
		intext = readStdin();
	}

	if(structured) {
		if(version == 0) {
			fprintf(stderr, "Version must be specified to encode structured symbols.\n");
			exit(EXIT_FAILURE);
		}
		if(outfile[0] == '-' && outfile[1] == '\0') {
			fprintf(stderr, "Output file name must be specified to encode structured symbols.\n");
			exit(EXIT_FAILURE);
		}
		qrencodeStructured(intext, outfile);
	} else {
		qrencode(intext, outfile);
	}

	return 0;
}
