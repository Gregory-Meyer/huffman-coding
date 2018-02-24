#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <array>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

struct TreeNode;

using CountMapT = std::unordered_map<char, std::uintmax_t>;
using CountVecT = std::vector<std::pair<char, std::uintmax_t>>;
using NodeVecT = std::vector<std::unique_ptr<TreeNode>>;
using BitsetT = boost::dynamic_bitset<>;
using SymbolMapT = std::unordered_map<char, BitsetT>;
using SymbolVecT = std::vector<std::pair<char, BitsetT>>;

class TreeNode {
public:
    template <typename S, typename D>
    TreeNode(S &&symbol, D &&weight)
        : symbol_{ std::forward<S>(symbol) },
          weight_{ std::forward<D>(weight) },
          left_{ nullptr }, right_{ nullptr }
    { }

    template <typename S, typename D, typename U1, typename U2>
    TreeNode(S &&symbol, D &&weight, U1 &&left, U2 &&right)
        : symbol_{ std::forward<S>(symbol) },
          weight_{ std::forward<D>(weight) },
          left_{ std::forward<U1>(left) }, right_{ std::forward<U2>(right) }
    { }

    constexpr const std::string& symbol() const noexcept {
        return symbol_;
    }

    constexpr std::uintmax_t weight() const noexcept {
        return weight_;
    }

    TreeNode* get_left() const noexcept {
        return left_.get();
    }

    TreeNode* get_right() const noexcept {
        return right_.get();
    }

    std::unique_ptr<TreeNode> release_left() noexcept {
        return std::move(left_);
    }

    std::unique_ptr<TreeNode> release_right() noexcept {
        return std::move(right_);
    }

private:
    std::string symbol_;
    std::uintmax_t weight_;
    std::unique_ptr<TreeNode> left_;
    std::unique_ptr<TreeNode> right_;
};

class SymbolMapMaker {
public:
    void operator()(const TreeNode &node, const BitsetT &bits) {
        if (node.symbol().size() != 1) {
            std::cerr << "invalid symbol..." << std::endl;
            throw std::invalid_argument{ "SymbolMapMaker::operator()" };
        }

        if (map_.count(node.symbol().front()) != 0) {
            std::cerr << "revisiting node..." << std::endl;
            throw std::invalid_argument{ "SymbolMapMaker::operator()" };
        }

        map_.emplace(node.symbol().front(), bits);
    }

    // consumes this object and transfers ownership
    SymbolMapT map() noexcept {
        return std::move(map_);
    }

private:
    SymbolMapT map_;
};

inline std::ostream& nl(std::ostream &os) {
    return os.put('\n');
}

CountMapT make_alphabet_map(std::istream &is) {
    CountMapT alphabet_counts;
    std::array<char, 4096> buffer;

    while (is.readsome(buffer.data(), buffer.size())) {
        const std::size_t read_count = is.gcount();

        for (const char c : std::string_view{ buffer.data(), read_count }) {
            ++alphabet_counts[c];
        }
    }

    return alphabet_counts;
}

CountVecT get_alphabet_counts(std::istream &is) {
    CountMapT map = make_alphabet_map(is);

    CountVecT counts;
    counts.reserve(map.size());

    for (const auto [symbol, count] : map) {
        counts.emplace_back(symbol, count);
    }

    return counts;
}

bool node_ptr_lt(const std::unique_ptr<TreeNode> &lhs,
                 const std::unique_ptr<TreeNode> &rhs) {
    return lhs->weight() > rhs->weight();
}

NodeVecT make_nodes(CountVecT counts) {
    NodeVecT nodes;
    nodes.reserve(counts.size());

    for (const auto [symbol, count] : counts) {
        nodes.push_back(std::make_unique<TreeNode>(symbol, count));
    }

    std::make_heap(nodes.begin(), nodes.end(), node_ptr_lt);

    return nodes;
}

