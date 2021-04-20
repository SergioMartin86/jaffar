#pragma once

#include "nlohmann/json.hpp"
#include <string>
#include <sstream>
#include <vector>
#include <iterator>

// Function to split a string into a sub-strings delimited by a character
// Taken from stack overflow answer to https://stackoverflow.com/questions/236129/how-do-i-iterate-over-the-words-of-a-string
// By Evan Teran

template <typename Out>
void split(const std::string &s, char delim, Out result) {
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
        *result++ = item;
    }
}

std::vector<std::string> split(const std::string &s, char delim);

// Logging functions
void exitWithError [[noreturn]] (const char *fileName, const int lineNumber, const char *format, ...);

#define EXIT_WITH_ERROR(...) \
  exitWithError(__FILE__, __LINE__, __VA_ARGS__)

// Checks if directory exists
bool dirExists(const std::string dirPath);

// Loads a string from a given file
bool loadStringFromFile(std::string &dst, const char *fileName);

// Checks whether a given key is present in the JSON object.
template <typename T, typename... Key>
bool isDefined(T &js, const Key &... key)
{
  auto *tmp = &js;
  bool result = true;
  decltype(tmp->begin()) it;
  ((result && ((it = tmp->find(key)) == tmp->end() ? (result = false) : (tmp = &*it, true))), ...);
  return result;
}

// Function to split a vector into n mostly fair chunks
template<typename T>
std::vector<T> splitVector(const T size, const T n)
{
  std::vector<T> subSizes(n);

  T length = size / n;
  T remain = size % n;

  for (T i = 0; i < n; i++)
   subSizes[i] = i < remain ? length + 1 : length;

  return subSizes;
}

// Functions to capture single key presses for commands in playback
int kbhit();
int getKeyPress();
