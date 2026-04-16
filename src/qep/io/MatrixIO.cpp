// io/MatrixIO.cpp
#include "io/MatrixIO.h"
#include "config/GlobalConfig.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstdint>
#include <Eigen/Sparse>
#include <chrono>

namespace QEP
{
    // ---------- 辅助：检测 COO 格式 ----------
    bool detectCOOFormat(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file.is_open())
            return false;
        std::string line;
        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#')
                continue;
            break;
        }
        std::stringstream ss(line);
        int rows, cols, nnz;
        if (ss >> rows >> cols >> nnz)
        {
            if (std::getline(file, line))
            {
                std::stringstream ss2(line);
                int row, col;
                double value;
                if (ss2 >> row >> col >> value)
                    return true;
            }
        }
        return false;
    }

    // 批量二进制文件转换
    void convertBinaryFiles(const std::vector<std::pair<std::string, std::string>> &files)
    {
        for (const auto &pair : files)
        {
            std::cout << "转换" << pair.first << "到二进制格式" << pair.second << "...\n";
            auto start = std::chrono::high_resolution_clock::now();
            QEP::convertTextCSRtoBinary(pair.first, pair.second);
            auto end = std::chrono::high_resolution_clock::now();
            std::cout << "耗时: " << std::chrono::duration<double>(end - start).count() << " 秒\n";
        }
    }

    // ---------- convertTextCSRtoBinary ----------
  bool convertTextCSRtoBinary(const std::string &text_file, const std::string &bin_file)
{
    // -------------------- 1. 打开文本文件 --------------------
    std::ifstream infile(text_file);
    if (!infile.is_open())
    {
        std::cerr << "无法打开文本文件: " << text_file << std::endl;
        return false;
    }

    // -------------------- 2. 跳过注释行，读取矩阵维度 n --------------------
    std::string line;
    while (std::getline(infile, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        break;
    }
    int n;
    std::stringstream ss(line);
    ss >> n;
    if (ss.fail() || n <= 0)
    {
        std::cerr << "无效的矩阵维度" << std::endl;
        return false;
    }

    // -------------------- 3. 读取行指针 row_ptr (n+1 个整数) --------------------
    std::vector<int> row_ptr(n + 1);
    for (int i = 0; i <= n; ++i)
    {
        infile >> row_ptr[i];
        if (infile.fail())
        {
            std::cerr << "读取行指针失败" << std::endl;
            return false;
        }
    }

    // 判断是否为 1-based 索引，并计算原始非零元个数
    bool is_one_based = (row_ptr[0] == 1);
    int nnz_raw = row_ptr[n] - (is_one_based ? 1 : 0);

    // -------------------- 4. 读取列索引和数值 --------------------
    std::vector<int> col_idx(nnz_raw);
    for (int k = 0; k < nnz_raw; ++k)
    {
        infile >> col_idx[k];
        if (infile.fail())
        {
            std::cerr << "读取列索引失败" << std::endl;
            return false;
        }
        // 若为 1-based，立即转换为 0-based
        if (is_one_based)
            col_idx[k]--;
    }

    std::vector<double> values(nnz_raw);
    for (int k = 0; k < nnz_raw; ++k)
    {
        infile >> values[k];
        if (infile.fail())
        {
            std::cerr << "读取数值失败" << std::endl;
            return false;
        }
    }
    infile.close();

    // -------------------- 5. 将行指针转换为 0-based（若原为 1-based）--------------------
    if (is_one_based)
    {
        for (int i = 0; i <= n; ++i)
            row_ptr[i]--;
    }

    // -------------------- 6. 过滤零元，重新构建 CSR 数组 --------------------
    std::vector<int> new_col_idx;
    std::vector<double> new_values;
    std::vector<int> new_row_ptr(n + 1, 0);
    int new_nnz = 0;

    for (int i = 0; i < n; ++i)
    {
        new_row_ptr[i] = new_nnz;
        for (int k = row_ptr[i]; k < row_ptr[i + 1]; ++k)
        {
            // 过滤绝对值为 0 的项（避免显式存储零元）
            if (values[k] != 0.0)
            {
                new_col_idx.push_back(col_idx[k]);
                new_values.push_back(values[k]);
                ++new_nnz;
            }
        }
    }
    new_row_ptr[n] = new_nnz;

    // 用过滤后的数据替换原始数据
    col_idx = std::move(new_col_idx);
    values = std::move(new_values);
    row_ptr = std::move(new_row_ptr);
    int nnz = new_nnz;   // 更新非零元个数

    // -------------------- 7. 写入二进制文件 --------------------
    std::ofstream outfile(bin_file, std::ios::binary);
    if (!outfile.is_open())
    {
        std::cerr << "无法创建二进制文件: " << bin_file << std::endl;
        return false;
    }

    outfile.write(reinterpret_cast<const char *>(&n), sizeof(int));
    outfile.write(reinterpret_cast<const char *>(&nnz), sizeof(int));
    outfile.write(reinterpret_cast<const char *>(row_ptr.data()), (n + 1) * sizeof(int));
    outfile.write(reinterpret_cast<const char *>(col_idx.data()), nnz * sizeof(int));
    outfile.write(reinterpret_cast<const char *>(values.data()), nnz * sizeof(double));

    outfile.close();
    std::cout << "转换成功,二进制文件: " << bin_file << " (nnz=" << nnz << ")" << std::endl;
    return true;
}

    // ---------- readBinaryCSR ----------
    Eigen::SparseMatrix<double> readBinaryCSR(const std::string &bin_file)
    {
        auto t_0 = std::chrono::high_resolution_clock::now();
        std::ifstream infile(bin_file, std::ios::binary);
        if (!infile.is_open())
        {
            std::cerr << "无法打开二进制文件: " << bin_file << std::endl;
            return {};
        }
        infile.seekg(0, std::ios::end);
        std::streamsize file_size = infile.tellg();
        infile.seekg(0, std::ios::beg);
        if (Config::LOG_LEVEL >= 3)
            std::cout << "二进制文件大小: " << file_size << " 字节" << std::endl;

        int n = 0, nnz = 0;
        if (!infile.read(reinterpret_cast<char *>(&n), sizeof(int)))
        {
            std::cerr << "读取矩阵维度失败" << std::endl;
            return {};
        }
        if (!infile.read(reinterpret_cast<char *>(&nnz), sizeof(int)))
        {
            std::cerr << "读取非零元素个数失败" << std::endl;
            return {};
        }
        if (Config::LOG_LEVEL >= 3)
            std::cout << "矩阵维度 n = " << n << ", nnz = " << nnz << std::endl;

        std::streamsize expected_size = sizeof(int) * 2 + (n + 1) * sizeof(int) + nnz * sizeof(int) + nnz * sizeof(double);
        if (file_size < expected_size)
        {
            std::cerr << "错误:文件大小 " << file_size << " 小于预期 " << expected_size << std::endl;
            return {};
        }

        std::vector<int> row_ptr(n + 1);
        std::vector<int> col_idx(nnz);
        std::vector<double> values(nnz);
        if (!infile.read(reinterpret_cast<char *>(row_ptr.data()), (n + 1) * sizeof(int)))
        {
            std::cerr << "读取行指针失败" << std::endl;
            return {};
        }
        if (!infile.read(reinterpret_cast<char *>(col_idx.data()), nnz * sizeof(int)))
        {
            std::cerr << "读取列索引失败" << std::endl;
            return {};
        }
        if (!infile.read(reinterpret_cast<char *>(values.data()), nnz * sizeof(double)))
        {
            std::cerr << "读取数值失败" << std::endl;
            return {};
        }
        infile.close();

        std::vector<Eigen::Triplet<double>> triplets;
        triplets.reserve(nnz);
        for (int i = 0; i < n; ++i)
        {
            for (int k = row_ptr[i]; k < row_ptr[i + 1]; ++k)
            {
                int j = col_idx[k];
                if (j < 0 || j >= n)
                {
                    std::cerr << "无效列索引: " << j << std::endl;
                    return {};
                }
                triplets.emplace_back(i, j, values[k]);
            }
        }
        Eigen::SparseMatrix<double> mat(n, n);
        mat.setFromTriplets(triplets.begin(), triplets.end());
        auto t_1 = std::chrono::high_resolution_clock::now();
        if (Config::LOG_LEVEL >= 3)
            std::cout << "读取时间: " << std::chrono::duration<double>(t_1 - t_0).count() << " s\n";
        return mat;
    }

    // ---------- read_sparse_matrix_csr ----------
    Eigen::SparseMatrix<double> read_sparse_matrix_csr(const std::string &filename)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        std::ifstream file(filename);
        if (!file.is_open())
        {
            std::cerr << "错误: 无法打开文件 " << filename << std::endl;
            return {};
        }
        std::cout << "开始读取CSR格式稀疏矩阵: " << filename << std::endl;

        std::string line;
        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#')
                continue;
            break;
        }
        int n;
        std::stringstream ss(line);
        ss >> n;
        if (ss.fail() || n <= 0)
        {
            std::cerr << "错误: 无法读取矩阵维度或维度无效" << std::endl;
            return {};
        }

        std::vector<int> row_ptr(n + 1);
        for (int i = 0; i <= n; ++i)
        {
            file >> row_ptr[i];
            if (file.fail())
            {
                std::cerr << "错误: 读取行指针时失败,位置 " << i << std::endl;
                return {};
            }
        }

        bool is_one_based = (row_ptr[0] == 1);
        int nnz = row_ptr[n] - (is_one_based ? 1 : 0);
        std::vector<int> col_idx(nnz);
        for (int k = 0; k < nnz; ++k)
        {
            file >> col_idx[k];
            if (file.fail())
            {
                std::cerr << "错误: 读取列索引时失败,位置 " << k << std::endl;
                return {};
            }
        }
        std::vector<double> values(nnz);
        for (int k = 0; k < nnz; ++k)
        {
            file >> values[k];
            if (file.fail())
            {
                std::cerr << "错误: 读取数值时失败,位置 " << k << std::endl;
                return {};
            }
        }
        file.close();

        Eigen::SparseMatrix<double> mat(n, n);
        mat.reserve(nnz);
        for (int i = 0; i < n; ++i)
        {
            int start = row_ptr[i] - (is_one_based ? 1 : 0);
            int end = row_ptr[i + 1] - (is_one_based ? 1 : 0);
            for (int k = start; k < end; ++k)
            {
                int col = col_idx[k];
                if (is_one_based)
                    col--;
                mat.insert(i, col) = values[k];
            }
        }
        mat.makeCompressed();
        auto end_time = std::chrono::high_resolution_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        std::cout << "读取耗时: " << elapsed_ms << " ms" << std::endl;
        return mat;
    }

    // ---------- read_sparse_matrix_coo ----------
    Eigen::SparseMatrix<double> read_sparse_matrix_coo(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            std::cerr << "错误: 无法打开文件 " << filename << std::endl;
            return {};
        }
        std::string line;
        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#')
                continue;
            break;
        }
        int rows, cols, nnz;
        std::stringstream ss(line);
        ss >> rows >> cols >> nnz;
        std::vector<Eigen::Triplet<double>> triplets;
        triplets.reserve(nnz);
        for (int k = 0; k < nnz; ++k)
        {
            int row, col;
            double value;
            file >> row >> col >> value;
            triplets.emplace_back(row, col, value);
        }
        file.close();
        Eigen::SparseMatrix<double> matrix(rows, cols);
        matrix.setFromTriplets(triplets.begin(), triplets.end());
        matrix.makeCompressed();
        std::cout << "读取COO格式稀疏矩阵: " << filename
                  << ", 维度: " << rows << "x" << cols
                  << ", 非零元素: " << nnz << std::endl;
        return matrix;
    }

    // ---------- read_dense_matrix ----------
    Eigen::MatrixXd read_dense_matrix(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file.is_open())
            throw std::runtime_error("Cannot open file: " + filename);
        int rows = 0, cols = 0;
        std::string line;
        while (std::getline(file, line))
        {
            line.erase(0, line.find_first_not_of(" \t"));
            if (line.empty() || line[0] == '#')
                continue;
            std::stringstream ss(line);
            if (ss >> rows >> cols)
                break;
        }
        if (rows <= 0 || cols <= 0)
            throw std::runtime_error("Invalid matrix dimensions in file: " + filename);
        Eigen::MatrixXd mat(rows, cols);
        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                file >> mat(i, j);
                if (!file)
                    throw std::runtime_error("Failed to read matrix element");
            }
        }
        return mat;
    }

    /**   一半存储补全
     * 读取一半存储的 1-based CSR 文件，输出完整的对称 CSR 文件
     * 输入格式：首行 n，接着 n+1 行行指针，接着 nnz 行列索引，接着 nnz 个值
     * 输出格式：首行 n，接着 n+1 行行指针（完整矩阵），接着 nnz_full 行列索引，接着 nnz_full 个值
     */
    void completeSymmetricCSR(const std::string &infile, const std::string &outfile)
    {
        // ==================== 1. 读取输入 CSR 文件（1-based 索引） ====================
        std::ifstream fin(infile);
        if (!fin)
            throw std::runtime_error("无法打开输入文件: " + infile);

        int n;
        fin >> n;                             // 矩阵维度
        std::vector<int> row_ptr_half(n + 1); // 行指针（1-based）
        for (int i = 0; i <= n; ++i)
            fin >> row_ptr_half[i];

        int nnz_half = row_ptr_half[n] - 1; // 非零元个数（1-based 最后一个索引减1）
        std::vector<int> col_ind_half(nnz_half);
        for (int i = 0; i < nnz_half; ++i)
            fin >> col_ind_half[i]; // 列索引（1-based）

        std::vector<double> values_half(nnz_half);
        for (int i = 0; i < nnz_half; ++i)
            fin >> values_half[i]; // 数值

        fin.close();

        // ==================== 2. 构建 Eigen 稀疏矩阵（仅存一半） ====================
        using Triplet = Eigen::Triplet<double>;
        std::vector<Triplet> triplets;
        triplets.reserve(nnz_half);

        for (int i = 0; i < n; ++i)
        {
            int start = row_ptr_half[i] - 1;   // 转为 0-based 起始
            int end = row_ptr_half[i + 1] - 1; // 转为 0-based 结束（不包含）
            for (int k = start; k < end; ++k)
            {
                int j = col_ind_half[k] - 1; // 列索引转为 0-based
                double v = values_half[k];
                triplets.emplace_back(i, j, v);
            }
        }

        Eigen::SparseMatrix<double> A_half(n, n);
        A_half.setFromTriplets(triplets.begin(), triplets.end());
        A_half.makeCompressed(); // 压缩存储

        // ==================== 3. 补全为完整对称矩阵 ====================
        Eigen::SparseMatrix<double> At_half = A_half.transpose();
        Eigen::SparseMatrix<double> A_full = A_half + At_half;

        // 恢复对角线（相加后对角线加倍，用原始值覆盖）
        A_full.diagonal() = A_half.diagonal();
        A_full.makeCompressed(); // 再次压缩（可选）

        int nnz_full = A_full.nonZeros();
        std::cout << "原始非零元: " << nnz_half << "，完整非零元: " << nnz_full << std::endl;

        // ==================== 4. 输出为 CSR 文件（1-based 索引） ====================
        std::ofstream fout(outfile);
        if (!fout)
            throw std::runtime_error("无法创建输出文件: " + outfile);

        fout << n << "\n";

        // 转换为 RowMajor 以便正确获取行指针
        Eigen::SparseMatrix<double, Eigen::RowMajor> A_row = A_full;

        // 计算行指针（1-based）
        std::vector<int> row_ptr_full(n + 1, 1); // 初始为 1
        for (int i = 0; i < n; ++i)
        {
            int start = A_row.outerIndexPtr()[i];
            int end = A_row.outerIndexPtr()[i + 1];
            row_ptr_full[i + 1] = row_ptr_full[i] + (end - start);
        }

        // 输出行指针
        for (int i = 0; i <= n; ++i)
            fout << row_ptr_full[i] << "\n";

        // 输出列索引（按行，1-based）
        for (int i = 0; i < n; ++i)
        {
            for (Eigen::SparseMatrix<double, Eigen::RowMajor>::InnerIterator it(A_row, i); it; ++it)
            {
                fout << it.col() + 1 << "\n"; // 转为 1-based
            }
        }

        // 输出数值（与列索引顺序严格一致）
        for (int i = 0; i < n; ++i)
        {
            for (Eigen::SparseMatrix<double, Eigen::RowMajor>::InnerIterator it(A_row, i); it; ++it)
            {
                fout << std::scientific << std::setprecision(15) << it.value() << "\n";
            }
        }

        fout.close();
    }
} // namespace QEP
