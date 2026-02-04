{
  "targets": [
    {
      "target_name": "yima_addon",
      "sources": [
        "cpp/yima.cpp",
        "cpp/1.bmp_extract/bmp_extract.cpp",
        "cpp/2.toml_handle/toml_handle.cpp",
        "cpp/3.data_csv_handle/data_csv_handle.cpp",
        "cpp/4.cmd_csv_handle/cmd_csv_handle.cpp",
        "cpp/5.txt_generator/txt_generator.cpp",
        "cpp/6.txt_handle/txt_handle.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        ".",
        "./cpp/1.bmp_extract",
        "./cpp/2.toml_handle",
        "./cpp/3.data_csv_handle",
        "./cpp/4.cmd_csv_handle",
        "./cpp/5.txt_generator",
        "./cpp/6.txt_handle"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "defines": [
        "NAPI_CPP_EXCEPTIONS",
        "YIMA_EXPORTS"
      ],
      "msvs_settings": {
        "VCCLCompilerTool": { "ExceptionHandling": 1, "AdditionalOptions": [ "/std:c++17" ] }
      },
      "xcode_settings": {
        "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
        "CLANG_CXX_LANGUAGE_STANDARD": "c++17"
      }
    }
  ]
}
