#include "io.h"
#include "scmp.h"

#include "nfa_gl/DdsFile.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>


template<typename DataT>
static void ResizeImage(const DataT *im, DataT *om, int W0, int H0, int W, int H, bool lerp)
{
    float wscale = float(W) / float(W0);
    float hscale = float(H) / float(H0);

    for (int col = 0; col < W; ++col)
    {
        for (int row = 0; row < H; ++row)
        {
            if (lerp)
            {
                float sourceCol(float(col) / wscale);
                float sourceRow(float(row) / hscale);
                double sum = 0.0, sumWeights = 0.0;
                for (int c = int(sourceCol) -1; c<int(sourceCol) + 3; ++c)
                {
                    for (int r = int(sourceRow) -1; r<int(sourceRow) + 3; ++r)
                    {
                        if (c >= 0 && c < W0 && r >= 0 && r < H0)
                        {
                            double d = std::pow(sourceCol - double(c), 2.0) + std::pow(sourceRow - double(r), 2.0);
                            d = std::max(0.1, d);
                            sum += double(im[W0*r + c]) / d;
                            sumWeights += 1.0 / d;
                        }
                        om[W*row + col] = sumWeights > 0.0 ? DataT(sum / sumWeights) : DataT(0.0);
                    }
                }
            }
            else
            {
                int sourceCol(float(col) / wscale);
                int sourceRow(float(row) / hscale);
                om[W*row + col] = im[W0*sourceRow + sourceCol];
            }
        }
    }
}


template<typename DataT>
static void ImportImage(
    const DataT *im1, int W1, int H1,
    DataT *im2, int W2, int H2,
    int W0, int H0, bool overwrite)
{
    for (int col = 0u; col < W1; ++col)
    {
        for (int row = 0u; row < H1; ++row)
        {
            int destCol = col + W0;
            int destRow = row + H0;
            if (destCol < 0 || destCol >= W2 || destRow < 0 || destRow >= H2)
            {
                continue;
            }
            if (overwrite)
            {
                im2[W2*destRow + destCol] = im1[W1*row + col];
            }
            else
            {
                im2[W2*destRow + destCol] += im1[W1*row + col];
            }
        }
    }
}


static void ImportDds(
    const std::uint8_t *_srcDdsData, std::size_t srcBytes,
    std::uint8_t *_dstDdsData, std::size_t dstBytes,
    int srcW, int srcH, int destW, int destH,
    int column0, int row0, std::string debugName, bool lerp)
{
    dds::DdsFile srcDds(_srcDdsData, srcBytes);
    dds::DdsFile dstDds(_dstDdsData, dstBytes);

    if (srcDds.bytesPerPixel() != dstDds.bytesPerPixel() || srcDds.glDataFormat() != dstDds.glDataFormat() || srcDds.glDataType() != dstDds.glDataType())
    {
        throw std::runtime_error(debugName + ": dds data aren't in the same pixel format. cannot import");
    }

    // adjust column0,row0 to texture coordinates
    column0 = 0.5 + float(column0) / float(destW) * dstDds.width();
    row0 = 0.5 + float(row0) / float(destH) * dstDds.height();

    // scale source texture to fit into dst texture coordinates assuming src texture maps to srcW/H world coordinates and dst texture maps to destW/H world coordinates
    int srcWScaled = 0.5 + float(srcW) / float(destW) * float(dstDds.width());
    int srcHScaled = 0.5 + float(srcH) / float(destH) * float(dstDds.height());
    // actually just set up the buffer here, we'll do the resize in the coming switch statement
    std::vector<std::uint8_t> srcScaled(srcWScaled*srcHScaled*srcDds.bytesPerPixel());

    std::size_t imageBytes;
    switch (srcDds.bytesPerPixel())
    {
    case 1:
        ResizeImage<std::uint8_t>((const std::uint8_t*)srcDds.get(imageBytes), (std::uint8_t*)srcScaled.data(),
            srcDds.width(), srcDds.height(), srcWScaled, srcHScaled, lerp);
        ImportImage<std::uint8_t>(
            (std::uint8_t*)srcScaled.data(), srcWScaled, srcHScaled,
            (std::uint8_t*)dstDds.getMutable(imageBytes), dstDds.width(), dstDds.height(),
            column0, row0, true);
        break;

    case 2:
        ResizeImage<std::uint16_t>((const std::uint16_t*)srcDds.get(imageBytes), (std::uint16_t*)srcScaled.data(),
            srcDds.width(), srcDds.height(), srcWScaled, srcHScaled, lerp);
        ImportImage<std::uint16_t>(
            (std::uint16_t*)srcScaled.data(), srcWScaled, srcHScaled,
            (std::uint16_t*)dstDds.getMutable(imageBytes), dstDds.width(), dstDds.height(),
            column0, row0, true);
        break;

    case 4:
        ResizeImage<std::uint32_t>((const std::uint32_t*)srcDds.get(imageBytes), (std::uint32_t*)srcScaled.data(),
            srcDds.width(), srcDds.height(), srcWScaled, srcHScaled, lerp);
        ImportImage<std::uint32_t>(
            (std::uint32_t*)srcScaled.data(), srcWScaled, srcHScaled,
            (std::uint32_t*)dstDds.getMutable(imageBytes), dstDds.width(), dstDds.height(),
            column0, row0, true);
        break;

    case 8:
        ResizeImage<std::uint64_t>((const std::uint64_t*)srcDds.get(imageBytes), (std::uint64_t*)srcScaled.data(),
            srcDds.width(), srcDds.height(), srcWScaled, srcHScaled, lerp);
        ImportImage<std::uint64_t>(
            (std::uint64_t*)srcScaled.data(), srcWScaled, srcHScaled,
            (std::uint64_t*)dstDds.getMutable(imageBytes), dstDds.width(), dstDds.height(),
            column0, row0, true);
        break;

    default:
        throw std::runtime_error(debugName + ": dds data unexpected bytes per pixel");
    }
}


template<typename DataT>
static void GainImage(std::vector<DataT> &im, float gain)
{
    for (auto &pix : im)
    {
        pix *= gain;
    }
}