void insert_node(NodeVecT &nodes) {
    std::pop_heap(nodes.begin(), nodes.end(), node_ptr_lt);
    std::unique_ptr<TreeNode> left{ std::move(nodes.back()) };
    nodes.pop_back();

    std::pop_heap(nodes.begin(), nodes.end(), node_ptr_lt);
    std::unique_ptr<TreeNode> right{ std::move(nodes.back()) };
    nodes.pop_back();

    nodes.push_back(std::make_unique<TreeNode>(
        left->symbol() + right->symbol(), left->weight() + right->weight(),
        std::move(left), std::move(right)
    ));
    std::push_heap(nodes.begin(), nodes.end(), node_ptr_lt);
}

std::unique_ptr<TreeNode> make_tree(CountVecT counts) {
    NodeVecT nodes = make_nodes(std::move(counts));

    while (nodes.size() > 1) {
        insert_node(nodes);
    }

    return std::move(nodes.front());
}

void print_node(const TreeNode &node) {
    std::cout << '\'' << node.symbol() << "': " << node.weight() << nl;
}

template <typename F>
void inorder_traversal(const TreeNode *const node, F &&f) {
    if (not node) {
        return;
    }

    inorder_traversal(node->get_left(), std::forward<F>(f));

    std::invoke(std::forward<F>(f), *node);

    inorder_traversal(node->get_right(), std::forward<F>(f));
}

// append 0 if left, append 1 if right
template <typename F>
void leaf_traversal_impl(const TreeNode *const node, F &&f,
                         BitsetT &code) {
    if (not node) {
        return;
    } else if (node->get_left() or node->get_right()) {
        code.push_back(false);
        leaf_traversal_impl(node->get_left(), std::forward<F>(f), code);
        code.flip(code.size() - 1);
        leaf_traversal_impl(node->get_right(), std::forward<F>(f), code);
        code.pop_back();

        return;
    }

    std::invoke(std::forward<F>(f), *node, code);
}

template <typename F>
void leaf_traversal(const TreeNode *const node, F &&f) {
    BitsetT code;

    leaf_traversal_impl(node, std::forward<F>(f), code);
}

void print_code(std::ostream &os, const SymbolVecT &code) {
    std::vector<std::size_t> length_counts;

    for (const auto &[symbol, bits] : code) {
        os << static_cast<int>(symbol) << " " << bits << nl;
    }
}

bool symbol_pair_lt(const std::pair<char, BitsetT> &lhs,
                    const std::pair<char, BitsetT> &rhs) {
    if (lhs.second.size() == rhs.second.size()) {
        return lhs.first < rhs.first;
    }

    return lhs.second.size() < rhs.second.size();
}

SymbolVecT canonical_transform(SymbolMapT map) {
    SymbolVecT code;
    code.reserve(map.size());

    for (auto &[symbol, bits] : map) {
        code.emplace_back(symbol, std::move(bits));
    }

    std::sort(code.begin(), code.end(), symbol_pair_lt);

    {
    const BitsetT *last = nullptr;
    for (auto &pair : code) {
        // would use a structured binding, but symbol is unused
        BitsetT &bits = pair.second;

        if (not last) {
            bits.reset();
        } else {
            const std::size_t last_length = last->size();
            const std::size_t current_length = bits.size();

            bits = BitsetT{ current_length, last->to_ulong() + 1 };
            bits <<= (current_length - last_length);
        }

        last = &bits;
    }
    }

    return code;
}

int main(const int argc, const char *const argv[]) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);

    if (argc != 3) {
        std::cerr << "usage: make_dictionary INPUT OUTPUT" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    CountVecT counts;

    {
        std::ifstream ifs{ argv[1], std::ifstream::binary };
        counts = get_alphabet_counts(ifs);
    }

    const std::unique_ptr<TreeNode> tree = make_tree(std::move(counts));

    SymbolMapMaker map_maker;
    leaf_traversal(tree.get(), map_maker);
    const SymbolVecT code = canonical_transform(map_maker.map());

    {
        std::ofstream ofs{ argv[2] };
        print_code(ofs, code);
    }
}
