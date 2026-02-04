/*
 * @Author: Frank Liu 2548253579@qq.com
 * @Date: 2026-01-22
 * @LastEditors: Frank Liu 2548253579@qq.com
 * @LastEditTime: 2026-01-22 16:00:00
 * @FilePath: \bmptranslator\yima.h
 * @Description: BMP 图像转换处理系统 DLL 导出函数定义
 */

#ifndef YIMA_H
#define YIMA_H

#include "yima_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BMP 图像转换处理主入口函数
 * @param dll_path DLL 文件夹路径 (如: "./dll")
 * @param config_path 配置文件夹路径 (如: "./config_files")
 * @param input_path 输入文件夹路径 (BMP 文件所在目录，如: "./input_bmp")
 * @param output_path 输出文件夹路径 (基础输出目录，如: "./output")
 * @return 成功返回 0，失败返回负数:
 *         0   : 处理成功
 *         -1  : DLL 加载失败
 *         -2  : 函数地址获取失败
 *         -3  : 标准异常发生
 *         -4  : 未知错误
 * 
 * @details
 * 函数会依次执行以下 6 个阶段:
 *   1. BMP 提取: 将输入文件夹中的 BMP 文件转换为 TOML 格式
 *   2. TOML 合并: 合并所有 TOML 文件为 combined.toml
 *   3. 数据 CSV: 生成像素数据 CSV
 *   4. 命令 CSV: 生成针脚命令 CSV
 *   5. TXT 生成: 生成最终的指令文本流
 *   6. TXT 处理: 压缩和后处理指令流
 * 
 * @example
 *   // 使用默认路径
 *   int result = ProcessBmpTranslation("./dll", "./config_files", 
 *                                      "./input_bmp", "./output");
 *   if (result != 0) {
 *       printf("Error: %d\\n", result);
 *   }
 */
YIMA_API int ProcessBmpTranslation(const char* dll_path, const char* config_path,
                                      const char* input_path, const char* output_path);

#ifdef __cplusplus
}
#endif

#endif  // YIMA_H
