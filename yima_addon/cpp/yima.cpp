#include <napi.h>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include "encoding_utils.h"

namespace fs = std::filesystem;

// 引入各模块的头文件
#include "1.bmp_extract/bmp_extract.h"
#include "2.toml_handle/toml_handle.h"
#include "3.data_csv_handle/data_csv_handle.h"
#include "4.cmd_csv_handle/cmd_csv_handle.h"
#include "5.txt_generator/txt_generator.h"
#include "6.txt_handle/txt_handle.h"

namespace fs = std::filesystem;

// 辅助函数：确保目录存在
void ensure_directory_exists(const fs::path& p) {
    if (!fs::exists(p)) {
        fs::create_directories(p);
    }
}

// 包装 Step 1: 遍历目录处理 BMP
int extract_bmp_to_toml_dir(const std::string& input_dir, const std::string& output_dir) {
    try {
        fs::path input_path = CreatePathFromUtf8(input_dir);
        fs::path output_path = CreatePathFromUtf8(output_dir);
        
        ensure_directory_exists(output_path);
        bool found = false;
        if (fs::exists(input_path) && fs::is_directory(input_path)) {
             for (const auto& entry : fs::directory_iterator(input_path)) {
                if (entry.is_regular_file() && entry.path().extension() == ".bmp") {
                    found = true;
                    // std::cout << "Processing BMP: " << entry.path().string() << std::endl;
                    char* result = process_bmp_to_toml(entry.path().string().c_str());
                    if (result) {
                        fs::path out_path = output_path / entry.path().filename().replace_extension(".toml");
                        std::ofstream out(out_path);
                        if (out.is_open()) {
                            out << result;
                            out.close();
                        }
                        free_toml_buffer(result);
                    } else {
                        std::cerr << "Failed to process: " << entry.path() << std::endl;
                        return -1;
                    }
                }
            }
        } else {
            #ifdef _WIN32
            std::cerr << "Input directory not found: " << WideToUtf8(input_path.wstring()) << std::endl;
            #else
            std::cerr << "Input directory not found: " << input_dir << std::endl;
            #endif
            return -1;
        }

        if (!found) std::cout << "No .bmp files found in " << input_dir << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception in step 1: " << e.what() << std::endl;
        return -1;
    }
}

// Main logic
int ProcessBmpTranslation(const std::string& config_path, const std::string& input_path, const std::string& output_path) {
    try {
        // Create fs::path objects from UTF-8 strings with proper encoding handling
        fs::path input_dir = CreatePathFromUtf8(input_path);
        fs::path output_dir = CreatePathFromUtf8(output_path);
        fs::path config_dir = CreatePathFromUtf8(config_path);

        // 1. 定义中间路径
        fs::path toml_dir = output_dir / "toml";

        ensure_directory_exists(output_dir);
        ensure_directory_exists(toml_dir);

        // Convert paths to UTF-8 strings for passing to C-style functions
        std::string toml_dir_str = PathToUtf8String(toml_dir);
        std::string output_dir_str = PathToUtf8String(output_dir);
        std::string config_path_str = PathToUtf8String(config_dir);

        // Step 1: Extract BMP to TOML
        std::cout << "[Step 1] Extracting BMP to TOML..." << std::endl;
        if (extract_bmp_to_toml_dir(input_path, toml_dir_str) != 0) return -1;

        // Step 2: Combine TOML
        std::cout << "[Step 2] Combining TOML files..." << std::endl;
        if (CombineTomlFiles(toml_dir_str.c_str(), config_path_str.c_str()) != 0) return -2;

        // Step 3: Generate Data CSV
        std::cout << "[Step 3] Generating Data CSV..." << std::endl;
        if (GenerateDataCsv(toml_dir_str.c_str(), output_dir_str.c_str()) != 0) return -3;

        // Step 4: Generate Command CSV
        std::cout << "[Step 4] Generating Command CSV..." << std::endl;
        if (GenerateCmdCsv(toml_dir_str.c_str(), output_dir_str.c_str(), config_path_str.c_str()) != 0) return -4;

        // Step 5: Generate TXT
        std::cout << "[Step 5] Generating TXT from CSV..." << std::endl;
        if (GenerateRawTxt(output_dir_str.c_str(), output_dir_str.c_str(), config_path_str.c_str()) != 0) return -5;

        // Step 6: Finalize TXT
        std::cout << "[Step 6] Finalizing TXT handle..." << std::endl;
        if (PostProcessTxt(output_dir_str.c_str(), output_dir_str.c_str()) != 0) return -6;

        std::cout << "--- All steps completed successfully! ---" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return -100;
    }
}

// Helper function to convert UTF-8 string properly on Windows
std::string ConvertToUtf8(const std::string& utf8_from_js) {
    // JavaScript strings are already UTF-8, just return them directly
    return utf8_from_js;
}

// N-API Wrapper
Napi::Number ProcessWrapped(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 3) {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return Napi::Number::New(env, -1);
    }

    if (!info[0].IsString() || !info[1].IsString() || !info[2].IsString()) {
        Napi::TypeError::New(env, "Wrong arguments: expected (config_path, input_path, output_path)").ThrowAsJavaScriptException();
        return Napi::Number::New(env, -1);
    }

    // Get UTF-8 strings from JavaScript
    // These strings are in UTF-8 encoding and will be properly converted in ProcessBmpTranslation
    std::string config_path = info[0].As<Napi::String>().Utf8Value();
    std::string input_path = info[1].As<Napi::String>().Utf8Value();
    std::string output_path = info[2].As<Napi::String>().Utf8Value();

    int result = ProcessBmpTranslation(config_path, input_path, output_path);
    return Napi::Number::New(env, result);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set(Napi::String::New(env, "processBmpTranslation"), Napi::Function::New(env, ProcessWrapped));
    return exports;
}

NODE_API_MODULE(yima_addon, Init)