static bool isInBounds(float *pos, float xlow, float zlow, float xhigh, float zhigh)
{
    return (pos[0] >= xlow && pos[0] < xhigh && pos[2] >= zlow && pos[2] < zhigh);
}


template<typename T>
static std::vector< std::shared_ptr<T> > ImportItemsInRectangle(
    const std::vector<std::shared_ptr<T> > &items,
    const std::vector<std::shared_ptr<T> > &otherItems,
    int xlow, int zlow, int xhigh, int zhigh)
{
    std::vector<std::shared_ptr<T> > newItems;
    for (auto & itemPtr : items)
    {
        if (!isInBounds(itemPtr->position, xlow, zlow, xhigh, zhigh))
        {
            newItems.push_back(itemPtr);
        }
    }
    for (auto _itemPtr : otherItems)
    {
        std::shared_ptr<T> itemPtr(new T(*_itemPtr));
        itemPtr->position[0] += xlow;
        itemPtr->position[2] += zlow;
        if (isInBounds(itemPtr->position, xlow, zlow, xhigh, zhigh))
        {
            newItems.push_back(itemPtr);
        }
    }
    return newItems;
}


namespace nfa {
    namespace scmp {

        void VerifyStatus(std::istream & is, bool eofOk)
        {
            if (is.good())
            {
                return;
            }
            if (!eofOk && is.eof())
            {
                throw std::runtime_error("SCMP unexpected end-of-file");
            }
            if (is.fail())
            {
                throw std::runtime_error("SCMP logical i/o error");
            }
            if (is.bad())
            {
                throw std::runtime_error("SCMP read i/o error");
            }
        }


        WaveTexture::WaveTexture(std::istream &is)
        {
            Read(is, normalMovement);
            Read<std::string>(is, path);
        }

        void WaveTexture::Save(std::ostream &os)
        {
            Write(os, normalMovement);
            Write<std::string>(os, path);
        }


        WaterShaderProperties::WaterShaderProperties(std::istream &is)
        {
            Read(is, hasWater);
            if (hasWater == 1)
            {
                Read(is, elevation);
                Read(is, elevationDeep);
                Read(is, elevationAbyss);
            }
            else
            {
                is.ignore(12u);
                elevation = 17.5f;
                elevationDeep = 15.0f;
                elevationAbyss = 2.5f;
            }

            Read(is, surfaceColor);
            Read(is, colorLerp);
            Read(is, refractionScale);
            Read(is, fresnelBias);
            Read(is, fresnelPower);
            Read(is, unitReflection);
            Read(is, skyReflection);
            Read(is, sunShininess);
            Read(is, sunStrength);
            Read(is, sunDirection);
            Read(is, sunColor);
            Read(is, sunReflection);
            Read(is, sunGlow);
            Read(is, cubemapTexturePath);
            Read(is, waterRampTexturePath);

            float normalRepeats[4];
            Read(is, normalRepeats);

            for (int i = 0; i < 4; ++i)
            {
                waveTextures.push_back(std::make_shared<WaveTexture>(is));
            }

            for (int i = 0; i < 4; ++i)
            {
                waveTextures[i]->normalRepeat = normalRepeats[i];
            }
        }

        void WaterShaderProperties::Save(std::ostream &os)
        {
            Write(os, hasWater);
            Write(os, elevation);
            Write(os, elevationDeep);
            Write(os, elevationAbyss);

            Write(os, surfaceColor);
            Write(os, colorLerp);
            Write(os, refractionScale);
            Write(os, fresnelBias);
            Write(os, fresnelPower);
            Write(os, unitReflection);
            Write(os, skyReflection);
            Write(os, sunShininess);
            Write(os, sunStrength);
            Write(os, sunDirection);
            Write(os, sunColor);
            Write(os, sunReflection);
            Write(os, sunGlow);
            Write(os, cubemapTexturePath);
            Write(os, waterRampTexturePath);

            float normalRepeats[4];
            for (int i = 0; i < 4; ++i)
            {
                normalRepeats[i] = waveTextures[i]->normalRepeat;
            }
            Write(os, normalRepeats);

            for (int i = 0; i < 4; ++i)
            {
                waveTextures[i]->Save(os);
            }
        }

        void WaterShaderProperties::ScaleSize(float scaley)
        {
            elevation *= scaley;
            elevationDeep *= scaley;
            elevationAbyss *= scaley;
        }

        WaveGenerator::WaveGenerator(std::istream &is)
        {
            Read(is, textureName);
            Read(is, rampName);
            Read(is, position);
            Read(is, rotation);
            Read(is, velocity);
            Read(is, lifetimeFirst);
            Read(is, lifetimeSecond);
            Read(is, periodFirst);
            Read(is, periodSecond);
            Read(is, scaleFirst);
            Read(is, scaleSecond);
            Read(is, frameCount);
            Read(is, frameRateFirst);
            Read(is, frameRateSecond);
            Read(is, stripCount);
        }

        void WaveGenerator::Save(std::ostream &os)
        {
            Write(os, textureName);
            Write(os, rampName);
            Write(os, position);
            Write(os, rotation);
            Write(os, velocity);
            Write(os, lifetimeFirst);
            Write(os, lifetimeSecond);
            Write(os, periodFirst);
            Write(os, periodSecond);
            Write(os, scaleFirst);
            Write(os, scaleSecond);
            Write(os, frameCount);
            Write(os, frameRateFirst);
            Write(os, frameRateSecond);
            Write(os, stripCount);
        }

        void WaveGenerator::ScaleSize(float scalex, float scaley, float scalez)
        {
            position[0] *= scalex;
            position[1] *= scaley;
            position[2] *= scalez;
        }


        Stratum::Stratum(std::istream &is)
        {
            Read(is, albedoPath);
            Read(is, normalsPath);
            Read(is, albedoScale);
            Read(is, normalsScale);
        }

        void Stratum::Save(std::ostream &os)
        {
            Write(os, albedoPath);
            Write(os, normalsPath);
            Write(os, albedoScale);
            Write(os, normalsScale);
        }

        void Stratum::ScaleSize(float scale)
        {
            albedoScale *= scale;
            normalsScale *= scale;
        }


        void Stratum::LoadAlbedo(std::istream &is)
        {
            Read(is, albedoPath);
            Read(is, albedoScale);
        }

