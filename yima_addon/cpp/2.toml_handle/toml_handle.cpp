#include "toml_handle.h"
#define TOML_ENABLE_FORMATTERS 1
#include "../toml.hpp"
#include "../encoding_utils.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <filesystem>

namespace fs = std::filesystem;

struct PixelGroup {
    std::string sema = "#000000";
    std::string shaxian = "#000000";
    std::string luola = "#000000";
    std::string dumu = "#000000";
    std::string direction = "#000000";
};

std::string GetStringFromNode(const toml::node* node) {
    if (!node) return "";
    if (auto s = node->as_string()) return s->get();
    if (auto i = node->as_integer()) return std::to_string(i->get());
    return "";
}

YIMA_API int CombineTomlFiles(const char* toml_input_dir, const char* config_dir) {
    try {
        std::vector<std::string> keys = { "sema", "shaxian", "luola", "dumu", "direction" };
        std::map<std::string, std::map<std::string, std::string>> colorMap;

        std::map<std::string, std::vector<std::string>> pixel_lists;
        std::map<int, std::map<int, PixelGroup>> dataGrid;
        int commonWidth = -1, commonHeight = -1;

        // 1. 读取单体文件
        for (const auto& key : keys) {
            #ifdef _WIN32
            fs::path fpath = fs::path(Utf8ToWide(toml_input_dir)) / (key + ".toml");
            #else
            fs::path fpath = fs::path(toml_input_dir) / (key + ".toml");
            #endif
            if (!fs::exists(fpath)) continue;
            std::string fname = fpath.string();
            std::cout << "[TOML Load] Processing: " << fname << std::endl;
            std::ifstream file(fpath, std::ios::binary);
            if (!file.is_open()) {
                std::cerr << "[TOML Load] Error: Cannot open file with ifstream: " << fname << std::endl;
                continue;
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            file.close();
            auto tbl = toml::parse(buffer.str(), fname);
            commonWidth = (int)tbl["width"].as_integer()->get();
            commonHeight = (int)tbl["height"].as_integer()->get();
            std::cout << "[TOML Load] Width=" << commonWidth << ", Height=" << commonHeight << std::endl;

            for (auto&& [k, node] : tbl) {
                if (std::string(k.str()).find("pixels") != std::string::npos && node.is_array()) {
                    auto p_arr = node.as_array();
                    for (auto&& p_node : *p_arr) pixel_lists[key].push_back(GetStringFromNode(&p_node));
                }
            }

            if (auto d_arr = tbl["data"].as_array()) {
                for (auto&& row_node : *d_arr) {
                    auto row = row_node.as_array();
                    int x = (int)row->get(0)->as_integer()->get();
                    int y = (int)row->get(1)->as_integer()->get();
                    std::string color = (row->get(2)->is_integer()) ? 
                        pixel_lists[key][(size_t)row->get(2)->as_integer()->get()] : GetStringFromNode(row->get(2));
                    if (key == "sema") dataGrid[y][x].sema = color;
                    else if (key == "shaxian") dataGrid[y][x].shaxian = color;
                    else if (key == "luola") dataGrid[y][x].luola = color;
                    else if (key == "dumu") dataGrid[y][x].dumu = color;
                    else if (key == "direction") dataGrid[y][x].direction = color;
                }
            }
        }

        // 2. 加载配置
        #ifdef _WIN32
        fs::path colorPath = fs::path(Utf8ToWide(config_dir)) / "color_to_number.toml";
        #else
        fs::path colorPath = fs::path(config_dir) / "color_to_number.toml";
        #endif
        std::cout << "[Config] Looking for: " << colorPath.string() << " - Exists: " << (fs::exists(colorPath) ? "YES" : "NO") << std::endl;
        if (fs::exists(colorPath)) {
            try {
                std::ifstream file(colorPath, std::ios::binary);
                if (!file.is_open()) {
                    std::cerr << "[Config] Failed to open file with ifstream" << std::endl;
                    throw std::runtime_error("Cannot open file with ifstream");
                }
                std::stringstream buffer;
                buffer << file.rdbuf();
                file.close();
                auto configTbl = toml::parse(buffer.str(), colorPath.string());
                for (const auto& key : keys) {
                    if (auto section = configTbl[key].as_table()) {
                        for (auto&& [k, value] : *section) {
                            colorMap[key][std::string(k.str())] = GetStringFromNode(&value);
                        }
                    }
                }
                std::cout << "[Config] Successfully loaded color_to_number.toml" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[Config] Error loading color_to_number.toml: " << e.what() << std::endl;
                throw;
            }
        } else {
            std::cerr << "[Config] Warning: color_to_number.toml not found" << std::endl;
        }



        // 3. 写入 combined.toml
        #ifdef _WIN32
        fs::path combinedPath = fs::path(Utf8ToWide(toml_input_dir)) / "combined.toml";
        #else
        fs::path combinedPath = fs::path(toml_input_dir) / "combined.toml";
        #endif
        std::ofstream out(combinedPath.string());
        out << "width = " << commonWidth << "\nheight = " << commonHeight << "\n";
        out << "shaxian_types = " << pixel_lists["shaxian"].size() << "\n\ndata = [\n";
        
        auto getT = [&](const std::string& key, const std::string& color) {
            return (colorMap.count(key) && colorMap[key].count(color)) ? colorMap[key][color] : "0";
        };

        for (int y = 1; y <= commonHeight; ++y) {
            out << "  ";
            for (int x = 1; x <= commonWidth; ++x) {
                const auto& c = dataGrid[y][x];
                out << "[" << x << "," << y << ",\"" << getT("sema", c.sema) << "\",\"" << getT("shaxian", c.shaxian) << "\",\"" 
                    << getT("luola", c.luola) << "\",\"" << getT("dumu", c.dumu) << "\",\"" 
                    << getT("direction", c.direction) << "\"]";
                if (!(y == commonHeight && x == commonWidth)) out << ", ";
            }
            out << "\n";
        }
        out << "]";
        std::cout << "[CombineTomlFiles] Successfully wrote combined.toml" << std::endl;
        return 0;
    } catch (const std::exception& e) { 
        std::cerr << "[CombineTomlFiles] Exception: " << e.what() << std::endl;
        return -3;
    } catch (...) { 
        std::cerr << "[CombineTomlFiles] Unknown exception" << std::endl;
        return -3;
    }
}