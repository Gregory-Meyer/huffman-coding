#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

struct TreeNode;

using BitsetT = boost::dynamic_bitset<>;
using SymbolVecT = std::vector<std::pair<char, BitsetT>>;
using BitMapT = std::unordered_map<BitsetT, char>;
using NodePairT = std::pair<std::unique_ptr<TreeNode>, BitsetT>;

class TreeNode {
public:
    TreeNode() noexcept = default;

    TreeNode(char symbol) noexcept : left_{ nullptr }, right_{ nullptr },
                                     symbol_{ symbol }  { }

    template <typename U1, typename U2>
    TreeNode(U1 &&left, U2 &&right) : left_{ std::forward<U1>(left) },
                                      right_{ std::forward<U2>(right) } { }

    constexpr char symbol() const noexcept {
        return symbol_;
    }

    TreeNode* get_left() const noexcept {
        return left_.get();
    }

    TreeNode* get_right() const noexcept {
        return right_.get();
    }

private:
    std::unique_ptr<TreeNode> left_;
    std::unique_ptr<TreeNode> right_;
    char symbol_;
};

inline std::ostream& nl(std::ostream &os) {
    return os.put('\n');
}

SymbolVecT parse_symbols(std::istream &is) {
    SymbolVecT symbols;

    int num_buffer;
    std::string bitstring_buffer;

    while (is >> num_buffer >> bitstring_buffer) {
        std::cerr << static_cast<char>(num_buffer) << ": " << bitstring_buffer << std::endl;

        std::reverse(bitstring_buffer.begin(), bitstring_buffer.end());

        BitsetT bits{ bitstring_buffer };

        symbols.emplace_back(static_cast<char>(num_buffer), std::move(bits));
    }

    return symbols;
}

bool node_pair_less(const NodePairT &lhs, const NodePairT &rhs) noexcept {
    if (lhs.second.size() == rhs.second.size()) {
        return lhs.second.to_ulong() < rhs.second.to_ulong();
    }

    return lhs.second.size() < rhs.second.size();
}

void combine_nodes(std::vector<NodePairT> &nodes) {
    std::pop_heap(nodes.begin(), nodes.end(), node_pair_less);
    std::unique_ptr<TreeNode> right = std::move(nodes.back().first);
    nodes.pop_back();

    std::pop_heap(nodes.begin(), nodes.end(), node_pair_less);
    std::unique_ptr<TreeNode> left = std::move(nodes.back().first);
    BitsetT bitset = std::move(nodes.back().second);
    nodes.pop_back();
    
    bitset.pop_back();

    auto center = std::make_unique<TreeNode>(std::move(left),
                                             std::move(right));

    nodes.emplace_back(std::move(center), std::move(bitset));
    std::push_heap(nodes.begin(), nodes.end(), node_pair_less);
}

std::unique_ptr<TreeNode> make_tree(SymbolVecT symbols) {
    std::vector<NodePairT> nodes;
    nodes.reserve(symbols.size());

    for (auto &[symbol, bits] : symbols) {
        nodes.emplace_back(std::make_unique<TreeNode>(symbol), std::move(bits));
    }

    symbols.clear();

    std::make_heap(nodes.begin(), nodes.end(), node_pair_less);

    while (nodes.size() > 1) {
        combine_nodes(nodes);
    }

    return std::move(nodes.front().first);
}

std::unique_ptr<TreeNode> parse_tree(std::istream &is) {
    SymbolVecT symbols = parse_symbols(is);

    return make_tree(std::move(symbols));
}

std::uint32_t num_bits(std::istream &is) {
    union Int32Char {
        std::uint32_t num;
        std::array<char, 4> arr;
    };

    Int32Char size_buffer;
    is.read(size_buffer.arr.data(), size_buffer.arr.size());

    return size_buffer.num;
}

void decompress(std::istream &is, std::ostream &os,
                std::unique_ptr<TreeNode> tree) {
    std::array<char, 4096> buffer;
    const TreeNode *current = tree.get();

    std::string output;

    const std::uint32_t final_bit_count = num_bits(is);
    std::uint32_t bit_count = 0;
    BitsetT bitstream;

    while (is.readsome(buffer.data(), buffer.size())) {
        if (bit_count >= final_bit_count) {
            break;
        }

        const std::size_t read_count = is.gcount();

        for (const char c : std::string_view{ buffer.data(), read_count }) {
            if (bit_count >= final_bit_count) {
                break;
            }

            for (int i = 0; i < 8; ++i) {
                if (bit_count >= final_bit_count) {
                    break;
                }

                const bool bit = c & (1 << i);
                bitstream.push_back(bit);

                if (bit) {
                    // std::cerr << "right" << std::endl;
                    current = current->get_right();
                } else {
                    // std::cerr << "left" << std::endl;
                    current = current->get_left();
                }

                if (not current->get_left() and not current->get_right()) {
                    // std::cerr << current->symbol() << std::endl;
                    output.push_back(current->symbol());
                    current = tree.get();
                }

                ++bit_count;
            }
        }
    }

    std::cerr << bitstream << std::endl;

    if (current != tree.get()) {
        std::cerr << "invalid huffman code" << std::endl;
        // throw std::runtime_error{ "decompress" };
    }

    os.write(output.data(), output.size());
}

void print_tree_impl(const TreeNode *const tree, std::ostream &os,
                     BitsetT &bitset) {
    if (not tree) {
        return;
    }

    if (tree->get_left() or tree->get_right()) {
        bitset.push_back(false);
        print_tree_impl(tree->get_left(), os, bitset);

        bitset.flip(bitset.size() - 1);
        print_tree_impl(tree->get_right(), os, bitset);

        bitset.pop_back();

        return;
    }

    os << tree->symbol() << ": " << bitset << nl;
}

void print_tree(const TreeNode *const tree, std::ostream &os) {
    BitsetT bitset;

    print_tree_impl(tree, os, bitset);
}

int main(const int argc, const char *const argv[]) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);

    if (argc != 4) {
        std::cerr << "usage: decode DICTIONARY INPUT OUTPUT" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::unique_ptr<TreeNode> tree;

    {
        std::ifstream ifs{ argv[1] };
        tree = parse_tree(ifs);
    }

    std::cout << std::endl << std::endl;

    print_tree(tree.get(), std::cout);

    {
        std::ifstream ifs{ argv[2], std::ifstream::binary };
        std::ofstream ofs{ argv[3], std::ofstream::binary };
        decompress(ifs, ofs, std::move(tree));
    }
}
