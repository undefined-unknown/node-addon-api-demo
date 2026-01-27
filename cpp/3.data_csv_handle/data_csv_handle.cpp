/*
 * @Author: Frank Liu 2548253579@qq.com
 * @Date: 2026-01-15 10:43:49
 * @LastEditors: Frank Liu 2548253579@qq.com
 * @LastEditTime: 2026-01-16 17:41:30
 * @FilePath: \bmptranslator\src\3.data_csv_handle\data_csv_handle.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "data_csv_handle.h"
#include "../toml.hpp"
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

struct FullPixelData {
    std::string sema, shaxian, luola, dumu, zhenban, sign;
};

std::string GetValue(const toml::node* n) {
    if (!n) return "";
    if (auto s = n->as_string()) return std::string(s->get());
    if (auto i = n->as_integer()) return std::to_string(i->get());
    return "";
}

YIMA_API int GenerateDataCsv(const char* toml_input_dir, const char* csv_output_dir) {
    try {
        fs::path csvDir = fs::path(csv_output_dir);
        if (!fs::exists(csvDir)) fs::create_directories(csvDir);
        
        fs::path combinedPath = fs::path(toml_input_dir) / "combined.toml";
        if (!fs::exists(combinedPath)) return -1;

        auto config = toml::parse_file(combinedPath.string());
        int width = (int)config["width"].as_integer()->get();
        int height = (int)config["height"].as_integer()->get();
        auto data_arr = config["data"].as_array();

        std::map<int, std::map<int, FullPixelData>> grid;
        for (auto&& row_node : *data_arr) {
            auto row = row_node.as_array();
            int x = (int)row->get(0)->as_integer()->get();
            int y = (int)row->get(1)->as_integer()->get();
            grid[y][x] = { GetValue(row->get(2)), GetValue(row->get(3)), GetValue(row->get(4)), 
                           GetValue(row->get(5)), GetValue(row->get(6)), GetValue(row->get(7)) };
        }

        std::ofstream csv(fs::path(csv_output_dir) / "pixel_data.csv");
        const unsigned char BOM[] = {0xEF, 0xBB, 0xBF};
        csv.write((const char*)BOM, sizeof(BOM));
        csv << "INDEX,X,Y,SEMA,SHAXIAN,LUOLA,DUMU,ZHENBAN,SIGN,PRE_ACTION,POST_ACTION,CMD\n";

        int idx = 1;
        std::string last_shaxian = "", last_zhenban = "1", last_sign = "+";

        for (int y = 1; y <= height; ++y) {
            std::vector<int> x_order;
            if (y % 2 != 0) { for (int x = width; x >= 1; --x) x_order.push_back(x); }
            else { for (int x = 1; x <= width; ++x) x_order.push_back(x); }

            for (int x : x_order) {
                const auto& d = grid[y][x];
                if (!last_shaxian.empty() && d.shaxian != last_shaxian) {
                    csv << ",,,," << last_sign + last_shaxian + d.shaxian << ",,,," << last_sign << ",,,shaxian_switch\n";
                }
                std::string pa = last_sign + last_zhenban + d.zhenban;
                csv << idx++ << "," << x << "," << y << "," << d.sign + d.sema << "," << d.shaxian << "," 
                    << d.luola << "," << d.dumu << "," << d.zhenban << "," << d.sign << "," << pa << "," << pa << ",\n";
                last_shaxian = d.shaxian; last_zhenban = d.zhenban; last_sign = d.sign;
            }
            if (y < height) {
                std::string next_sx = ( (y+1) % 2 != 0 ) ? grid[y+1][width].shaxian : grid[y+1][1].shaxian;
                csv << ",,,," << last_sign + grid[y][x_order.back()].shaxian + next_sx << "," << grid[y][x_order.back()].luola << ",,," << last_sign << ",,,line_switch\n";
            }
        }
        return 0;
    } catch (...) { return -1; }
}