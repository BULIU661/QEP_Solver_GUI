//==============================================================================
//  include/config/TableFormatter.h  —  集中式表格与框线格式化引擎（header-only）
//==============================================================================

#ifndef TABLE_FORMATTER_H
#define TABLE_FORMATTER_H

#include <string>
#include <vector>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <algorithm>
#include <cmath>

namespace fmt {

// ==================== 显示宽度计算 ====================

inline int displayWidth(const std::string &s) {
    int w = 0;
    for (size_t i = 0; i < s.size(); ) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 0x80) { w++; i++; }
        else if (c < 0xE0) { w += 2; i += 2; }
        else if (c == 0xE2) { w++; i += 3; }
        else { w += 2; i += 3; }
    }
    return w;
}

// ==================== 辅助函数 ====================

inline std::string rep(int n, char c) { return std::string(n, c); }

inline std::string padRight(const std::string &s, int width) {
    int dw = displayWidth(s);
    if (dw >= width) return s;
    return s + std::string(width - dw, ' ');
}
inline std::string padLeft(const std::string &s, int width) {
    int dw = displayWidth(s);
    if (dw >= width) return s;
    return std::string(width - dw, ' ') + s;
}
inline std::string padCenter(const std::string &s, int width) {
    int dw = displayWidth(s);
    if (dw >= width) return s;
    int left = (width - dw) / 2;
    int right = width - dw - left;
    return std::string(left, ' ') + s + std::string(right, ' ');
}
inline std::string truncate(const std::string &s, int maxWidth) {
    if (displayWidth(s) <= maxWidth) return s;
    int w = 0;
    for (size_t i = 0; i < s.size(); ) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        int adv = (c < 0x80) ? 1 : ((c < 0xE0) ? 2 : 3);
        int dw = (c < 0x80) ? 1 : ((c == 0xE2) ? 1 : 2);
        if (w + dw > maxWidth - 3) return s.substr(0, i) + "...";
        w += dw; i += adv;
    }
    return s;
}

// ==================== 节标题（无框线） ====================

inline std::string secHdr(const std::string &title) {
    return "\n\xE2\x94\x80\xE2\x94\x80 " + title + " \xE2\x94\x80\xE2\x94\x80";  // ── Title ──
}
inline std::string secLine(const std::string &label, const std::string &value) {
    return "  " + label + " : " + value;
}
inline std::string subHdr(const std::string &title) {
    return "\n  \xE2\x96\xB8 " + title;  // ▸ Title
}

// ==================== 列规范 ====================

enum class Align { Left, Right, Center };

struct Column {
    std::string header;
    int width;
    Align align;

    std::string format(const std::string &cell) const {
        switch (align) {
            case Align::Right:  return padLeft(cell, width);
            case Align::Center: return padCenter(cell, width);
            default:            return padRight(cell, width);
        }
    }
};

// ==================== TableBuilder（无竖线分隔符） ====================

class TableBuilder {
public:
    TableBuilder() = default;

    TableBuilder& addCol(const std::string &header, int width, Align a = Align::Left) {
        cols_.push_back({header, width, a});
        return *this;
    }

    TableBuilder& addRow(std::initializer_list<std::string> cells) {
        std::vector<std::string> row;
        for (const auto &c : cells) row.push_back(c);
        rows_.push_back(row);
        return *this;
    }

    TableBuilder& addRow(const std::vector<std::string> &cells) {
        rows_.push_back(cells);
        return *this;
    }

    TableBuilder& sep() {
        rows_.push_back({"__SEP__"});
        return *this;
    }

    int totalWidth() const {
        if (cols_.empty()) return 0;
        int w = 0;
        for (size_t i = 0; i < cols_.size(); ++i) {
            if (i > 0) w += 3;  // 3 spaces between columns
            w += cols_[i].width;
        }
        return w;
    }

