
#include "ini.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define INI_STRING_DELIM '"'
#define INI_REM_CHAR1 '#'
#define INI_REM_CHAR2 ';'
#define INI_SECTION_START_CHAR '['
#define INI_SECTION_END_CHAR ']'
#define INI_EQU_CHAR1 '='
#define INI_EQU_CHAR2 ':'

#define _AT_EOL(p)           (*(p) == '\n')
#define _AT_REMARK(p)        (*(p) == INI_REM_CHAR1 || *(p) == INI_REM_CHAR2)
#define _AT_EQUALS(p)        (*(p) == INI_EQU_CHAR1 || *(p) == INI_EQU_CHAR2)
#define _AT_SECTION_START(p) (*(p) == INI_SECTION_START_CHAR)
#define _AT_SECTION_END(p)   (*(p) == INI_SECTION_END_CHAR)

#define INI_SECTION_LEN (200)
#define INI_LINE_LEN   (4096)


char *_skip_space(char *ptr)
{
  while(ptr && *ptr && isspace(*ptr) && !_AT_EOL(ptr)) { ptr++; }
  return ptr;
}


int _parse_section_header(char **parse_pos_ptr, char *section)
{
  int i = 0;
  char *parse_pos = *parse_pos_ptr;
  parse_pos++; // Skip '['
  while (*parse_pos && !_AT_SECTION_END(parse_pos) && i < INI_SECTION_LEN) {
    section[i++] = *parse_pos;
    parse_pos++;
  }
  section[i] = '\0';
  if (!_AT_SECTION_END(parse_pos)) { return INI_FAIL; }
  parse_pos++; // Skip ']'
  *parse_pos_ptr = parse_pos;
  return INI_OK;
}


int _parse_key(char **parse_pos_ptr, char *key_str)
{
  int i = 0;
  char *parse_pos = *parse_pos_ptr;
  while (*parse_pos && !_AT_EQUALS(parse_pos) && i < INI_LINE_LEN) {
    key_str[i++] = *parse_pos;
    parse_pos++;
  }
  while (i && isspace(key_str[i-1])) { i--; }
  key_str[i] = '\0';
  if (!_AT_EQUALS(parse_pos)) { return INI_FAIL; }
  *parse_pos_ptr = parse_pos;
  return INI_OK;
}


int _parse_value(char **parse_pos_ptr, char *value_str)
{
  int i = 0;
  char *parse_pos = *parse_pos_ptr;
  if (*parse_pos == INI_STRING_DELIM) {
    parse_pos++; // Skip '"'
    while(*parse_pos && *parse_pos != INI_STRING_DELIM && i < INI_LINE_LEN)
    {
      value_str[i++] = *parse_pos;
      parse_pos++;
    }
    if (*parse_pos != INI_STRING_DELIM) { return INI_FAIL; }
    parse_pos++; // Skip '"'
  }
  else {
    while (*parse_pos && !_AT_REMARK(parse_pos) && !_AT_EOL(parse_pos)) {
      value_str[i++] = *parse_pos;
      parse_pos++;
    }
    while (i && isspace(value_str[i-1])) { i--; }
  }
  value_str[i] = '\0';
  *parse_pos_ptr = parse_pos;
  return INI_OK;
}

int ini_read(char *filepath, ini_callback callback, void *data_ptr)
{
  char section[INI_SECTION_LEN+1];
  char buffer[INI_LINE_LEN+1];
  char key_str[INI_LINE_LEN+1];
  char value_str[INI_LINE_LEN+1];
  int  error_code = INI_OK;
  FILE *file_ptr  = NULL;
  char *parse_pos = NULL;
  int  line_no    = 0;

  assert(filepath != NULL);
  assert(callback != NULL);

  section[0] = '\0';
  key_str[0] = '\0';
  value_str[0] = '\0';

  file_ptr = fopen(filepath, "r");
  if (file_ptr == NULL) { return INI_FAIL; }

  while (!feof(file_ptr) && error_code == INI_OK)
  {
      parse_pos = fgets(buffer, INI_LINE_LEN, file_ptr);
      if (parse_pos == NULL || feof(file_ptr)) { break; }

      line_no++;
      parse_pos = _skip_space(parse_pos);
      if (_AT_SECTION_START(parse_pos))
      {
        if (_parse_section_header(&parse_pos, section) != INI_OK)
        { error_code = line_no; break; }
      }
      else if (isalpha(*parse_pos))
      {
        if (_parse_key(&parse_pos, key_str) != INI_OK)
        { error_code = line_no; break; }
        parse_pos = _skip_space(parse_pos);
        if (!_AT_EQUALS(parse_pos))
        { error_code = line_no; break; }
        parse_pos++;
        parse_pos = _skip_space(parse_pos);
        if (_parse_value(&parse_pos, value_str) != INI_OK)
        { error_code = line_no; break; }
        if (*value_str) {
          // Only try to update if there is a non-empty value string.
          if (callback(section, key_str, value_str, data_ptr) != INI_OK) {
            error_code = line_no;
            break;
          }
        }
      }

      parse_pos = _skip_space(parse_pos);
      if (*parse_pos && !(_AT_REMARK(parse_pos) || _AT_EOL(parse_pos)))
      { error_code = line_no; break; }
  }

  fclose(file_ptr);
  return error_code;
}
