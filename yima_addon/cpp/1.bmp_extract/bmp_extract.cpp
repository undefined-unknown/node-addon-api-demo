#include "bmp_extract.h"
#include "../yima_common.h"
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <set>
#include <cstdint>
#include <cstring>
#include <iomanip>

#pragma pack(push, 2)
struct BmpFileHeader {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

struct BmpInfoHeader {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};

struct RgbQuad {
    uint8_t rgbBlue; uint8_t rgbGreen; uint8_t rgbRed; uint8_t rgbReserved;
};
#pragma pack(pop)

extern "C" {
    YIMA_API char* process_bmp_to_toml(const char* file_path) {
        std::ifstream file(file_path, std::ios::binary);
        if (!file) return nullptr;

        BmpFileHeader bmfh;
        BmpInfoHeader bmih;
        file.read(reinterpret_cast<char*>(&bmfh), sizeof(bmfh));
        file.read(reinterpret_cast<char*>(&bmih), sizeof(bmih));

        if (bmih.biBitCount != 8) return nullptr;

        int numColors = (bmih.biClrUsed == 0) ? 256 : bmih.biClrUsed;
        std::vector<RgbQuad> palette(numColors);
        file.read(reinterpret_cast<char*>(palette.data()), numColors * sizeof(RgbQuad));

        int32_t width = bmih.biWidth;
        int32_t height = (bmih.biHeight < 0) ? -bmih.biHeight : bmih.biHeight;
        file.seekg(bmfh.bfOffBits, std::ios::beg);

        int rowSize = ((width * 8 + 31) / 32) * 4;
        std::vector<uint8_t> rowData(rowSize);

        std::set<std::string> color_palette;
        std::stringstream data_ss;
        data_ss << "data = [\n";

        for (int y = 0; y < height; ++y) {
            file.read(reinterpret_cast<char*>(rowData.data()), rowSize);
            data_ss << "  "; 
            for (int x = 0; x < width; ++x) {
                uint8_t index = rowData[x];
                uint8_t r = palette[index].rgbRed;
                uint8_t g = palette[index].rgbGreen;
                uint8_t b = palette[index].rgbBlue;

                std::stringstream hex_ss;
                hex_ss << "#" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)r
                       << std::setw(2) << std::setfill('0') << (int)g
                       << std::setw(2) << std::setfill('0') << (int)b;
                std::string hex = hex_ss.str();
                color_palette.insert(hex);

                // --- 坐标逻辑：左下角原点，且全部坐标 + 1 ---
                int tx = x + 1; 
                int ty = y + 1; 

                data_ss << "[" << tx << "," << ty << ",\"" << hex << "\"]";
                if (!(y == height - 1 && x == width - 1)) data_ss << ", ";
            }
            data_ss << "\n"; 
        }
        data_ss << "]";

        std::stringstream final_toml;
        final_toml << "width = " << width << "\nheight = " << height << "\nall_pixels = [";
        for (auto it = color_palette.begin(); it != color_palette.end(); ++it) {
            final_toml << "\"" << *it << "\"" << (std::next(it) == color_palette.end() ? "" : ", ");
        }
        final_toml << "]\n\n" << data_ss.str();

        std::string res_str = final_toml.str();
        char* out = new char[res_str.size() + 1];
        std::copy(res_str.begin(), res_str.end(), out);
        out[res_str.size()] = '\0';
        return out;
    }

    YIMA_API void free_toml_buffer(char* ptr) {
        if (ptr) delete[] ptr;
    }
}