    std::string render(int indent = 0) const {
        if (cols_.empty()) return "";
        std::string pre(indent, ' ');
        std::ostringstream oss;

        // underline for header
        auto underLine = [&]() {
            int tw = 0;
            for (size_t i = 0; i < cols_.size(); ++i) {
                if (i > 0) tw += 3;
                tw += cols_[i].width;
            }
            oss << pre << rep(tw, '\xC4') << "\n";  // ASCII ─ (0xC4 in codepage 437, but use '-' for safety)
        };

        // 表头
        oss << pre;
        for (size_t i = 0; i < cols_.size(); ++i) {
            if (i > 0) oss << "   ";
            oss << cols_[i].format(cols_[i].header);
        }
        oss << "\n";

        // 下划线 (用 ASCII '-')
        {
            int tw = 0;
            for (size_t i = 0; i < cols_.size(); ++i) {
                if (i > 0) tw += 3;
                tw += cols_[i].width;
            }
            oss << pre << rep(tw, '-') << "\n";
        }

        // 数据行
        for (const auto &row : rows_) {
            if (row.size() == 1 && row[0] == "__SEP__") continue;
            oss << pre;
            for (size_t i = 0; i < cols_.size(); ++i) {
                if (i > 0) oss << "   ";
                std::string cell = (i < row.size()) ? row[i] : "";
                oss << cols_[i].format(cell);
            }
            oss << "\n";
        }
        return oss.str();
    }

private:
    std::vector<Column> cols_;
    std::vector<std::vector<std::string>> rows_;
};

// ==================== 4 级残差状态 ====================

inline const char* residualStatus(double relRes) {
    if (relRes < 0)       return "N/A";
    if (relRes < 1e-10)   return "EXCELLENT";
    if (relRes < 1e-8)    return "OK";
    if (relRes < 1e-6)    return "ACCEPTABLE";
    return "WARNING";
}

// ==================== 概览卡片与性能统计 ====================

template<typename Result>
inline std::string summaryCard(const std::string &caseName,
                                const std::string &solverName,
                                const Result &res) {
    std::ostringstream oss;
    int nEig = static_cast<int>(res.eigenvalues_real.size());
    oss << "\n  Problem   : " << caseName << "\n";
    oss << "  Method    : " << solverName << "\n";
    std::string st = res.eigensolver_fully_converged
        ? "CONVERGED - " + std::to_string(nEig) + " eigenvalues"
        : "PARTIAL - not fully converged";
    oss << "  Status    : " << (res.success ? "[OK] " : "[FAIL] ") << st << "\n";
    oss << "  Eigenvalues: " << nEig << " found\n";
    char buf[128];
    snprintf(buf, sizeof(buf), "  Total Time : %.3f s", res.solution_time);
    oss << buf << "\n";
    if (!res.residuals_rel.empty()) {
        double maxRel = *std::max_element(res.residuals_rel.begin(), res.residuals_rel.end());
        snprintf(buf, sizeof(buf), "  Max Residual: %.2e", maxRel);
        oss << buf << "\n";
    }
    return oss.str();
}

template<typename Result>
inline std::string performanceStats(const Result &res) {
    std::ostringstream oss;
    oss << "\n  Performance Statistics\n";
    oss << "  " << rep(50, '-') << "\n";
    char buf[128];
    snprintf(buf, sizeof(buf), "  Build time        : %.4f s", res.build_time); oss << buf << "\n";
    snprintf(buf, sizeof(buf), "  Preprocessing time: %.4f s", res.preprocessing_time); oss << buf << "\n";
    snprintf(buf, sizeof(buf), "  Arnoldi time      : %.4f s", res.arnoldi_time); oss << buf << "\n";
    snprintf(buf, sizeof(buf), "  Total time        : %.4f s", res.solution_time); oss << buf << "\n\n";
    snprintf(buf, sizeof(buf), "  Arnoldi iterations: %d", res.arnoldi_iterations); oss << buf << "\n";
    snprintf(buf, sizeof(buf), "  Inner solves      : %d", res.total_inner_solves); oss << buf << "\n";
    snprintf(buf, sizeof(buf), "  Inner iterations  : %d", res.total_inner_iterations); oss << buf << "\n";
    if (res.total_inner_solves > 0) {
        snprintf(buf, sizeof(buf), "  Avg iterations/solve: %.1f", res.avg_inner_iterations); oss << buf << "\n";
    }
    snprintf(buf, sizeof(buf), "  Linear solve time : %.4f s", res.linear_solve_total_time); oss << buf << "\n";
    return oss.str();
}

} // namespace fmt

#endif // TABLE_FORMATTER_H
