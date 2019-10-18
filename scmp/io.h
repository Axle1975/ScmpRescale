#pragma once

#include <istream>
#include <ostream>
#include <string>
#include <type_traits>

namespace nfa {
    namespace scmp {

        std::size_t BytesRemaining(std::istream &is);

        template<typename T>
        inline void Read(std::istream &is, T &result)
        {
            is.read((char*)&result, sizeof(T));
        }

        template<typename T>
        inline void Write(std::ostream &os, const T &data)
        {
            os.write((const char*)&data, sizeof(T));
        }

        template<>
        inline void Read<std::string>(std::istream &is, std::string &result)
        {
            result.clear();
            for (int ch = is.get(); ch != 0 && is.good(); ch = is.get())
            {
                result += char(ch);
            }
        }

        template<>
        inline void Write<std::string>(std::ostream &os, const std::string &data)
        {
            os.write(data.c_str(), data.size() + 1);
        }

        template<typename ContainerT>
        // the enable_if requires ContainerT to be a container of primitives, 
        // to ensure that that &buffer[0] is actually a pointer to a data buffer, not to some class or struct (an easy mistake to make)
        typename std::enable_if< std::is_fundamental<typename ContainerT::value_type>::value >::type
        ReadBuffer(std::istream &is, ContainerT &buffer, std::size_t itemCount)
        {
            if (itemCount == 0)
            {
                return;
            }

            if (BytesRemaining(is) / sizeof(ContainerT::value_type) < itemCount)
            {
                std::ostringstream ss;
                ss << "Not enough bytes remaining to read " << itemCount << " items of size " << sizeof(ContainerT::value_type);
                throw std::runtime_error(ss.str());
            }

            buffer.resize(itemCount);
            is.read((char*)&buffer[0], itemCount * sizeof(buffer[0]));
        }

        template<typename ContainerT>
        // the enable_if requires ContainerT to be a container of primitives, 
        // to ensure that that &buffer[0] is actually a pointer to a data buffer, not to some class or struct (an easy mistake to make)
        typename std::enable_if< std::is_fundamental<typename ContainerT::value_type>::value >::type
            WriteBuffer(std::ostream &os, const ContainerT &buffer, std::size_t itemCount)
        {
            os.write((const char*)&buffer[0], itemCount * sizeof(buffer[0]));
        }
    }
}
