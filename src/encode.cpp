#include <cstdlib>
#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <unordered_map>
#include <vector>

#include <boost/dynamic_bitset.hpp>

using BitsetT = boost::dynamic_bitset<>;
using SymbolMapT = std::unordered_map<char, BitsetT>;

inline std::ostream& nl(std::ostream &os) {
    return os.put('\n');
}

SymbolMapT parse_map(std::istream &is) {
    SymbolMapT map;

    int num_buffer;
    BitsetT bits_buffer;

    while (is >> num_buffer >> bits_buffer) {
        map[static_cast<char>(num_buffer)] = bits_buffer;
    }

    return map;
}

BitsetT huffman_bitstream(std::istream &is, SymbolMapT map) {
    std::array<char, 4096> buffer;
    std::string contents;

    while (is.readsome(buffer.data(), buffer.size())) {
        contents.insert(contents.cend(), buffer.begin(),
                        buffer.begin() + is.gcount());
    }

    BitsetT bitstream;

    for (const char character : contents) {
        const auto &map_bits = map.at(character);

        for (BitsetT::size_type i = 0; i < map_bits.size(); ++i) {
            bitstream.push_back(map_bits.test(i));
        }
    }

    return bitstream;
}

void pad_bitstream(BitsetT &bitstream) {
    while (bitstream.size() % 8 != 0) {
        bitstream.push_back(false);
    }
}

std::string convert_bitstream(BitsetT bitstream) {
    pad_bitstream(bitstream);

    std::string output;
    output.reserve(bitstream.size() / 8);

    while (not bitstream.empty()) {
        char to_put = '\0';

        for (int i = 0; i < 8; ++i) {
            if (bitstream.test(bitstream.size() - 1)) {
                to_put |= static_cast<char>(1 << (7 - i));
            }

            bitstream.pop_back();
        }

        output.push_back(to_put);
    }

    std::reverse(output.begin(), output.end());

    return output;
}

void compress_stream(std::istream &is, std::ostream &os, SymbolMapT map) {
    union Int32Char {
        constexpr explicit Int32Char(const std::uint32_t number) noexcept
            : num{ number }
        { }

        std::uint32_t num;
        std::array<char, 4> arr;
    };

    BitsetT bitstream = huffman_bitstream(is, std::move(map));
    const Int32Char length{ static_cast<std::uint32_t>(bitstream.size()) };

    const std::string output = convert_bitstream(std::move(bitstream));

    os.write(length.arr.data(), length.arr.size());
    os.write(output.data(), output.size());
} 

int main(const int argc, const char *const argv[]) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);

    if (argc != 4) {
        std::cerr << "usage: encode DICTIONARY INPUT OUTPUT" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    SymbolMapT map;

    {
        std::ifstream ifs{ argv[1] };
        map = parse_map(ifs);
    }

    {
        std::ifstream ifs{ argv[2], std::ifstream::binary };
        std::ofstream ofs{ argv[3], std::ofstream::binary };
        compress_stream(ifs, ofs, std::move(map));
    }
}
