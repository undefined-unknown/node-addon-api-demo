#ifndef BMP_EXTRACT_H
#define BMP_EXTRACT_H

#include "../yima_common.h"

extern "C" {
    // 处理 BMP 并返回 TOML 字符串
    YIMA_API char* process_bmp_to_toml(const char* file_path);
    
    // 专门释放由 DLL 申请的内存
    YIMA_API void free_toml_buffer(char* ptr);
}

#endif