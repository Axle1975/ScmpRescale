#include "DdsFile.h"

#include <algorithm>
#include <stdexcept>

using namespace dds;

enum DDPF
{
    DDPF_ALPHAPIXELS = 0x000001, 
                            // Texture contains alpha data; dwRGBAlphaBitMask contains valid data.
    DDPF_ALPHA = 0x000002,  // Used in some older DDS files for alpha channel only uncompressed data 
                            // (dwRGBBitCount contains the alpha channel bitcount; dwABitMask contains 
                            // valid data)
    DDPF_FOURCC = 0x000004, // Texture contains compressed RGB data; dwFourCC contains valid data.
    DDPF_RGB = 0x000040,    // Texture contains uncompressed RGB data; dwRGBBitCount and the RGB masks 
                            // (dwRBitMask, dwGBitMask, dwBBitMask) contain valid data.
    DDPF_YUV = 0x000200,    // Used in some older DDS files for YUV uncompressed data (dwRGBBitCount 
                            // contains the YUV bit count; dwRBitMask contains the Y mask, dwGBitMask 
                            // contains the U mask, dwBBitMask contains the V mask)
    DDPF_LUMINANCE = 0x020000 
                            // Used in some older DDS files for single channel color uncompressed data 
                            // (dwRGBBitCount contains the luminance channel bit count; dwRBitMask 
                            // contains the channel mask). Can be combined with DDPF_ALPHAPIXELS for a 
                            // two channel DDS file.
};

enum DDSD
{
    /*
     * Note  When you write.dds files, you should set the DDSD_CAPS and DDSD_PIXELFORMAT flags, and for 
     * mipmapped textures you should also set the DDSD_MIPMAPCOUNT flag.However, when you read a.dds file, 
     * you should not rely on the DDSD_CAPS, DDSD_PIXELFORMAT, and DDSD_MIPMAPCOUNT flags being set because 
     * some writers of such a file might not set these flags.
     */
    DDSD_CAPS = 0x1,            //Required in every.dds file.
    DDSD_HEIGHT = 0x2,          //Required in every.dds file.
    DDSD_WIDTH = 0x4,           //Required in every.dds file.
    DDSD_PITCH = 0x8,           //Required when pitch is provided for an uncompressed texture.
    DDSD_PIXELFORMAT = 0x1000,  //Required in every.dds file.
    DDSD_MIPMAPCOUNT = 0x20000, //Required in a mipmapped texture.
    DDSD_LINEARSIZE = 0x80000,  //Required when pitch is provided for a compressed texture.
    DDSD_DEPTH = 0x800000       //Required in a depth texture.
};

enum DDSCAPS1
{
    DDSCAPS_COMPLEX = 0x8,      // Optional; must be used on any file that contains more than one surface 
                                // (a mipmap, a cubic environment map, or mipmapped volume texture).
    DDSCAPS_MIPMAP = 0x400000,  // Optional; should be used for a mipmap.
    DDSCAPS_TEXTURE = 0x1000    // Required
};

enum DDSCAPS2
{
    DDSCAPS2_CUBEMAP = 0x200,           // Required for a cube map.
    DDSCAPS2_CUBEMAP_POSITIVEX = 0x400, // Required when these surfaces are stored in a cube map.
    DDSCAPS2_CUBEMAP_NEGATIVEX = 0x800, // Required when these surfaces are stored in a cube map.
    DDSCAPS2_CUBEMAP_POSITIVEY = 0x1000,// Required when these surfaces are stored in a cube map.
    DDSCAPS2_CUBEMAP_NEGATIVEY = 0x2000,// Required when these surfaces are stored in a cube map.
    DDSCAPS2_CUBEMAP_POSITIVEZ = 0x4000,// Required when these surfaces are stored in a cube map.
    DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x8000,// Required when these surfaces are stored in a cube map.
    DDSCAPS2_VOLUME = 0x200000,         // Required for a volume texture.
};


struct DdsPixelFormat
{
    std::uint32_t size;     // Structure size; set to 32 (bytes).
    std::uint32_t flags;    // DDPF
    std::uint32_t fourCC;   // Four-character codes for specifying compressed or custom formats. Possible 
                            // values include: DXT1, DXT2, DXT3, DXT4, or DXT5. A FourCC of DX10 indicates 
                            // the prescense of the DDS_HEADER_DXT10 extended header, and the dxgiFormat 
                            // member of that structure indicates the true format. When using a four-character 
                            // code, dwFlags must include DDPF_FOURCC.
    std::uint32_t rgbBitCount; 
                            // Number of bits in an RGB(possibly including alpha) format.Valid when dwFlags 
                            // includes DDPF_RGB, DDPF_LUMINANCE, or DDPF_YUV.
    std::uint32_t rBitMask; // Red (or lumiannce or Y) mask for reading color data. For instance, given the 
                            // A8R8G8B8 format, the red mask would be 0x00ff0000.
    std::uint32_t gBitMask; // Green (or U) mask for reading color data. For instance, given the A8R8G8B8 
                            // format, the green mask would be 0x0000ff00.
    std::uint32_t bBitMask; // Blue (or V) mask for reading color data. For instance, given the A8R8G8B8 format, 
                            // the blue mask would be 0x000000ff.
    std::uint32_t aBitMask; // Alpha mask for reading alpha data. dwFlags must include DDPF_ALPHAPIXELS or 
                            // DDPF_ALPHA. For instance, given the A8R8G8B8 format, the alpha mask would be 
                            // 0xff000000.
};

