#pragma once

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
