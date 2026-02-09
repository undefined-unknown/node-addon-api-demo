#ifndef TOML_HANDLE_H
#define TOML_HANDLE_H

#include "../yima_common.h"
#include <string>

extern "C" {
    /**
     * @brief 合并指定的四个 TOML 文件
     * @param toml_input_dir 输入 TOML 文件夹路径
     * @param csv_output_dir CSV 输出文件夹路径
     * @param config_dir 配置文件夹路径
     * @return 0: 成功, -1: 文件读取失败, -2: 宽高不一致, -3: 数据格式错误
     */
    YIMA_API int CombineTomlFiles(const char* toml_input_dir, const char* config_dir);
}

#endif // TOML_HANDLE_H