struct DdsHeader
{
    std::uint32_t size;     // Size of structure. This member must be set to 124.
    std::uint32_t flags;    // DDSD
    std::uint32_t height;   // Surface height (in pixels).
    std::uint32_t width;    // Surface width (in pixels).
    std::uint32_t pitchOrLinearSize;
                            // The pitch or number of bytes per scan line in an uncompressed texture; the total 
                            // number of bytes in the top level texture for a compressed texture. For information 
                            // about how to compute the pitch, see the DDS File Layout section of the Programming 
                            // Guide for DDS.
    std::uint32_t depth;    // Depth of a volume texture (in pixels), otherwise unused. 
    std::uint32_t mipMapCount;
                            // Number of mipmap levels, otherwise unused.
    std::uint32_t reserved[11];
                            // Unused.

    DdsPixelFormat format;

    std::uint32_t caps1;    // DDSCAPS1
    std::uint32_t caps2;    // DDSCAPS2
    std::uint32_t caps3;    // unused
    std::uint32_t caps4;    // unused
    std::uint32_t reserved2;// unused
};


#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3

DdsFile::DdsFile(void *data, std::size_t dataSize):
    m_dataSize(dataSize),
    m_dataFormat(0u),
    m_dataType(0u),
    m_bytesPerPixel(0u)
{
    init(data, dataSize);
    m_mutableData = const_cast<void*>(m_data);
}

DdsFile::DdsFile(const void *data, std::size_t dataSize) :
m_dataSize(dataSize),
m_dataFormat(0u),
m_dataType(0u),
m_bytesPerPixel(0u)
{
    init(data, dataSize);
    m_mutableData = NULL;
}


void DdsFile::init(const void *data, std::size_t dataSize)
{
    if (dataSize < 4u + sizeof(DdsHeader))
    {
        throw std::runtime_error("DdsFile: data file not large enough to container a header!");
    }

    if (strncmp((const char*)data, "DDS ", 4))
    {
        throw std::runtime_error("DdsFile: not a dds file!");
    }
    m_data = (const void*)((const char*)data + 4u);

    const DdsHeader *header = (const DdsHeader*)m_data;
    if (header->size != sizeof(DdsHeader))
    {
        throw std::runtime_error("DdsFile: unexpected DdsHeader.size!");
    }

    if(header->format.size != sizeof(DdsPixelFormat))
    {
        throw std::runtime_error("DdsFile: unexpected DdsPixelFormat.size!");
    }
    else
    {
        const DdsPixelFormat &fmt = header->format;
        if (header->flags & DDPF_FOURCC && header->format.fourCC)
        {
            if (!strncmp((const char*)&fmt.fourCC, "DXT1", 4))
            {
                m_dataFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
            }
            else if (!strncmp((const char*)&fmt.fourCC, "DXT3", 4))
            {
                m_dataFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            }
            else if (!strncmp((const char*)&fmt.fourCC, "DXT5", 5))
            {
                m_dataFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            }
            else
            {
                throw std::runtime_error("DdsFile: unsupported FOURCC dds format!");
            }

            m_bytesPerPixel = (dataSize - header->size - 4) / header->width / header->height;
            if (m_bytesPerPixel * header->width * header->height + header->size + 4 != dataSize)
            {
                throw std::runtime_error("DdsFile: unable to ascertain pixel format");
            }
        }
        else if (fmt.rgbBitCount == 32 && fmt.aBitMask == 0xff000000 && fmt.rBitMask == 0xff0000 && fmt.gBitMask == 0xff00 && fmt.bBitMask == 0xff)
        {
            m_bytesPerPixel = 4;
            m_dataFormat = GL_BGRA;
            m_dataType = GL_UNSIGNED_INT_8_8_8_8_REV;
        }
        else if (fmt.rgbBitCount == 24 && fmt.rBitMask == 0xff0000 && fmt.gBitMask == 0xff00 && fmt.bBitMask == 0xff)
        {
            m_bytesPerPixel = 3;
            m_dataFormat = GL_BGR;
            m_dataType = GL_UNSIGNED_BYTE;
        }
        else
        {
            throw std::runtime_error("DdsFile: unsupported dds format!");
        }
    }
}


GLenum DdsFile::glDataFormat() const
{
    return m_dataFormat;
}

GLenum DdsFile::glDataType() const
{
    return m_dataType;
}

unsigned DdsFile::width() const
{
    const DdsHeader *header = (DdsHeader*)m_data;
    return header->width;
}

unsigned DdsFile::height() const
{
    const DdsHeader *header = (DdsHeader*)m_data;
    return header->height;
}

unsigned DdsFile::mipMapCount() const
{
    const DdsHeader *header = (DdsHeader*)m_data;
    return std::max(header->mipMapCount, 1u);
}

std::size_t DdsFile::bytesPerPixel() const
{
    return m_bytesPerPixel;
}

char *DdsFile::getMutable(std::size_t &bytes)
{
    if (!m_mutableData)
    {
        throw std::runtime_error("Attempt to get mutable data from const DdsFile");
    }

    const DdsHeader *header = (const DdsHeader*) m_mutableData;
    bytes = m_dataSize - header->size;
    char *image = (char*)m_mutableData + header->size;
    return image;
}

const char *DdsFile::get(std::size_t &bytes) const
{
    const DdsHeader *header = (DdsHeader*)m_data;
    bytes = m_dataSize - header->size;
    const char *image = (const char*)m_data + header->size;
    return image;
}