        void Stratum::SaveAlbedo(std::ostream &os)
        {
            Write(os, albedoPath);
            Write(os, albedoScale);
        }


        void Stratum::LoadNormal(std::istream &is)
        {
            Read(is, normalsPath);
            Read(is, normalsScale);
        }

        void Stratum::SaveNormal(std::ostream &os)
        {
            Write(os, normalsPath);
            Write(os, normalsScale);
        }


        Decal::Decal(std::istream &is)
        {
            std::uint32_t numberOfTextures;
            std::uint32_t texPathLength;

            Read(is, unknown);
            Read(is, type);
            Read(is, numberOfTextures);

            for (std::uint32_t i = 0; i<numberOfTextures; ++i)
            {
                Read(is, texPathLength);
                texPaths.push_back(std::string());
                ReadBuffer(is, texPaths.back(), texPathLength);
            }
 
            Read(is, scale);
            Read(is, position);
            Read(is, rotation);
            Read(is, cutOffLOD);
            Read(is, nearCutOffLOD);
            Read(is, ownerArmy);
        }

        void Decal::Save(std::ostream &os)
        {
            std::uint32_t numberOfTextures = texPaths.size();

            Write(os, unknown);
            Write(os, type);
            Write(os, numberOfTextures);

            for (std::uint32_t i = 0; i<numberOfTextures; ++i)
            {
                std::uint32_t texPathLength = texPaths[i].size();
                Write(os, texPathLength);
                WriteBuffer(os, texPaths[i], texPathLength);
            }

            Write(os, scale);
            Write(os, position);
            Write(os, rotation);
            Write(os, cutOffLOD);
            Write(os, nearCutOffLOD);
            Write(os, ownerArmy);
        }

        void Decal::ScaleSize(float scalex, float scaley, float scalez)
        {
            position[0] *= scalex;
            position[1] *= scaley;
            position[2] *= scalez;
            scale[0] *= scalex;
            scale[1] *= scaley;
            scale[2] *= scalez;
        }


        DecalGroup::DecalGroup(std::istream &is)
        {
            std::uint32_t groupCount;

            Read(is, id);
            Read(is, name);
            Read(is, groupCount);
            ReadBuffer(is, data, groupCount);
        }

        void DecalGroup::Save(std::ostream &os)
        {
            std::uint32_t groupCount = data.size();

            Write(os, id);
            Write(os, name);
            Write(os, groupCount);
            WriteBuffer(os, data, groupCount);
        }


        Prop::Prop(std::istream &is)
        {
            Read(is, blueprintPath);
            Read(is, position);
            Read(is, rotationX);
            Read(is, rotationY);
            Read(is, rotationZ);
            Read(is, unknown);
        }

        void Prop::Save(std::ostream &os)
        {
            Write(os, blueprintPath);
            Write(os, position);
            Write(os, rotationX);
            Write(os, rotationY);
            Write(os, rotationZ);
            Write(os, unknown);
        }

        void Prop::ScaleSize(float scalex, float scaley, float scalez)
        {
            position[0] *= scalex;
            position[1] *= scaley;
            position[2] *= scalez;
        }

        V59ObjectA::V59ObjectA(std::istream &is)
        {
            Read(is, p1_v3f);
            Read(is, p2_sf);
            Read(is, p3_sf);
            Read(is, p4_sf);
            Read(is, p5_si);         // read into LoadV59ObjectsA, Object* ((v2=a1)+56) {eg 16}
            Read(is, p6_si);         // read into LoadV59ObjectsA, Object* ((v2=a1)+60) {eg 6}
            Read(is, p7_sf);         // read into LoadV59ObjectsA, Object* ((v2=a1)+64) {eg 165.008347, 312.006897}
            Read(is, p8_v3f);       // read into LoadV59ObjectsA, Object* ((v2=a1)+68) { eg {0.648593724, 0.820468724, 0.839999974} or {0.899999976, 0.949999988, 0.969999969} }
            Read(is, p9_v3f);       // read into LoadV59ObjectsA, Object* ((v2=a1)+80)  { eg {0.180000007, 0.430000007, 0.550000012} or {0.000000000, 0.250000000, 0.500000000} }
            Read(is, p10_sf);         // read into LoadV59ObjectsA, Object* ((v2=a1)+120) { eg 0.1 }
            Read(is, p11_str1);        // read into LoadV59ObjectsA, Object* ((v2=a1)+124) { eg "/textures/environment/Decal_test_Albedo003.dds" or NULL }
            Read(is, p12_str2);        // read into LoadV59ObjectsA, Object* ((v2=a1)+152) { eg "/textures/environment/Decal_test_Glow003.dds" or NULL }

            Read(is, p13_count);   // eg 9 or 0
            for (std::uint32_t i = 0u; i < p13_count; ++i)
            {
                p14_vNBuffers40.resize(p14_vNBuffers40.size() + 1);
                ReadBuffer(is, p14_vNBuffers40.back(), 40u);
            }

            Read(is, p15_str3);
            Read(is, p16_str4);        // read into v2+220 using "copystring"
            Read(is, p17_str5);        // read into v2+248 using "copystring"
            Read(is, p18_sf);         // read into v2+276 { eg 1.8 }
            Read(is, p19_v3f);      // read into v2+280
            Read(is, p20_str6);        // read into v2+292

            Read(is, p21_si);         // ignored, probably always 4
            for (std::uint32_t i = 0u; i < p21_si; ++i)
            {
                p22_v4Buffers20.resize(p22_v4Buffers20.size() + 1);
                ReadBuffer(is, p22_v4Buffers20.back(), 20u);
            }
        }

