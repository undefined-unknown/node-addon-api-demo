#ifndef CMD_CSV_HANDLE_H
#define CMD_CSV_HANDLE_H

#include "../yima_common.h"

extern "C" {
    /**
     * @brief 根据 combined.toml 和配置文件生成命令 CSV
     * @param toml_input_dir TOML 文件夹路径
     * @param csv_output_dir CSV 输出文件夹路径
     * @param config_dir 配置文件夹路径
     * @return 0: 成功, -1: 文件读取失败
     */
    YIMA_API int GenerateCmdCsv(const char* toml_input_dir, const char* csv_output_dir, const char* config_dir);
}
#endif