#include "txt_generator.h"
#include "../toml.hpp"
#include "../encoding_utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

// 辅助函数：解析 CSV，处理引号包裹的多行内容
std::vector<std::vector<std::string>> ParsePixelCmdCsv(const std::string& path) {
    std::vector<std::vector<std::string>> data;
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return data;

    unsigned char bom[3];
    file.read((char*)bom, 3);
    if (!(bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF)) file.seekg(0);

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    std::vector<std::string> row;
    std::string field;
    bool inQuotes = false;

    for (size_t i = 0; i < content.size(); ++i) {
        char c = content[i];
        if (c == '"') {
            if (inQuotes && i + 1 < content.size() && content[i + 1] == '"') {
                field += '"';
                i++;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == ',' && !inQuotes) {
            row.push_back(field);
            field.clear();
        } else if ((c == '\n' || c == '\r') && !inQuotes) {
            if (c == '\r' && i + 1 < content.size() && content[i + 1] == '\n') i++;
            row.push_back(field);
            if (!row.empty()) data.push_back(row);
            row.clear();
            field.clear();
        } else {
            field += c;
        }
    }
    return data;
}

// 辅助函数：修剪字符串首尾的空白字符和换行符
std::string TrimCmd(const std::string& s) {
    auto first = s.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    auto last = s.find_last_not_of(" \t\n\r");
    return s.substr(first, last - first + 1);
}

YIMA_API int GenerateRawTxt(const char* csv_input_dir, const char* txt_output_dir, const char* config_dir) {
    try {
        #ifdef _WIN32
        fs::path txtDir = fs::path(Utf8ToWide(txt_output_dir));
        fs::path csvInputDir = fs::path(Utf8ToWide(csv_input_dir));
        #else
        fs::path txtDir = fs::path(txt_output_dir);
        fs::path csvInputDir = fs::path(csv_input_dir);
        #endif
        
        if (!fs::exists(txtDir)) fs::create_directories(txtDir);
        
        // --- 1. 加载 head_tail_cmd.toml 配置 ---
        std::string head_cmd = "";
        std::string tail_cmd = "";
        #ifdef _WIN32
        fs::path configPath = fs::path(Utf8ToWide(config_dir)) / "head_tail_cmd.toml";
        #else
        fs::path configPath = fs::path(config_dir) / "head_tail_cmd.toml";
        #endif
        
        if (fs::exists(configPath)) {
            try {
                std::ifstream file(configPath, std::ios::binary);
                if (!file.is_open()) {
                    std::cerr << "[Step 5] Error: Cannot open head_tail_cmd.toml with ifstream" << std::endl;
                } else {
                    std::stringstream buffer;
                    buffer << file.rdbuf();
                    file.close();
                    auto tbl = toml::parse(buffer.str(), configPath.string());
                    if (auto section = tbl["head_tail_cmd"].as_table()) {
                        if (auto h = section->get("head")) head_cmd = TrimCmd(h->as_string()->get());
                        if (auto t = section->get("tail")) tail_cmd = TrimCmd(t->as_string()->get());
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "[Step 5] Exception loading head_tail_cmd.toml: " << e.what() << std::endl;
            }
        }

        fs::path csvPath = csvInputDir / "pixel_cmd.csv";
        if (!fs::exists(csvPath)) return -1;

        auto data = ParsePixelCmdCsv(csvPath.string());
        if (data.size() < 2) return -1;

        // --- 2. 同时打开两个输出文件 ---
        fs::path rawPath = txtDir / "cmd_raw.txt";
        fs::path simplePath = txtDir / "cmd_simple.txt";
        std::ofstream rawFile(rawPath.string(), std::ios::binary);     // 带注释
        std::ofstream simpleFile(simplePath.string(), std::ios::binary); // 纯指令
        if (!rawFile.is_open() || !simpleFile.is_open()) return -2;

        // --- 3. 写入头部命令 ---
        if (!head_cmd.empty()) {
            rawFile << "# [HEAD START]\n" << head_cmd << "\n# [HEAD END]\n\n";
            simpleFile << head_cmd << "\n";
        }

        std::vector<std::string> headers = data[0];

        // --- 4. 写入像素与控制数据指令 ---
        for (size_t i = 1; i < data.size(); ++i) {
            const auto& row = data[i];
            if (row.size() < 8) continue;

            std::string index = row[0];
            if (index.empty()) index = "CONTROL_LINE";

            for (size_t col = 1; col < 8; ++col) {
                std::string cleaned_cmd = TrimCmd(row[col]);
                if (cleaned_cmd.empty()) continue;

                std::string label = headers[col];
                std::string special_tag = "";
                if (col == 6) special_tag = "[LINE_SWITCH] ";
                if (col == 7) special_tag = "[SHAXIAN_SWITCH] ";

                // raw 文件写入注释和指令
                rawFile << "# INDEX: " << index << ", Source: " << special_tag << label << "\n";
                rawFile << cleaned_cmd << "\n";

                // simple 文件仅写入指令
                simpleFile << cleaned_cmd << "\n";
            }
        }

        // --- 5. 写入尾部命令 ---
        if (!tail_cmd.empty()) {
            rawFile << "\n# [TAIL START]\n" << tail_cmd << "\n# [TAIL END]\n";
            simpleFile << tail_cmd << "\n";
        }

        rawFile.close();
        simpleFile.close();
        return 0;
    } catch (...) {
        return -1;
    }
}