        void V59ObjectA::Save(std::ostream &os)
        {
            Write(os, p1_v3f);
            Write(os, p2_sf);
            Write(os, p3_sf);
            Write(os, p4_sf);
            Write(os, p5_si);         // read into LoadV59ObjectsA, Object* ((v2=a1)+56) {eg 16}
            Write(os, p6_si);         // read into LoadV59ObjectsA, Object* ((v2=a1)+60) {eg 6}
            Write(os, p7_sf);         // read into LoadV59ObjectsA, Object* ((v2=a1)+64) {eg 165.008347, 312.006897}
            Write(os, p8_v3f);       // read into LoadV59ObjectsA, Object* ((v2=a1)+68) { eg {0.648593724, 0.820468724, 0.839999974} or {0.899999976, 0.949999988, 0.969999969} }
            Write(os, p9_v3f);       // read into LoadV59ObjectsA, Object* ((v2=a1)+80)  { eg {0.180000007, 0.430000007, 0.550000012} or {0.000000000, 0.250000000, 0.500000000} }
            Write(os, p10_sf);         // read into LoadV59ObjectsA, Object* ((v2=a1)+120) { eg 0.1 }
            Write(os, p11_str1);        // read into LoadV59ObjectsA, Object* ((v2=a1)+124) { eg "/textures/environment/Decal_test_Albedo003.dds" or NULL }
            Write(os, p12_str2);        // read into LoadV59ObjectsA, Object* ((v2=a1)+152) { eg "/textures/environment/Decal_test_Glow003.dds" or NULL }

            Write(os, p13_count);   // eg 9 or 0
            for (std::uint32_t i = 0u; i < p13_count; ++i)
            {
                WriteBuffer(os, p14_vNBuffers40[i], 40u);
            }

            Write(os, p15_str3);
            Write(os, p16_str4);        // read into v2+220 using "copystring"
            Write(os, p17_str5);        // read into v2+248 using "copystring"
            Write(os, p18_sf);         // read into v2+276 { eg 1.8 }
            Write(os, p19_v3f);      // read into v2+280
            Write(os, p20_str6);        // read into v2+292

            Write(os, p21_si);         // ignored, probably always 4
            for (std::uint32_t i = 0u; i < p21_si; ++i)
            {
                WriteBuffer(os, p22_v4Buffers20[i], 20u);
            }
        }


        V59ObjectB::V59ObjectB(std::istream &is, std::uint32_t versionMinor)
        {
            Read(is, p1_str1);
            Read(is, p2_str2);
            Read(is, p3_count);
            for (std::uint32_t i = 0u; i < p3_count; ++i)
            {
                p4_unk.resize(p4_unk.size()+1);
                Read(is, p4_unk.back());
            }
        }

        void V59ObjectB::Save(std::ostream &os)
        {
            Write(os, p1_str1);
            Write(os, p2_str2);
            Write(os, p3_count);
            for (std::uint32_t i = 0u; i < p3_count; ++i)
            {
                Write(os, p4_unk[i]);
            }
        }

        Scmp::Scmp(std::istream &is)
        {
            // header
            VerifyStatus(is, false);
            Read(is, magicMap1A);
            Read(is, versionMajor);

            if (magicMap1A != 0x1a70614d)
            {
                throw std::runtime_error("SCMP format error: magic number not present");
            }
            if (versionMajor != 2)
            {
                std::ostringstream s;
                s << "SCMP version error: unsupported SCMP version: " << versionMajor << '.' << versionMinor;
                throw std::runtime_error(s.str());
            }

            // preview
            VerifyStatus(is, false);
            {
                float width_float;
                float height_float;
                std::uint32_t alwaysZero;
                std::uint32_t bufferLength;

                Read(is, magicBeeffeed);
                Read(is, part1_version);
                Read(is, width_float);
                Read(is, height_float);
                Read(is, wstring1);
                Read(is, alwaysZero);
                Read(is, bufferLength);

                ReadBuffer(is, previewImageData, bufferLength);
            }

            // heightmap
            VerifyStatus(is, false);
            Read(is, versionMinor);
            if (versionMinor <= 0)
            {
                versionMinor = 56;
            }

            Read(is, width);
            Read(is, height);
            Read(is, heightScale);
            ReadBuffer(is, heightMapData, (height + 1)*(width + 1));
            if (versionMinor >= 54)
            {
                Read(is, unknownv54String);
            }

            // texture definition section
            VerifyStatus(is, false);
            Read(is, terrainShader);
            Read(is, backgroundTexturePath);
            Read(is, skyCubeMapTexturePath);

            if (versionMinor >= 55)
            {
                std::int32_t count;
                Read(is, count);
                for (std::int32_t i = 0; i < count; ++i)
                {
                    std::string profile, texturePath;
                    Read(is, profile);
                    Read(is, texturePath);
                    environmentCubeMapTextures[profile] = texturePath;
                }
            }
            else
            {
                std::string texturePath;
                Read(is, texturePath);
                environmentCubeMapTextures["<default>"] = texturePath;
            }

            Read(is, lightingMultiplier);
            Read(is, sunDirection);
            Read(is, sunAmbience);
            Read(is, sunColour);
            Read(is, shadowFillColour);
            Read(is, specularColour);
            Read(is, bloom);
            Read(is, fogColour);
            Read(is, fogStart);
            Read(is, fogEnd);

            // water
            VerifyStatus(is, false);
            {
                waterShaderProperties.reset(new WaterShaderProperties(is));

                std::uint32_t waveGeneratorCount;
                Read(is, waveGeneratorCount);

                for (std::uint32_t i = 0; i < waveGeneratorCount; ++i)
                {
                    waveGenerators.push_back(std::make_shared<WaveGenerator>(is));
                }
            }

            // minimap
            VerifyStatus(is, false);
            if (versionMinor >= 56)
            {
                Read(is, minimapContourInterval);
                Read(is, minimapDeepWaterColor);
                Read(is, minimapContourColor);
                Read(is, minimapShoreColor);
                Read(is, minimapLandStartColor);
                Read(is, minimapLandEndColor);
            }
            else
            {
                minimapContourInterval = 20;
                minimapDeepWaterColor = 0xff0e3eff;
                minimapContourColor = 0xff215cff;
                minimapShoreColor = 0xff4785ff;
                minimapLandStartColor = 0xff4c9d32;
                minimapLandEndColor = 0xffffffff;
            }

            if (versionMinor >= 57)
            {
                Read(is, unknownV57field);
            }

            // strata
            VerifyStatus(is, false);
            if (versionMinor < 54)
            {
                Read(is, tileset);
                Read(is, stratumCount);

                strata.resize(10u);
                std::uint32_t _stratumCount(stratumCount);
                for (int i = 0; i < 10u && _stratumCount>0; ++i)
                {
                    if (i < 5 || i >= 9)
                    {
                        strata[i].reset(new Stratum(is));
                        --_stratumCount;
                    }
                }
            }
            else
            {
                stratumCount = 10u;
                strata.resize(10u);
                for (int i = 0; i < 10; ++i)
                {
                    strata[i].reset(new Stratum());
                    strata[i]->LoadAlbedo(is);
                }
                for (int i = 0; i < 9; ++i)
                {
                    strata[i]->LoadNormal(is);
                }
            }

            // decals
            VerifyStatus(is, false);
            {
                std::uint32_t decalCount, decalGroupCount;

                Read(is, unknownPreDecals);

                Read(is, decalCount);
                for (std::uint32_t i = 0; i < decalCount; ++i)
                {
                    decals.push_back(std::make_shared<Decal>(is));
                }

                Read(is, decalGroupCount);
                for (std::uint32_t i = 0; i < decalGroupCount; ++i)
                {
                    decalGroups.push_back(std::make_shared<DecalGroup>(is));
                }
            }

            {
                Read(is, widthOther);
                Read(is, heightOther);
            }

            // normal map
            VerifyStatus(is, false);
            {
                std::uint32_t normalMapCount;
                std::uint32_t normalMapDataSize;
                Read(is, normalMapCount);

                for (std::uint32_t i = 0u; i < normalMapCount; ++i)
                {
                    Read(is, normalMapDataSize);
                    normalMapData.resize(normalMapData.size() + 1);
                    ReadBuffer(is, normalMapData.back(), normalMapDataSize);

                }
            }

            // texture map
            VerifyStatus(is, false);
            {
                std::uint32_t count = 2u;
                std::uint32_t size;
                if (versionMinor < 54)
                {
                    Read(is, count); // always 1
                }

                for (std::uint32_t i = 0u; i < count; ++i)
                {
                    Read(is, size);
                    strataLerpData.resize(strataLerpData.size() + 1);
                    ReadBuffer(is, strataLerpData.back(), size);
                }
            }

            // watermap
            VerifyStatus(is, false);
            {
                std::uint32_t count;    
                std::uint32_t size;
                Read(is, count);    // always 1

                for (std::uint32_t i = 0u; i < count; ++i)
                {
                    Read(is, size);
                    waterLerpData.resize(waterLerpData.size() + 1);
                    ReadBuffer(is, waterLerpData.back(), size);
                }
            }
            ReadBuffer(is, waterFoamMask, width*height / 4);
            ReadBuffer(is, waterFlatnessMask, width*height / 4);
            ReadBuffer(is, waterDepthBiasMask, width*height / 4);

            ReadBuffer(is, terrainTypeData, width*height);

            if (versionMinor < 53)
            {
                std::string dummy;
                Read(is, dummy);    // always null strings
                Read(is, dummy);
            }

            // v59 objects
            VerifyStatus(is, false);
            if (versionMinor >= 59)
            {
                v59ObjectA.reset(new V59ObjectA(is));

                std::uint32_t count;
                Read(is, count);
                for (std::uint32_t i = 0u; i < count; ++i)
                {
                    v59ObjectB.push_back(std::make_shared<V59ObjectB>(is, versionMinor));
                }
            }

            // Props
            VerifyStatus(is, false);
            {
                std::uint32_t propCount;
                Read(is, propCount);
                for (std::uint32_t i = 0; i < propCount; ++i)
                {
                    props.push_back(std::make_shared<Prop>(is));
                }
            }

            VerifyStatus(is, true);
        }

