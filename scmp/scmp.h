#pragma once

#include "io.h"

#include <cstdint>
#include <istream>
#include <limits>
#include <map>
#include <memory>
#include <vector>

static_assert(std::numeric_limits<float>::is_iec559, "Only support IEC 559 (IEEE 754) float");
static_assert(sizeof(float) * CHAR_BIT == 32, "Only support float => Single Precision IEC 559 (IEEE 754)");

namespace nfa {
    namespace scmp {

        template<int N>
        union UnknownFields
        {
            std::uint32_t asInts[N];
            float asFloats[N];
        };


        struct WaveTexture
        {
            WaveTexture(std::istream &is);
            void Save(std::ostream &os);

            std::string path;
            float normalMovement[2];
            float normalRepeat;
        };


        struct WaterShaderProperties
        {
            WaterShaderProperties(std::istream &is);
            void Save(std::ostream &os);
            void ScaleSize(float scaley);

            std::uint8_t hasWater;
            float elevation;
            float elevationDeep;
            float elevationAbyss;
            float surfaceColor[3];
            float colorLerp[2];
            float refractionScale;
            float fresnelBias;
            float fresnelPower;
            float unitReflection;
            float skyReflection;
            float sunShininess;
            float sunStrength;
            float sunDirection[3];
            float sunColor[3];
            float sunReflection;
            float sunGlow;
            std::string cubemapTexturePath;
            std::string waterRampTexturePath;
            std::vector< std::shared_ptr<WaveTexture> > waveTextures;
        };


        struct WaveGenerator
        {
            WaveGenerator(std::istream &is);
            void Save(std::ostream &os);
            void ScaleSize(float scalex, float scaley, float scalez);

            float position[3];
            float rotation;
            std::string textureName;
            std::string rampName;
            float velocity[3];
            float lifetimeFirst;
            float lifetimeSecond;
            float periodFirst;
            float periodSecond;
            float scaleFirst;
            float scaleSecond;
            float frameCount;
            float frameRateFirst;
            float frameRateSecond;
            float stripCount;
        };


        struct Stratum
        {
            Stratum() { }
            Stratum(std::istream &is);

            void Save(std::ostream &os);
            void ScaleSize(float scale);

            void LoadAlbedo(std::istream &is);
            void LoadNormal(std::istream &is);
            void SaveAlbedo(std::ostream &os);
            void SaveNormal(std::ostream &os);

            std::string albedoPath;
            std::string normalsPath;
            float albedoScale;
            float normalsScale;
        };


        struct Decal
        {
            enum Type
            {
                TYPE_UNDEFINED,
                TYPE_ALBEDO,
                TYPE_NORMALS,
                TYPE_WATER_MASK,
                TYPE_WATER_ALBEDO,
                TYPE_WATER_NORMALS,
                TYPE_GLOW,
                TYPE_NORMALS_ALPHA,
                TYPE_GLOW_MASK,
                TYPE_FORCE_DWORD
            };
            Type GetType() const { return (Type)type; }
            Decal(std::istream &is);
            void Save(std::ostream &os);
            void ScaleSize(float scalex, float scaley, float scalez);

            UnknownFields<1> unknown;
            std::int32_t type;
            float position[3];
            float rotation[3];
            std::vector< std::string > texPaths;
            float scale[3];
            float cutOffLOD;
            float nearCutOffLOD;
            std::int32_t ownerArmy;
        };


        struct DecalGroup
        {
            DecalGroup(std::istream &is);
            void Save(std::ostream &os);

            std::int32_t id;
            std::string name;
            std::vector<std::int32_t> data;
        };


        struct Prop
        {
            Prop(std::istream &is);
            void Save(std::ostream &os);
            void ScaleSize(float scalex, float scaley, float scalez);

            std::string blueprintPath;
            float position[3];
            float rotationX[3];
            float rotationY[3];
            float rotationZ[3];
            UnknownFields<3> unknown;
        };


        struct V59ObjectA
        {
            V59ObjectA(std::istream &is);
            void Save(std::ostream &os);

