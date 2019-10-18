#include "io.h"

namespace nfa {
    namespace scmp {

        std::size_t BytesRemaining(std::istream &is)
        {
            std::streamsize pos = is.tellg();
            is.seekg(0, std::ios_base::end);
            std::streamsize end = is.tellg();
            is.seekg(pos, std::ios_base::beg);
            return end - pos;
        }

    }
}
