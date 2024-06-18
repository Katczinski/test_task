#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

bool is_valid_ip(char *str);
bool is_valid_file_path(char *str);
bool is_valid_prefix(char *str);
bool extract_ip_port(char *str, unsigned int *ip, short unsigned int *port);

#endif