        void Scmp::Save(std::ostream &os)
        {
            // header
            Write(os, magicMap1A);
            Write(os, versionMajor);

            // preview
            {
                float width_float = width;
                float height_float = height;
                std::uint32_t alwaysZero = 0u;
                std::uint32_t bufferLength = previewImageData.size();

                Write(os, magicBeeffeed);
                Write(os, part1_version);
                Write(os, width_float);
                Write(os, height_float);
                Write(os, wstring1);
                Write(os, alwaysZero);
                Write(os, bufferLength);

                WriteBuffer(os, previewImageData, bufferLength);
            }

            // heightmap
            Write(os, versionMinor);
            Write(os, width);
            Write(os, height);
            Write(os, heightScale);
            WriteBuffer(os, heightMapData, (height + 1)*(width + 1));
            if (versionMinor >= 54)
            {
                Write(os, unknownv54String);
            }

            // texture definition section
            Write(os, terrainShader);
            Write(os, backgroundTexturePath);
            Write(os, skyCubeMapTexturePath);

            if (versionMinor >= 55)
            {
                std::int32_t count = environmentCubeMapTextures.size();
                Write(os, count);
                for (auto it = environmentCubeMapTextures.begin(); it != environmentCubeMapTextures.end(); ++it)
                {
                    std::string profile = it->first, texturePath = it->second;
                    Write(os, profile);
                    Write(os, texturePath);
                }
            }
            else
            {
                std::string texturePath = environmentCubeMapTextures["<default>"];
                Write(os, texturePath);
            }

            Write(os, lightingMultiplier);
            Write(os, sunDirection);
            Write(os, sunAmbience);
            Write(os, sunColour);
            Write(os, shadowFillColour);
            Write(os, specularColour);
            Write(os, bloom);
            Write(os, fogColour);
            Write(os, fogStart);
            Write(os, fogEnd);

            // water
            {
                waterShaderProperties->Save(os);
                std::uint32_t waveGeneratorCount = waveGenerators.size();
                Write(os, waveGeneratorCount);

                for (std::uint32_t i = 0; i < waveGeneratorCount; ++i)
                {
                    waveGenerators[i]->Save(os);
                }
            }

            // minimap
            if (versionMinor >= 56)
            {
                Write(os, minimapContourInterval);
                Write(os, minimapDeepWaterColor);
                Write(os, minimapContourColor);
                Write(os, minimapShoreColor);
                Write(os, minimapLandStartColor);
                Write(os, minimapLandEndColor);
            }

            if (versionMinor >= 57)
            {
                Write(os, unknownV57field);
            }

            // strata
            if (versionMinor < 54)
            {
                Write(os, tileset);
                Write(os, stratumCount);

                std::uint32_t _stratumCount(stratumCount);
                for (int i = 0; i < 10u && _stratumCount>0; ++i)
                {
                    if (i < 5 || i >= 9)
                    {
                        strata[i]->Save(os);
                        --_stratumCount;
                    }
                }
            }
            else
            {
                strata.resize(10u);
                for (int i = 0; i < 10; ++i)
                {
                    strata[i]->SaveAlbedo(os);
                }
                for (int i = 0; i < 9; ++i)
                {
                    strata[i]->SaveNormal(os);
                }
            }

            // decals
            {
                std::uint32_t decalCount = decals.size(), decalGroupCount = decalGroups.size();

                Write(os, unknownPreDecals);

                Write(os, decalCount);
                for (std::uint32_t i = 0; i < decalCount; ++i)
                {
                    decals[i]->Save(os);
                }

                Write(os, decalGroupCount);
                for (std::uint32_t i = 0; i < decalGroupCount; ++i)
                {
                    decalGroups[i]->Save(os);
                }
            }

            {
                Write(os, widthOther);
                Write(os, heightOther);
            }

            // normal map
            {
                std::uint32_t normalMapCount = normalMapData.size();
                Write(os, normalMapCount);

                for (std::uint32_t i = 0u; i < normalMapCount; ++i)
                {
                    std::uint32_t normalMapDataSize = normalMapData[i].size();
                    Write(os, normalMapDataSize);
                    WriteBuffer(os, normalMapData[i], normalMapDataSize);

                }
            }

            // texture map
            {
                std::uint32_t count = strataLerpData.size();
                if (versionMinor < 54)
                {
                    Write(os, count); // always 1
                }

                for (std::uint32_t i = 0u; i < count; ++i)
                {
                    std::uint32_t size = strataLerpData[i].size();
                    Write(os, size);
                    WriteBuffer(os, strataLerpData[i], size);
                }
            }

            // watermap
            {
                std::uint32_t count = waterLerpData.size();
                Write(os, count);    // always 1

                for (std::uint32_t i = 0u; i < count; ++i)
                {
                    std::uint32_t size = waterLerpData[i].size();
                    Write(os, size);
                    WriteBuffer(os, waterLerpData[i], size);
                }
            }
            WriteBuffer(os, waterFoamMask, waterFoamMask.size());           // width*height / 4);
            WriteBuffer(os, waterFlatnessMask, waterFlatnessMask.size());   // width*height / 4);
            WriteBuffer(os, waterDepthBiasMask, waterDepthBiasMask.size()); // width*height / 4);
            WriteBuffer(os, terrainTypeData, terrainTypeData.size());       // width*height);

            if (versionMinor < 53)
            {
                std::string dummy;
                Write(os, dummy);    // always null strings
                Write(os, dummy);
            }

            // v59 objects
            if (versionMinor >= 59)
            {
                v59ObjectA->Save(os);

                std::uint32_t count = v59ObjectB.size();
                Write(os, count);
                for (std::uint32_t i = 0u; i < count; ++i)
                {
                    v59ObjectB[i]->Save(os);
                }
            }

            // Props
            {
                std::uint32_t propCount = props.size();
                Write(os, propCount);
                for (std::uint32_t i = 0; i < propCount; ++i)
                {
                    props[i]->Save(os);
                }
            }

        }


