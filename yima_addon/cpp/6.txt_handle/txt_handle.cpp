/*
 * @Author: Frank Liu 2548253579@qq.com
 * @Date: 2026-01-16 15:49:35
 * @LastEditors: Frank Liu 2548253579@qq.com
 * @LastEditTime: 2026-01-16 17:42:44
 * @FilePath: \bmptranslator\src\6.txt_handle\txt_handle.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "txt_handle.h"
#include "../encoding_utils.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

// 限制搜索窗口以换取速度
const size_t MAX_PATTERN_LEN = 50;  // 模式长度上限
const size_t MAX_LOOKAHEAD = 200;   // 向前搜索的范围限制

bool IsSequenceEqual(const std::vector<std::string>& lines, size_t s1, size_t s2, size_t len) {
    if (s2 + len > lines.size()) return false;
    for (size_t i = 0; i < len; ++i) {
        if (lines[s1 + i] != lines[s2 + i]) return false;
    }
    return true;
}

// 快速递归压缩：仅对当前位置进行局部最优匹配
void FastCompress(const std::vector<std::string>& lines, std::ofstream& outFile) {
    size_t i = 0;
    while (i < lines.size()) {
        size_t bestL = 0;
        size_t bestCount = 0;
        int maxSavings = 0;

        // 在窗口范围内寻找从当前位置 i 开始的最优循环
        size_t searchL = std::min(MAX_PATTERN_LEN, (lines.size() - i) / 2);
        for (size_t L = 1; L <= searchL; ++L) {
            size_t count = 1;
            while (i + (count + 1) * L <= lines.size() && IsSequenceEqual(lines, i, i + count * L, L)) {
                count++;
            }

            int savings = (int)((count - 1) * L - 2); // 收益计算
            if (savings > maxSavings) {
                maxSavings = savings;
                bestL = L;
                bestCount = count;
            }
        }

        if (maxSavings > 0) {
            outFile << "RS " << bestCount << "\n";
            // 对循环体进行递归压缩，以支持嵌套 RS/RE
            std::vector<std::string> body(lines.begin() + i, lines.begin() + i + bestL);
            FastCompress(body, outFile);
            outFile << "RE\n";
            i += bestCount * bestL;
        } else {
            outFile << lines[i] << "\n";
            i++;
        }
    }
}

YIMA_API int PostProcessTxt(const char* txt_input_dir, const char* txt_output_dir) {
    try {
        #ifdef _WIN32
        fs::path inputPath = fs::path(Utf8ToWide(txt_input_dir)) / "cmd_simple.txt";
        fs::path outputPath = fs::path(Utf8ToWide(txt_output_dir)) / "cmd_compressed.txt";
        #else
        fs::path inputPath = fs::path(txt_input_dir) / "cmd_simple.txt";
        fs::path outputPath = fs::path(txt_output_dir) / "cmd_compressed.txt";
        #endif
        if (!fs::exists(inputPath)) return -1;

        std::ifstream inFile(inputPath.string());
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(inFile, line)) {
            std::string trimmed = line;
            // 移除首尾空白
            trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
            trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);
            if (!trimmed.empty()) lines.push_back(trimmed);
        }
        inFile.close();

        std::ofstream outFile(outputPath.string());
        if (!outFile.is_open()) return -2;

        FastCompress(lines, outFile);

        outFile.close();
        return 0;
    } catch (...) {
        return -1;
    }
}