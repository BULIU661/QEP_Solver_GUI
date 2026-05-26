//==============================================================================
//  src/qep/io/MatrixIO.h  —  稀疏/稠密矩阵文件读写接口声明
//==============================================================================

#ifndef MATRIX_IO_H
#define MATRIX_IO_H

#include <vector>
#include <utility>
#include <Eigen/SparseCore>
#include <string>


namespace QEP {

// 二进制 CSR 读写
void convertBinaryFiles(const std::vector<std::pair<std::string, std::string>> &files);
bool convertTextCSRtoBinary(const std::string &text_file, const std::string &bin_file);
Eigen::SparseMatrix<double> readBinaryCSR(const std::string &bin_file);

// 文本 CSR/COO 读取（内部使用，不对外暴露，但为了测试可声明）
Eigen::SparseMatrix<double> read_sparse_matrix_csr(const std::string &filename);
Eigen::SparseMatrix<double> read_sparse_matrix_coo(const std::string &filename);

// 密集矩阵读取
Eigen::MatrixXd read_dense_matrix(const std::string &filename);

// 检测文件格式
bool detectCOOFormat(const std::string &filename);

void completeSymmetricCSR(const std::string &infile, const std::string &outfile);



} // namespace QEP

#endif // MATRIX_IO_H
