#ifndef _INI_UTILS_H_
#define _INI_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

const char * strlwc(const char * in, char *out, unsigned len);
char * xstrdup(const char * s);
unsigned strstrip(char * s);

#ifdef __cplusplus
}
#endif

#endif