        void Scmp::Resize(int newWidth, int newHeight)
        {
            float scalex = float(newWidth) / float(width);
            float scalez = float(newHeight) / float(height);
            float scaley = std::sqrt(scalex*scalez);

            std::vector<std::int16_t> newHeightMapData((newWidth + 1)*(newHeight + 1));
            ResizeImage<std::int16_t>(heightMapData.data(), newHeightMapData.data(), width + 1, height + 1, newWidth + 1, newHeight + 1, true);
            GainImage<std::int16_t>(newHeightMapData, scaley);
            heightMapData = newHeightMapData;

            waterShaderProperties->ScaleSize(scaley);

            for (auto wg : waveGenerators)
            {
                wg->ScaleSize(scalex, scaley, scalez);
            }
            for (auto s : strata)
            {
                s->ScaleSize(std::sqrt(scalex*scalez));
            }
            for (auto d : decals)
            {
                d->ScaleSize(scalex, scaley, scalez);
            }

            // normalMapData, strataLerpData, waterLerpData ... all DDS format ...

            // waterFoamMask, waterFlatnessMask, waterDepthBiasMask all 64k

            // terrainTypeData .... width x height or widthOther x heightOther ???

            for (auto p : props)
            {
                p->ScaleSize(scalex, scaley, scalez);
            }

            for (std::vector<std::uint8_t> *dataPtr : { &waterFoamMask, &waterFlatnessMask, &waterDepthBiasMask, &terrainTypeData })
            {
                int sizeDivisor = width*height / dataPtr->size();
                std::vector<std::uint8_t> newData(newWidth*newHeight / sizeDivisor);
                int widthDivisor = int(0.5 + std::sqrt(double(sizeDivisor)));
                ResizeImage<std::uint8_t>(
                    dataPtr->data(), newData.data(), 
                    width / widthDivisor, height / widthDivisor, newWidth / widthDivisor, newHeight / widthDivisor, false);
                *dataPtr = newData;
            }

            widthOther = widthOther * newWidth / width;
            heightOther = heightOther * newHeight / height;

            width = newWidth;
            height = newHeight;
        }


