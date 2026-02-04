#ifndef TXT_HANDLE_H
#define TXT_HANDLE_H

#include "../yima_common.h"

extern "C" {
    /**
     * @brief 后处理 TXT 文件，执行循环压缩
     * @param txt_input_dir TXT 输入文件夹路径
     * @param txt_output_dir TXT 输出文件夹路径
     * @return 0: 成功, -1: 文件读取失败, -2: 文件打开失败
     */
    YIMA_API int PostProcessTxt(const char* txt_input_dir, const char* txt_output_dir);
}

#endif