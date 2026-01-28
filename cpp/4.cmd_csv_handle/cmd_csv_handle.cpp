#include "cmd_csv_handle.h"
#include "../toml.hpp"
#include "../encoding_utils.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

// 辅助函数：安全提取字符串内容
std::string GetV(const toml::node* n) {
    if (!n) return "";
    if (auto s = n->as_string()) return std::string(s->get());
    if (auto i = n->as_integer()) return std::to_string(i->get());
    return "";
}

YIMA_API int GenerateCmdCsv(const char* toml_input_dir, const char* csv_output_dir, const char* config_dir) {
    try {
        std::cout << "[Step 4] Starting GenerateCmdCsv" << std::endl;
        
        #ifdef _WIN32
        fs::path combinedPath = fs::path(Utf8ToWide(toml_input_dir)) / "combined.toml";
        #else
        fs::path combinedPath = fs::path(toml_input_dir) / "combined.toml";
        #endif
        std::cout << "[Step 4] Looking for combined.toml: " << combinedPath.string() << " - Exists: " << (fs::exists(combinedPath) ? "YES" : "NO") << std::endl;
        if (!fs::exists(combinedPath)) {
            std::cerr << "[Step 4] Error: combined.toml not found" << std::endl;
            return -1;
        }

        // 存储所有映射表
        std::map<std::string, std::map<std::string, std::string>> maps;

        // 统一 Lambda 名称为 load_config
        auto load_config = [&](std::string p, std::string s, std::string key) {
            std::cout << "[Step 4] Loading config: " << p << " section: " << s << " key: " << key << std::endl;
            if (fs::exists(p)) {
                try {
                    std::ifstream file(p, std::ios::binary);
                    if (!file.is_open()) {
                        std::cerr << "[Step 4] Error: Cannot open " << p << " with ifstream" << std::endl;
                        return;
                    }
                    std::stringstream buffer;
                    buffer << file.rdbuf();
                    file.close();
                    auto tbl = toml::parse(buffer.str(), p);
                    if (auto sect = tbl[s].as_table()) {
                        for (auto&& [k, v] : *sect) { 
                            maps[key][std::string(k.str())] = GetV(&v);
                            std::cout << "[Step 4] Loaded mapping: " << key << "[" << std::string(k.str()) << "]" << std::endl;
                        }
                    }
                    std::cout << "[Step 4] Successfully loaded: " << p << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "[Step 4] Parse error in " << p << ": " << e.what() << std::endl;
                }
            } else {
                std::cerr << "[Step 4] Warning: File not found: " << p << std::endl;
            }
        };

        // 加载所有配置文件
        #ifdef _WIN32
        fs::path cfgDir = fs::path(Utf8ToWide(config_dir));
        #else
        fs::path cfgDir = fs::path(config_dir);
        #endif
        std::cout << "[Step 4] Config directory: " << cfgDir.string() << std::endl;
        load_config((cfgDir / "dumu_to_cmd.toml").string(), "dumu", "dumu");
        load_config((cfgDir / "pre_action_to_cmd.toml").string(), "pre_action", "pre");
        load_config((cfgDir / "post_action_to_cmd.toml").string(), "post_action", "post");
        load_config((cfgDir / "sema_to_cmd.toml").string(), "sema", "sema");
        load_config((cfgDir / "luola_to_cmd.toml").string(), "luola", "luola");
        load_config((cfgDir / "line_switch_to_cmd.toml").string(), "line_switch", "ls");
        load_config((cfgDir / "shaxian_switch_to_cmd.toml").string(), "shaxian_switch", "ss");

        // 解析 combined.toml
        std::cout << "[Step 4] Parsing combined.toml" << std::endl;
        std::ifstream file(combinedPath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[Step 4] Error: Cannot open combined.toml with ifstream" << std::endl;
            return -1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        auto config = toml::parse(buffer.str(), combinedPath.string());
        
        int width = (int)config["width"].as_integer()->get();
        int height = (int)config["height"].as_integer()->get();
        std::cout << "[Step 4] Parsed dimensions: width=" << width << ", height=" << height << std::endl;
        auto data_arr = config["data"].as_array();
        std::cout << "[Step 4] Data array size: " << data_arr->size() << std::endl;

        struct PD { std::string sema, shaxian, luola, dumu, zhenban, sign; };
        std::map<int, std::map<int, PD>> grid;
        for (auto&& r_n : *data_arr) {
            auto r = r_n.as_array();
            grid[(int)r->get(1)->as_integer()->get()][(int)r->get(0)->as_integer()->get()] = 
                { GetV(r->get(2)), GetV(r->get(3)), GetV(r->get(4)), GetV(r->get(5)), GetV(r->get(6)), GetV(r->get(7)) };
        }
        std::cout << "[Step 4] Grid populated successfully" << std::endl;

        #ifdef _WIN32
        fs::path csv_out_dir = fs::path(Utf8ToWide(csv_output_dir));
        #else
        fs::path csv_out_dir = fs::path(csv_output_dir);
        #endif
        
        std::cout << "[Step 4] Writing CSV to: " << (csv_out_dir / "pixel_cmd.csv").string() << std::endl;
        std::ofstream csv(csv_out_dir / "pixel_cmd.csv");
        if (!csv.is_open()) {
            std::cerr << "[Step 4] Error: Cannot open pixel_cmd.csv for writing" << std::endl;
            return -1;
        }
        const unsigned char BOM[] = {0xEF, 0xBB, 0xBF};
        csv.write((const char*)BOM, sizeof(BOM));
        // 表头：8 列结构
        csv << "INDEX,PRE_ACTION_CMD,DUMU_CMD,SEMA_CMD,POST_ACTION_CMD,LUOLA_CMD,LINE_SWITCH_CMD,SHAXIAN_SWITCH_CMD\n";

        int idx = 1;
        std::string last_shaxian = "", last_zhenban = "1", last_sign = "+";

        for (int y = 1; y <= height; ++y) {
            std::vector<int> x_order;
            if (y % 2 != 0) { for (int x = width; x >= 1; --x) x_order.push_back(x); }
            else { for (int x = 1; x <= width; ++x) x_order.push_back(x); }

            for (int x : x_order) {
                const auto& d = grid[y][x];
                
                // shaxian_switch 逻辑
                if (!last_shaxian.empty() && d.shaxian != last_shaxian) {
                    csv << ",,,,,,,\"" << maps["ss"][last_sign + last_shaxian + d.shaxian] << "\"\n";
                }
                
                std::string pa = last_sign + last_zhenban + d.zhenban;
                csv << idx++ << ",\"" << maps["pre"][pa] << "\",\"" << maps["dumu"][d.dumu] << "\",\"" 
                    << maps["sema"][d.sign + d.sema] << "\",\"" << maps["post"][pa] << "\",,,\n";
                
                last_shaxian = d.shaxian; last_zhenban = d.zhenban; last_sign = d.sign;
            }
            
            // line_switch 逻辑
            if (y < height) {
                std::string cur_luola = grid[y][x_order.back()].luola;
                std::string next_sx = ( (y+1) % 2 != 0 ) ? grid[y+1][width].shaxian : grid[y+1][1].shaxian;
                std::string ls_key = last_sign + grid[y][x_order.back()].shaxian + next_sx;
                csv << ",,,,,\"" << maps["luola"][cur_luola] << "\",\"" << maps["ls"][ls_key] << "\",\n";
            }
        }
        csv.close();
        std::cout << "[Step 4] Successfully generated pixel_cmd.csv" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[Step 4] Exception: " << e.what() << std::endl;
        return -4;
    } catch (...) {
        std::cerr << "[Step 4] Unknown exception" << std::endl;
        return -4;
    }
}