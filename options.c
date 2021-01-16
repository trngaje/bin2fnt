#include "options.h"

struct option longopts[] = {
	{ "extract_char", required_argument, NULL, 'c' },
	{ "extract_unicode", required_argument, NULL, 'u' },
	{ "extract_list", required_argument, NULL, 'l' },
    { "code_custom_start", required_argument, NULL, 's' },
    { "code_custom_end", required_argument, NULL, 'e' },	

    { "verbose", no_argument, NULL, 'v' },
    { "help", no_argument, NULL, 'h' },
    { 0, 0, 0, 0 }};



struct opt g_opt;
