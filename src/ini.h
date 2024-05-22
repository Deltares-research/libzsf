
#pragma once

#ifndef _INI_H
#define _INI_H

#define INI_OK     (0)  /* okay */
#define INI_FAIL  (-1)  /* failed */

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*ini_callback)(char *section, char *key, char *value, void *data_ptr);

/* read ini file at filepath, for each key/value pair in the file
 * the callback function is called. It is up to the callback function
 * to figure out what to do with the value and store it into memory
 * pointed to by dataptr in a meaningful way. the callback function
 * should return INI_FAIL if there was a fatal problem, or INI_OK otherwise.
 *
 * Returns: 0  if all was ok.
 *          -1 if there was a file i/o problem (file not found or read error).
 *          >0 line number if there was a problem in that line of the file.
 */
int ini_read(char *filepath, ini_callback callback, void *data_ptr);

#ifdef __cplusplus
}
#endif

#endif // _INI_H