        void Scmp::Import(const Scmp &other, int column0, int row0)
        {
            // previewImageData.  not important, user can update it with any map editor

            ImportImage(
                other.heightMapData.data(), 1 + other.width, 1 + other.height,
                this->heightMapData.data(), 1 + this->width, 1 + this->height,
                column0, row0, true);

            ImportImage(
                other.terrainTypeData.data(), other.width, other.height,
                this->terrainTypeData.data(), this->width, this->height,
                column0, row0, true);

            for (std::size_t n = 0u; n < normalMapData.size() && n < other.normalMapData.size(); ++n)
            {
                ImportDds(
                    (std::uint8_t*)other.normalMapData[n].data(), other.normalMapData[n].size(),
                    (std::uint8_t*)normalMapData[n].data(), normalMapData[n].size(),
                    other.width, other.height, width, height,
                    column0, row0, "normalMapData", false);
            }

            for (std::size_t n = 0u; n < strataLerpData.size() && n < other.strataLerpData.size(); ++n)
            {
                ImportDds(
                    (std::uint8_t*)other.strataLerpData[n].data(), other.strataLerpData[n].size(),
                    (std::uint8_t*)strataLerpData[n].data(), strataLerpData[n].size(),
                    other.width, other.height, width, height,
                    column0, row0, "strataLerpData", false);
            }

            for (std::size_t n = 0u; n < waterLerpData.size() && n < other.waterLerpData.size(); ++n)
            {
                ImportDds(
                    (std::uint8_t*)other.waterLerpData[n].data(), other.waterLerpData[n].size(),
                    (std::uint8_t*)waterLerpData[n].data(), waterLerpData[n].size(),
                    other.width, other.height, width, height,
                    column0, row0, "waterLerpData", false);
            }

            int columnEnd = column0 + other.width;
            int rowEnd = row0 + other.height;

            waveGenerators = ImportItemsInRectangle(waveGenerators, other.waveGenerators, column0, row0, columnEnd, rowEnd);
            decals = ImportItemsInRectangle(decals, other.decals, column0, row0, columnEnd, rowEnd);
            props = ImportItemsInRectangle(props, other.props, column0, row0, columnEnd, rowEnd);
        }


        void Scmp::DumpTextures(const std::string &prefix) const
        {
            DumpTexture(prefix + "preview.dds", previewImageData);
            for (unsigned i = 0u; i < normalMapData.size(); ++i)
            {
                std::ostringstream ss;
                ss << prefix << "normalmap" << i << ".dds";
                DumpTexture(ss.str(), normalMapData[i]);
            }
            for (unsigned i = 0u; i < strataLerpData.size(); ++i)
            {
                std::ostringstream ss;
                ss << prefix << "texturemap" << i << ".dds";
                DumpTexture(ss.str(), strataLerpData[i]);
            }
            for (unsigned i = 0u; i < waterLerpData.size(); ++i)
            {
                std::ostringstream ss;
                ss << prefix << "watermap" << i << ".dds";
                DumpTexture(ss.str(), waterLerpData[i]);
            }
            DumpTexture(prefix + "waterFoamMask.dat", waterFoamMask);
            DumpTexture(prefix + "waterFlatnessMask.dat", waterFoamMask);
            DumpTexture(prefix + "waterDepthMask.dat", waterFoamMask);
            DumpTexture(prefix + "terrainTypeData.dat", waterFoamMask);
        }


        void Scmp::DumpTexture(const std::string &filename, const std::vector<std::uint8_t> &data) const
        {
            std::ofstream fs(filename, std::ios::out);
            fs.write((const char*)data.data(), data.size()*sizeof(std::uint8_t));
        }

        void DdsInfo(std::ostream &os, const char *data, std::size_t bytes)
        {
            os << "header=\"" << std::string(data, data + 4) << '"'
                << " size=" << *(std::uint32_t*)(data + 0x0c) << 'x' << *(std::uint32_t*)(data + 0x10)
                << " bytes=" << bytes
                << " fourcc=\"" << std::string(data + 0x54, data + 0x58) << "\" / " 
                << std::hex << *(std::uint32_t*)(data + 0x54) << std::dec;
        }

