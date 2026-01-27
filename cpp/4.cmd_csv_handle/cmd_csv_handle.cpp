#include "cmd_csv_handle.h"
#include "../toml.hpp"
#include <fstream>
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
        fs::path combinedPath = fs::path(toml_input_dir) / "combined.toml";
        if (!fs::exists(combinedPath)) return -1;

        // 存储所有映射表
        std::map<std::string, std::map<std::string, std::string>> maps;

        // 统一 Lambda 名称为 load_config
        auto load_config = [&](std::string p, std::string s, std::string key) {
            if (fs::exists(p)) {
                try {
                    auto tbl = toml::parse_file(p);
                    if (auto sect = tbl[s].as_table()) {
                        for (auto&& [k, v] : *sect) { 
                            maps[key][std::string(k.str())] = GetV(&v); 
                        }
                    }
                } catch (...) { std::cerr << "Parse error in: " << p << std::endl; }
            }
        };

        // 加载所有配置文件
        fs::path cfgDir = fs::path(config_dir);
        load_config((cfgDir / "dumu_to_cmd.toml").string(), "dumu", "dumu");
        load_config((cfgDir / "pre_action_to_cmd.toml").string(), "pre_action", "pre");
        load_config((cfgDir / "post_action_to_cmd.toml").string(), "post_action", "post");
        load_config((cfgDir / "sema_to_cmd.toml").string(), "sema", "sema");
        load_config((cfgDir / "luola_to_cmd.toml").string(), "luola", "luola");
        load_config((cfgDir / "line_switch_to_cmd.toml").string(), "line_switch", "ls");
        load_config((cfgDir / "shaxian_switch_to_cmd.toml").string(), "shaxian_switch", "ss");

        // 解析 combined.toml
        auto config = toml::parse_file(combinedPath.string());
        int width = (int)config["width"].as_integer()->get();
        int height = (int)config["height"].as_integer()->get();
        auto data_arr = config["data"].as_array();

        struct PD { std::string sema, shaxian, luola, dumu, zhenban, sign; };
        std::map<int, std::map<int, PD>> grid;
        for (auto&& r_n : *data_arr) {
            auto r = r_n.as_array();
            grid[(int)r->get(1)->as_integer()->get()][(int)r->get(0)->as_integer()->get()] = 
                { GetV(r->get(2)), GetV(r->get(3)), GetV(r->get(4)), GetV(r->get(5)), GetV(r->get(6)), GetV(r->get(7)) };
        }

        std::ofstream csv(fs::path(csv_output_dir) / "pixel_cmd.csv");
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
        return 0;
    } catch (...) { return -1; }
}