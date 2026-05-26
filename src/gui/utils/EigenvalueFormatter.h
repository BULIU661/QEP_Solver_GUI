//==============================================================================
//  src/gui/utils/EigenvalueFormatter.h  —  特征值字符串统一格式化（header-only）
//==============================================================================
#ifndef QEP_GUI_EIGENVALUE_FORMATTER_H
#define QEP_GUI_EIGENVALUE_FORMATTER_H

#include <QString>
#include <string>
#include <cmath>

namespace fmt {

inline std::string formatEigenvalue(double re, double im, int precision = 6) {
    char buf[128];
    if (std::abs(im) < 1e-8)
        snprintf(buf, sizeof(buf), "%.*g", precision, re);
    else if (std::abs(re) < 1e-8)
        snprintf(buf, sizeof(buf), "%.*gi", precision, im);
    else
        snprintf(buf, sizeof(buf), "%.*g%+.*gi", precision, re, precision, im);
    return std::string(buf);
}

inline QString formatEigenvalueQt(double re, double im, int precision = 6) {
    if (std::abs(im) < 1e-8)
        return QString::number(re, 'g', precision);
    if (std::abs(re) < 1e-8)
        return QString::number(im, 'g', precision) + "i";
    return QString::number(re, 'g', precision)
           + (im >= 0 ? "+" : "")
           + QString::number(im, 'g', precision) + "i";
}

} // namespace fmt

#endif // QEP_GUI_EIGENVALUE_FORMATTER_H