            float p1_v3f[3];     // read into LoadV59ObjectsA, Object* ((v2=a1)+32) { halfWidth, 0.0, halfHeight }
            float p2_sf;         // read into LoadV59ObjectsA, Object* ((v2=a1)+44) { eg -2.5, -100. }
            float p3_sf;         // read into LoadV59ObjectsA, Object* ((v2=a1)+48) { eg 2343.16284 }
            float p4_sf;         // read into LoadV59ObjectsA, Object* ((v2=a1)+52) { eg 1.25663698 }
            std::uint32_t p5_si;         // read into LoadV59ObjectsA, Object* ((v2=a1)+56) {eg 16}
            std::uint32_t p6_si;         // read into LoadV59ObjectsA, Object* ((v2=a1)+60) {eg 6}
            float p7_sf;         // read into LoadV59ObjectsA, Object* ((v2=a1)+64) {eg 165.008347, 312.006897}
            float p8_v3f[3];     // read into LoadV59ObjectsA, Object* ((v2=a1)+68) { eg {0.648593724, 0.820468724, 0.839999974} or {0.899999976, 0.949999988, 0.969999969} }
            float p9_v3f[3];     // read into LoadV59ObjectsA, Object* ((v2=a1)+80)  { eg {0.180000007, 0.430000007, 0.550000012} or {0.000000000, 0.250000000, 0.500000000} }
            float p10_sf;         // read into LoadV59ObjectsA, Object* ((v2=a1)+120) { eg ... ?}
            std::string p11_str1;        // read into LoadV59ObjectsA, Object* ((v2=a1)+124) { eg "/textures/environment/Decal_test_Albedo003.dds" or NULL }
            std::string p12_str2;        // read into LoadV59ObjectsA, Object* ((v2=a1)+152) { eg "/textures/environment/Decal_test_Glow003.dds" or NULL }
            std::uint32_t p13_count;   // eg 9 or 0
            std::vector< std::vector<uint8_t> > p14_vNBuffers40;
            std::string p15_str3;        // read into v2+192 using "copystring"
            std::string p16_str4;        // read into v2+220 using "copystring"
            std::string p17_str5;        // read into v2+248 using "copystring"
            float p18_sf;         // read into v2+276 { eg 1.8 }
            float p19_v3f[3];      // read into v2+280
            std::string p20_str6;        // read into v2+292
            std::uint32_t p21_si;         // ignored
            std::vector< std::vector<uint8_t> > p22_v4Buffers20;
        };


        struct V59ObjectB
        {
            V59ObjectB(std::istream &is, std::uint32_t versionMinor);
            void Save(std::ostream &os);

            std::string p1_str1;
            std::string p2_str2;
            std::uint32_t p3_count;
            std::vector< UnknownFields<9> > p4_unk;
        };


        struct Scmp
        {
            Scmp(std::istream &is);
            void Save(std::ostream &os);

            void DumpTextures(const std::string &prefix) const;
            void DumpTexture(const std::string &filename, const std::vector<std::uint8_t> &data) const;
            void MapInfo(std::ostream &);
            void Resize(int width, int height);
            void Import(const Scmp &other, int column0, int row0, bool additiveTerrain);
            std::int16_t HeightMapAt(int x, int z);

            std::uint32_t magicMap1A;
            std::uint32_t magicBeeffeed;
            std::uint32_t part1_version;
            std::uint16_t wstring1;
            std::int32_t versionMajor;
            std::int32_t versionMinor;
            std::vector<std::uint8_t> previewImageData;     // dds

            // height map
            std::int32_t width;
            std::int32_t height;
            float heightScale; // usually 1/128
            std::vector<std::int16_t> heightMapData;        // raw
            std::string unknownv54String;

            // texture definition
            std::string terrainShader; // usually "TTerrain"
            std::string backgroundTexturePath;
            std::string skyCubeMapTexturePath;

            std::map<std::string, std::string> environmentCubeMapTextures; // keyed by <faction> or <default>

            float lightingMultiplier;
            float sunDirection[3];
            float sunAmbience[3];
            float sunColour[3];
            float shadowFillColour[3];
            float specularColour[4];
            float bloom;
            float fogColour[3];
            float fogStart;
            float fogEnd;
            std::shared_ptr<WaterShaderProperties> waterShaderProperties;
            std::vector<std::shared_ptr<WaveGenerator> > waveGenerators;

            std::int32_t minimapContourInterval;
            std::uint32_t minimapDeepWaterColor;
            std::uint32_t minimapContourColor;
            std::uint32_t minimapShoreColor;
            std::uint32_t minimapLandStartColor;
            std::uint32_t minimapLandEndColor;
            UnknownFields<1> unknownV57field;

            std::string tileset; // always "No Tileset"
            std::uint32_t stratumCount;  // number of actually populated strata
            std::vector<std::shared_ptr<Stratum> > strata;  // always size 10, but depending on mapversion not all are populated

            UnknownFields<2> unknownPreDecals;
            std::vector<std::shared_ptr<Decal> > decals;
            std::vector<std::shared_ptr<DecalGroup> > decalGroups;

            // usually same as width/height, but sometimes half
            std::uint32_t widthOther;
            std::uint32_t heightOther;

            std::vector< std::vector<uint8_t> > normalMapData;  // in the wild, only 1 of these
            std::vector< std::vector<uint8_t> > strataLerpData; // may be 1 or 2, depending on version
            std::vector< std::vector<uint8_t> > waterLerpData;  // in the wild, only 1 of these

            std::vector<std::uint8_t> waterFoamMask;      // obviously not used.. each byte is 00
            std::vector<std::uint8_t> waterFlatnessMask;  // obviously not used.. each byte is FF
            std::vector<std::uint8_t> waterDepthBiasMask; // obviously not used.. each byte is 7f
            std::vector<std::uint8_t> terrainTypeData;

            std::shared_ptr<V59ObjectA> v59ObjectA;
            std::vector< std::shared_ptr<V59ObjectB> > v59ObjectB;  // in the wild, always empty

            std::vector<std::shared_ptr<Prop> > props;
        };
    }
}