        void Scmp::MapInfo(std::ostream &os)
        {
            os << "version: " << versionMajor << '.' << versionMinor << std::endl;
            os << "preview dds: "; DdsInfo(os, (const char*)previewImageData.data(), previewImageData.size()); os << std::endl;
            os << "heightmap: " << width << 'x' << height << 'x' << heightScale << std::endl;
            os << "terrainShader: " << terrainShader << std::endl;
            os << "backgroundTexturePath: " << backgroundTexturePath << std::endl;
            os << "skyCubeMapTexturePath: " << skyCubeMapTexturePath << std::endl;
            for (auto &texture : environmentCubeMapTextures)
            {
                os << "environmentCubeMapTextures[" << texture.first << "]: " << texture.second << std::endl;
            }
            os << "lightingMultiplier: " << lightingMultiplier << std::endl;
            os << "sunDirection: " << sunDirection[0] << ", " << sunDirection[1] << ", " << sunDirection[2] << std::endl;
            os << "sunAmbience: " << sunAmbience[0] << ", " << sunAmbience[1] << ", " << sunAmbience[2] << std::endl;
            os << "sunColour: " << sunColour[0] << ", " << sunColour[1] << ", " << sunColour[2] << std::endl;
            os << "shadowFillColour: " << shadowFillColour[0] << ", " << shadowFillColour[1] << ", " << shadowFillColour[2] << std::endl;
            os << "specularColour: " << specularColour[0] << ", " << specularColour[1] << ", " << specularColour[2] << ", " << specularColour[3] << std::endl;
            os << "bloom: " << bloom << std::endl;
            os << "fogColour: " << fogColour[0] << ", " << fogColour[1] << ", " << fogColour[2] << std::endl;
            os << "fogStart: " << fogStart << std::endl;
            os << "fogEnd: " << fogEnd << std::endl;
            os << "waterShaderProperties hasWater: " << int(waterShaderProperties->hasWater) << std::endl;
            os << "waterShaderProperties elevation: " << waterShaderProperties->elevation << std::endl;
            os << "waterShaderProperties elevationDeep: " << waterShaderProperties->elevationDeep << std::endl;
            os << "waterShaderProperties elevationAbyss: " << waterShaderProperties->elevationAbyss << std::endl;
            os << "waterShaderProperties surfaceColor: " << waterShaderProperties->surfaceColor[0] << ", " << waterShaderProperties->surfaceColor[1] << ", " << waterShaderProperties->surfaceColor[2] << std::endl;
            os << "waterShaderProperties colorLerp: " << waterShaderProperties->colorLerp[0] << ", " << waterShaderProperties->colorLerp[1] << std::endl;
            os << "waterShaderProperties refractionScale: " << waterShaderProperties->refractionScale << std::endl;
            os << "waterShaderProperties fresnelBias: " << waterShaderProperties->fresnelBias << std::endl;
            os << "waterShaderProperties fresnelPower: " << waterShaderProperties->fresnelPower << std::endl;
            os << "waterShaderProperties unitReflection: " << waterShaderProperties->unitReflection << std::endl;
            os << "waterShaderProperties skyReflection: " << waterShaderProperties->skyReflection << std::endl;
            os << "waterShaderProperties sunShininess: " << waterShaderProperties->sunShininess << std::endl;
            os << "waterShaderProperties sunStrength: " << waterShaderProperties->sunStrength << std::endl;
            os << "waterShaderProperties sunDirection: " << waterShaderProperties->sunDirection[0] << ", " << waterShaderProperties->sunDirection[1] << ", " << waterShaderProperties->sunDirection[2] << std::endl;
            os << "waterShaderProperties sunColor: " << waterShaderProperties->sunColor[0] << ", " << waterShaderProperties->sunColor[1] << ", " << waterShaderProperties->sunColor[2] << std::endl;
            os << "waterShaderProperties sunReflection: " << waterShaderProperties->sunReflection << std::endl;
            os << "waterShaderProperties sunGlow: " << waterShaderProperties->sunGlow << std::endl;
            os << "waterShaderProperties cubemapTexturePath: " << waterShaderProperties->cubemapTexturePath << std::endl;
            os << "waterShaderProperties waterRampTexturePath: " << waterShaderProperties->waterRampTexturePath << std::endl;
            for (auto &wt : waterShaderProperties->waveTextures)
            {
                os << "waterShaderProperties waveTexture --- path: " << wt->path << std::endl;
                os << "waterShaderProperties waveTexture normalMovement: " << wt->normalMovement[0] << ", " << wt->normalMovement[1] << std::endl;
                os << "waterShaderProperties waveTexture normalRepeat: " << wt->normalRepeat << std::endl;
            }
            os << "number of waveGenerators: " << waveGenerators.size() << std::endl;
            for (auto &wg : waveGenerators)
            {
                os << "--- waveGenerator textureName: " << wg->textureName << std::endl;
                os << "waveGenerator rampName: " << wg->rampName << std::endl;
                os << "waveGenerator position: " << wg->position[0] << ", " << wg->position[1] << ", " << wg->position[2] << std::endl;
                os << "waveGenerator velocity: " << wg->velocity[0] << ", " << wg->velocity[1] << ", " << wg->velocity[2] << std::endl;
                os << "waveGenerator rotation: " << wg->rotation << std::endl;
                os << "waveGenerator lifetimeFirst: " << wg->lifetimeFirst << std::endl;
                os << "waveGenerator lifetimeSecond: " << wg->lifetimeSecond << std::endl;
                os << "waveGenerator periodFirst: " << wg->periodFirst << std::endl;
                os << "waveGenerator periodSecond: " << wg->periodSecond << std::endl;
                os << "waveGenerator scaleFirst: " << wg->scaleFirst << std::endl;
                os << "waveGenerator scaleSecond: " << wg->scaleSecond << std::endl;
                os << "waveGenerator frameCount: " << wg->frameCount << std::endl;
                os << "waveGenerator frameRateFirst: " << wg->frameRateFirst << std::endl;
                os << "waveGenerator frameRateSecond: " << wg->frameRateSecond << std::endl;
                os << "waveGenerator stripCount: " << wg->stripCount << std::endl;
                break;
            }

            os << "minimapContourInterval: " << minimapContourInterval << std::endl;
            os << "minimapDeepWaterColor: " << std::hex << minimapDeepWaterColor << std::endl;
            os << "minimapContourColor: " << std::hex << minimapContourColor << std::endl;
            os << "minimapShoreColor: " << std::hex << minimapShoreColor << std::endl;
            os << "minimapLandStartColor: " << std::hex << minimapLandStartColor << std::endl;
            os << "minimapLandEndColor: " << std::hex << minimapLandEndColor << std::endl;
            os << std::dec;
            os << "tileset: \"" << tileset << '"' << std::endl;
            for (auto &stratum : strata)
            {
                if (stratum)
                {
                    os << "--- stratum albedoPath: " << stratum->albedoPath << std::endl;
                    os << "stratum normalsPath: " << stratum->normalsPath << std::endl;
                    os << "stratum albedoScale: " << stratum->albedoScale << std::endl;
                    os << "stratum normalsScale: " << stratum->normalsScale << std::endl;
                }
                else
                {
                    os << "--- stratum NULL" << std::endl;
                }
            }

            os << "unknownPreDecals(int): " << unknownPreDecals.asInts[0] << ", " << unknownPreDecals.asInts[1] << std::endl;
            os << "unknownPreDecals(float): " << unknownPreDecals.asFloats[0] << ", " << unknownPreDecals.asFloats[1] << std::endl;

            os << "number of decals: " << decals.size() << std::endl;
            os << "number of decalGroups: " << decalGroups.size() << std::endl;
            os << "size other: " << widthOther << 'x' << heightOther << std::endl;
            os << "number of normalMapDatas: " << normalMapData.size() << std::endl;
            for (auto &nm : normalMapData)
            {
                os << "normalMapData dds: "; DdsInfo(os, (const char*)nm.data(), nm.size()); os << std::endl;
            }
            os << "number of strataLerpData: " << strataLerpData.size() << std::endl;
            for (auto &tm : strataLerpData)
            {
                os << "strataLerpData dds: "; DdsInfo(os, (const char*)tm.data(), tm.size()); os << std::endl;
            }
            os << "number of waterLerpData: " << waterLerpData.size() << std::endl;
            for (auto &wm : waterLerpData)
            {
                os << "waterLerpData dds: "; DdsInfo(os, (const char*)wm.data(), wm.size()); os << std::endl;
            }
            os << "waterFoamMask: " << waterFoamMask.size() << " bytes" << std::endl;
            os << "waterFlatnessMask: " << waterFlatnessMask.size() << " bytes" << std::endl;
            os << "waterDepthBiasMask: " << waterDepthBiasMask.size() << " bytes" << std::endl;
            os << "terrainTypeData: " << terrainTypeData.size() << " bytes" << std::endl;


        }

    }
}
