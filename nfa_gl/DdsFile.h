#pragma once

#include <cstdint>
#include <cstddef>
#define NOMINMAX

typedef unsigned long GLenum;

// glDataFormats
#define GL_BGR 0x80E0
#define GL_BGRA 0x80E1
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3

// glDataTypes
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367

namespace dds
{
    class DdsFile
    {
    public:
        DdsFile(void *data, std::size_t dataSize);
        DdsFile(const void *data, std::size_t dataSize);

        GLenum glDataFormat() const;
        GLenum glDataType() const;
        unsigned width() const;
        unsigned height() const;
        unsigned mipMapCount() const;
        std::size_t bytesPerPixel() const;
        const char *get(std::size_t &bytes) const;
        char *getMutable(std::size_t &bytes);

    private:
        void init(const void *data, std::size_t dataSize);

        const void *m_data;
        void *m_mutableData;
        std::size_t m_dataSize;

        GLenum m_dataFormat;
        GLenum m_dataType;
        std::size_t m_bytesPerPixel;

    };
}
