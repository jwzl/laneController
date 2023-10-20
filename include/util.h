#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdint.h>

typedef struct {
	int Year;
	int Month;
	int DayOfWeek;
	int Day;
    int Hour;
	int Minute;
	int Second;
	int Milliseconds; //us
}__time_info;

void get_local_time(__time_info* info);
void util_sleep(uint64_t milliseconds);
void util_sleep_v2(long milliseconds);
int64_t get_time(void);
uint64_t get_timestamp(void);
char* util_strdup(const char* str);
int* util_intdup(int i);
double* util_doubledup(double d);
int util_strlen(char *str);
char* get_client_timestamp();
char* combine_strings(int strAmount, char *str1, ...);
char** string_split(const char* in, const char d);
void free_memory(char **str);
void free_string_split_result(char** result);
int string_contain(const char *str, const char *sub_str);

#endif
