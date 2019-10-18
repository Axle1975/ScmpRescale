#include "scmp/scmp.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>

void ValidateScmp(const nfa::scmp::Scmp &scmp)
{
    for (auto &normalMap : scmp.normalMapData)
    {
        const char *data = (const char*) normalMap.data();
        if (strncmp(data, "DDS ", 4))
            throw std::runtime_error("normal map is not a DDS!");
        if (strncmp(data + 0x54, "DXT5", 4))
            throw std::runtime_error("normal map DDS is not DXT5!");
    }
}

void LoadScmp(const std::string &fn)
{
    try
    {
        std::cout << fn << " ... ";
        std::ifstream fs(fn, std::ios::binary);
        nfa::scmp::Scmp scmp(fs);
        ValidateScmp(scmp);
        std::cout << "OK" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}

void main(int argc, char *argv[])
{
    if (argc <= 1)
    {
        std::cerr << "usage: test_scmp d:\\directory_full_of_maps" << std::endl;
        return;
    }

    for (int i = 1; i < argc; ++i)
    {
        boost::filesystem::path p(argv[i]);
        if (boost::filesystem::is_directory(p))
        {
            boost::filesystem::directory_iterator itEnd;
            for (auto it = boost::filesystem::directory_iterator(p); it != itEnd; ++it)
            {
                if (boost::iequals(it->path().extension().string(), ".scmap"))
                {
                    LoadScmp(it->path().string());
                }
            }
        }
        else if (boost::iequals(p.extension().string(), ".scmap"))
        {
            LoadScmp(p.string());
        }
    }
}