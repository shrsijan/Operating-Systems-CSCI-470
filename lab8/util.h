#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>

/**
 * Parse input file containing memory operations
 * @param input_file: file pointer to input file
 * @param input: array to store parsed operations [pid][size]
 * @param n: pointer to store number of operations
 * @param size: pointer to store total memory partition size
 */
void parse_file(FILE *input_file, int input[][2], int *n, int *size);

#endif