// utils/ConsoleUtils.cpp
#include "ConsoleUtils.h"
#include <string>


namespace QEP {

int getDisplayWidth(const std::string &str) {
    int width = 0;
    for (size_t i = 0; i < str.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        if ((c & 0x80) == 0) width += 1;
        else if ((c & 0xE0) == 0xC0) { width += 2; ++i; }
        else if ((c & 0xF0) == 0xE0) { width += 2; i += 2; }
        else if ((c & 0xF8) == 0xF0) { width += 2; i += 3; }
    }
    return width;
}

std::string truncateToWidth(const std::string &str, int maxDisplayWidth) {
    int width = 0;
    size_t bytePos = 0;
    while (bytePos < str.size() && width < maxDisplayWidth) {
        unsigned char c = static_cast<unsigned char>(str[bytePos]);
        int charWidth = 1, charBytes = 1;
        if ((c & 0x80) == 0) { charWidth = 1; charBytes = 1; }
        else if ((c & 0xE0) == 0xC0) { charWidth = 2; charBytes = 2; }
        else if ((c & 0xF0) == 0xE0) { charWidth = 2; charBytes = 3; }
        else if ((c & 0xF8) == 0xF0) { charWidth = 2; charBytes = 4; }
        if (width + charWidth > maxDisplayWidth) break;
        width += charWidth;
        bytePos += charBytes;
    }
    return str.substr(0, bytePos);
}

std::string padToWidth(const std::string &str, int targetWidth) {
    int currentWidth = getDisplayWidth(str);
    int padding = targetWidth - currentWidth;
    if (padding <= 0) return truncateToWidth(str, targetWidth);
    return str + std::string(padding, ' ');
}

std::string padToWidthRight(const std::string &str, int targetWidth) {
    int currentWidth = getDisplayWidth(str);
    int padding = targetWidth - currentWidth;
    if (padding <= 0) return truncateToWidth(str, targetWidth);
    return std::string(padding, ' ') + str;
}

} // namespace QEP
