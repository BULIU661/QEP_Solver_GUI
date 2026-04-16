// utils/ConsoleUtils.h
#ifndef CONSOLE_UTILS_H
#define CONSOLE_UTILS_H

#include <string>

namespace QEP {

int getDisplayWidth(const std::string &str);
std::string truncateToWidth(const std::string &str, int maxDisplayWidth);
std::string padToWidth(const std::string &str, int targetWidth);
std::string padToWidthRight(const std::string &str, int targetWidth);

} // namespace QEP

#endif // CONSOLE_UTILS_H
