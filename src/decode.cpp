#include <climits>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>

#include <boost/dynamic_bitset.hpp>

using BitsetT = boost::dynamic_bitset<>;
using SymbolMapT = std::unordered_map<char, BitsetT>;
using BitMapT = std::unordered_map<BitsetT, char>;

namespace std {

template <>
struct hash<boost::dynamic_bitset<>> {
    size_t operator()(const boost::dynamic_bitset<> &bitset) const {
        size_t hashed = 0;
        const size_t rsize = min(bitset.size(), sizeof(size_t) * CHAR_BIT);

        for (size_t i = 0; i < rsize; ++i) {
            if (bitset.test(i)) {
                hashed |= 1 << i;
            }
        }

        return hashed;
    }
};

} // namespace std

inline std::ostream& nl(std::ostream &os) {
    return os.put('\n');
}

std::pair<BitMapT, std::uintmax_t> parse_map(std::istream &is) {
    SymbolMapT encoding_map;

    int num_buffer;
    BitsetT bits_buffer;

    while (is >> num_buffer >> bits_buffer) {
        encoding_map[static_cast<char>(num_buffer)] = bits_buffer;
    }

    BitMapT decoding_map;
    std::uintmax_t max_length = 0;

    for (auto &[symbol, bits] : encoding_map) {
        if (decoding_map.count(bits) != 0) {
            std::cerr << "duplicate bit sequence" << std::endl;
            throw std::runtime_error{ "parse_map" };
        }

        decoding_map.emplace(std::move(bits), symbol);

        if (bits.size() > max_length) {
            max_length = bits.size();
        }
    }

    return { decoding_map, max_length };
}

void decompress(std::istream &is, std::ostream &os, BitMapT map,
                const std::uintmax_t max_length) {
    std::array<char, 4096> input_buffer;
    BitsetT bit_buffer;

    while (is.readsome(input_buffer.data(), input_buffer.size())) {
        if (bit_buffer.size() > max_length) {
            
        }
    }
}

int main(const int argc, const char *const argv[]) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);

    if (argc != 4) {
        std::cerr << "usage: decode DICTIONARY INPUT OUTPUT" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    BitMapT map;
    std::uintmax_t max_bitset_length;

    {
        std::ifstream ifs{ argv[1] };
        std::tie(map, max_bitset_length) = parse_map(ifs);
    }

    {
        std::ifstream ifs{ argv[2], std::ifstream::binary };
        std::ofstream ofs{ argv[3], std::ofstream::binary };
        decompress(ifs, ofs, std::move(map), max_bitset_length);
    }
}
