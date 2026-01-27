#ifndef TXT_GENERATOR_H
#define TXT_GENERATOR_H

#include "../yima_common.h"

extern "C" {
    /**
     * @brief 基于 CSV 生成原始指令 TXT
     * @param csv_input_dir CSV 输入文件夹路径
     * @param txt_output_dir TXT 输出文件夹路径
     * @param config_dir 配置文件夹路径
     * @return 0: 成功, -1: 文件读取失败, -2: 文件打开失败
     */
    YIMA_API int GenerateRawTxt(const char* csv_input_dir, const char* txt_output_dir, const char* config_dir);
}

#endif