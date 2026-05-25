//==============================================================================
//  src/qep/utils/CreateProblem.cpp  —  从矩阵文件构造二次特征值问题
//==============================================================================

#include "CreateProblem.h"
#include "io/MatrixIO.h"
#include <Eigen/Sparse>
#include <Eigen/Dense>
#include <fstream>
#include <iomanip>
#include <filesystem>

// QuadraticEigenvalueProblem 结构体包含以下字段：
    // Eigen::SparseMatrix<double> M;
    // Eigen::SparseMatrix<double> C;
    // Eigen::SparseMatrix<double> K;
    // bool use_sparse = true;
    // bool is_symmetric = true;
    // int dimension = 0;
    // std::string name;
    // double condition_number_K = 0.0; // K矩阵的条件数，用于自适应参数调整
    // 可扩展至自动化求解问题，例如：通过创建问题时判断矩阵性质和条件数自动选取方法等

namespace QEP {

QuadraticEigenvalueProblem createTestProblemFromFiles(
    const std::string &name,
    const std::string &M_file,
    const std::string &C_file,
    const std::string &K_file,
    bool is_sparse,
    const Config::SolverParams &params,
    bool auto_convert,
    bool overwrite) {
    QuadraticEigenvalueProblem problem;
    problem.name = name;

    // auto_convert 控制是否自动将 .txt 转为 .bin；overwrite 控制是否覆盖已有 .bin
    auto ensureBinary = [&](const std::string &filePath) -> std::string {
        if (filePath.size() < 4) return filePath;
        std::string ext = filePath.substr(filePath.size() - 4);
        std::string base = filePath.substr(0, filePath.size() - 4);
        std::string binPath = base + ".bin";
        std::string txtPath = base + ".txt";

        // auto_convert 关闭：不做任何转换，直接返回原路径
        if (!auto_convert)
            return filePath;

        // 已经是 .bin → 检查是否存在
        if (ext == ".bin" || ext == ".BIN") {
            std::ifstream test(binPath);
            if (test.good()) {
                // overwrite 开启时，若 .txt 存在则重新转换
                if (overwrite) {
                    std::ifstream txtTest(txtPath);
                    if (txtTest.good()) {
                        convertTextCSRtoBinary(txtPath, binPath);
                    }
                }
                return binPath;
            }
            // .bin 不存在，尝试从 .txt 创建
            std::ifstream txtTest(txtPath);
            if (txtTest.good()) {
                convertTextCSRtoBinary(txtPath, binPath);
                return binPath;
            }
            return filePath; // 都不存在，让 readBinaryCSR 报错
        }

        // .txt → 转为 .bin
        if (ext == ".txt" || ext == ".TXT") {
            std::ifstream test(binPath);
            if (!test.good() || overwrite)
                convertTextCSRtoBinary(filePath, binPath);
            return binPath;
        }

        return filePath;
    };
    std::string M_bin = ensureBinary(M_file);
    std::string C_bin = ensureBinary(C_file);
    std::string K_bin = ensureBinary(K_file);

    // 尝试读取矩阵文件
    try {
        if (is_sparse) {
            problem.M = readBinaryCSR(M_bin);
            problem.C = readBinaryCSR(C_bin);
            problem.K = readBinaryCSR(K_bin);
        } else {
            Eigen::MatrixXd M_dense = read_dense_matrix(M_file);
            Eigen::MatrixXd C_dense = read_dense_matrix(C_file);
            Eigen::MatrixXd K_dense = read_dense_matrix(K_file);
            problem.M = M_dense.sparseView();
            problem.C = C_dense.sparseView();
            problem.K = K_dense.sparseView();
        }
        problem.dimension = problem.M.rows();
        if (problem.C.rows() != problem.dimension || problem.K.rows() != problem.dimension) {
            problem.dimension = 0; // 调用方通过 dimension==0 检测失败
            return problem;
        }
    } catch (const std::exception &e) {
        problem.dimension = 0;
        problem.load_error = std::string("Matrix load failed: ") + e.what();
        return problem;
    }

    // 条件按需计算：仅当自适应参数或条件估计开启时才计算 K 矩阵条件数
    if (params.enable_condition_estimation || params.enable_adaptive)
    {
        problem.condition_number_K = estimateConditionNumber(problem.K);
    }

    return problem;
}

nlohmann::json computeProblemMetadata(
    const QuadraticEigenvalueProblem &problem,
    bool compute_expensive)
{
    nlohmann::json meta;

    // 基础信息（零开销）
    meta["dimension"] = problem.dimension;
    meta["M_nnz"] = static_cast<int>(problem.M.nonZeros());
    meta["C_nnz"] = static_cast<int>(problem.C.nonZeros());
    meta["K_nnz"] = static_cast<int>(problem.K.nonZeros());

    // 存储类型 → 字符串
    auto storageToStr = [](MatrixStorageType s) -> std::string {
        switch (s) {
            case MatrixStorageType::FullSymmetric:    return "FullSymmetric";
            case MatrixStorageType::FullNonSymmetric: return "FullNonSymmetric";
            case MatrixStorageType::UpperTriangular:  return "UpperTriangular";
            case MatrixStorageType::LowerTriangular:  return "LowerTriangular";
            case MatrixStorageType::NonSquare:        return "NonSquare";
            default: return "Unknown";
        }
    };

    // 定性 → 字符串
    auto defToStr = [](MatrixDefiniteness d) -> std::string {
        switch (d) {
            case MatrixDefiniteness::SymmetricPositiveDefinite:     return "SPD";
            case MatrixDefiniteness::SymmetricPositiveSemiDefinite: return "SPSD";
            case MatrixDefiniteness::SymmetricIndefinite:           return "Indefinite";
            case MatrixDefiniteness::SymmetricNegativeDefinite:     return "NegativeDefinite";
            case MatrixDefiniteness::NonSymmetric:                  return "NonSymmetric";
            case MatrixDefiniteness::Unknown:                       return "Unknown";
            default: return "Unknown";
        }
    };

    // 逐矩阵计算性质
    auto computeProps = [&](const Eigen::SparseMatrix<double> &mat) -> nlohmann::json {
        nlohmann::json props;
        bool sym = isSymmetric(mat);
        props["symmetric"] = sym;
        props["storage"] = storageToStr(checkMatrixStorage(mat));
        if (sym)
            props["definiteness"] = defToStr(classifyDefiniteness(mat, 1e-12));
        return props;
    };

    meta["M_properties"] = computeProps(problem.M);
    meta["C_properties"] = computeProps(problem.C);
    meta["K_properties"] = computeProps(problem.K);

    // 条件数（可能开销较大，由 compute_expensive 门控）
    nlohmann::json cond;
    if (compute_expensive) {
        cond["M"] = estimateConditionNumber(problem.M);
        cond["C"] = estimateConditionNumber(problem.C);
        cond["K"] = problem.condition_number_K > 0.0
                     ? problem.condition_number_K
                     : estimateConditionNumber(problem.K);
    } else {
        cond["M"] = -1.0;
        cond["C"] = -1.0;
        cond["K"] = problem.condition_number_K;
    }
    meta["condition_numbers"] = cond;

    return meta;
}

bool writeProblemJson(const std::string &directory,
                      const nlohmann::json &newData)
{
    namespace fs = std::filesystem;
    fs::path jsonPath = fs::path(directory) / "problem.json";

    // 与已有 problem.json 合并
    nlohmann::json combined;
    if (fs::exists(jsonPath)) {
        try {
            std::ifstream in(jsonPath);
            in >> combined;
        } catch (const std::exception &) {
            combined = nlohmann::json::object();
        }
    }

    // 写入新字段
    for (auto it = newData.begin(); it != newData.end(); ++it)
        combined[it.key()] = it.value();

    try {
        std::ofstream out(jsonPath);
        out << std::setw(4) << combined << std::endl;
        return true;
    } catch (const std::exception &) {
        return false;
    }
}

} // namespace QEP
