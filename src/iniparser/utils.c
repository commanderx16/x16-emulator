#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <stdarg.h>
#include <ctype.h>
#include "utils.h"

/*-------------------------------------------------------------------------*/
/**
  @brief    Convert a string to lowercase.
  @param    in   String to convert.
  @param    out Output buffer.
  @param    len Size of the out buffer.
  @return   ptr to the out buffer or NULL if an error occured.

  This function convert a string into lowercase.
  At most len - 1 elements of the input string will be converted.
 */
/*--------------------------------------------------------------------------*/
const char * strlwc(const char * in, char *out, unsigned len)
{
    unsigned i ;

    if (in==NULL || out == NULL || len==0) return NULL ;
    i=0 ;
    while (in[i] != '\0' && i < len-1) {
        out[i] = (char)tolower((int)in[i]);
        i++ ;
    }
    out[i] = '\0';
    return out ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Duplicate a string
  @param    s String to duplicate
  @return   Pointer to a newly allocated string, to be freed with free()

  This is a replacement for strdup(). This implementation is provided
  for systems that do not have it.
 */
/*--------------------------------------------------------------------------*/
char * xstrdup(const char * s)
{
    char * t ;
    size_t len ;
    if (!s)
        return NULL ;

    len = strlen(s) + 1 ;
    t = (char*) malloc(len) ;
    if (t) {
        memcpy(t, s, len) ;
    }
    return t ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Remove blanks at the beginning and the end of a string.
  @param    str  String to parse and alter.
  @return   unsigned New size of the string.
 */
/*--------------------------------------------------------------------------*/
unsigned strstrip(char * s)
{
    char *last = NULL ;
    char *dest = s;

    if (s==NULL) return 0;

    last = s + strlen(s);
    while (isspace((int)*s) && *s) s++;
    while (last > s) {
        if (!isspace((int)*(last-1)))
            break ;
        last -- ;
    }
    *last = (char)0;

    memmove(dest,s,last - s + 1);
    return last - s;
}
