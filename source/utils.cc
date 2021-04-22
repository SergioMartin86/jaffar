#include "utils.h"
#include <stdarg.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

void exitWithError [[noreturn]] (const char *fileName, const int lineNumber, const char *format, ...)
{
  char *outstr = 0;
  va_list ap;
  va_start(ap, format);
  vasprintf(&outstr, format, ap);

  std::string outString = outstr;
  free(outstr);

  char info[1024];

  snprintf(info, sizeof(info) - 1, " + From %s:%d\n", fileName, lineNumber);
  outString += info;

  throw std::runtime_error(outString.c_str());
}

bool dirExists(const std::string dirPath)
{
  DIR *dir = opendir(dirPath.c_str());
  if (dir)
  {
    closedir(dir);
    return true;
  }

  return false;
}

bool loadStringFromFile(std::string &dst, const char *fileName)
{
  FILE *fid = fopen(fileName, "r");
  if (fid != NULL)
  {
    fseek(fid, 0, SEEK_END);
    long fsize = ftell(fid);
    fseek(fid, 0, SEEK_SET); /* same as rewind(f); */

    char *string = (char *)malloc(fsize + 1);
    fread(string, 1, fsize, fid);
    fclose(fid);

    string[fsize] = '\0';

    dst = string;

    free(string);
    return true;
  }
  return false;
}

