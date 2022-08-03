#pragma once

#include <bcos-crypto/hasher/Hasher.h>
#include <bcos-utilities/Ranges.h>
#include <boost/format.hpp>
#include <boost/throw_exception.hpp>
#include <algorithm>
#include <exception>
#include <iterator>
#include <span>
#include <stdexcept>
#include <type_traits>

namespace bcos::tool::merkle
{

template <class Range, class HashType>
concept InputRange = RANGES::random_access_range<Range> &&
    std::is_same_v<std::remove_cvref_t<RANGES::range_value_t<Range>>, HashType>;

template <class Range, class HashType>
concept OutputRange = RANGES::random_access_range<Range> && RANGES::output_range<Range, HashType>;

template <class HashType>
struct Proof
{
    std::vector<HashType> hashes;
    std::vector<size_t> levels;

    auto operator<=>(const Proof& rhs) const = default;
};

template <bcos::crypto::hasher::Hasher HasherType, class HashType, size_t width = 2>
class Merkle
{
public:
    static_assert(width >= 2, "Width too short, at least 2");
    using ProofType = Proof<HashType>;

    Merkle() = default;
    Merkle(const Merkle&) = default;
    Merkle(Merkle&&) = default;
    Merkle& operator=(const Merkle&) = default;
    Merkle& operator=(Merkle&&) = default;
    ~Merkle() = default;

    auto operator<=>(const Merkle& rhs) const = default;

    static bool verifyProof(const ProofType& proof, HashType hash, const HashType& root)
    {
        if (proof.hashes.empty() || proof.levels.empty()) [[unlikely]]
            BOOST_THROW_EXCEPTION(std::invalid_argument{"Empty input proof!"});

        auto range = RANGES::subrange<decltype(proof.hashes.begin())>{
            proof.hashes.begin(), proof.hashes.begin()};

        HasherType hasher;
        for (auto it = proof.levels.begin(); it != proof.levels.end(); ++it)
        {
            range = {RANGES::end(range), RANGES::end(range) + *it};
            if (RANGES::end(range) > proof.hashes.end() || RANGES::size(range) > width) [[unlikely]]
                BOOST_THROW_EXCEPTION(std::invalid_argument{"Proof level length out of range!"});

            if (RANGES::find(range, hash) == RANGES::end(range)) [[unlikely]]
                return false;

            if (std::next(it) != proof.levels.end())
            {
                for (auto& rangeHash : range)
                {
                    hasher.update(rangeHash);
                }
                hasher.final(hash);
            }
        }

        if (hash != root) [[unlikely]]
        {
            return false;
        }

        return true;
    }

    ProofType generateProof(const HashType& hash) const
    {
        if (empty()) [[unlikely]]
            BOOST_THROW_EXCEPTION(std::runtime_error{"Empty merkle!"});

        // Query the first level hashes(ordered)
        auto levelRange = RANGES::subrange<decltype(m_nodes.begin())>{
            m_nodes.begin(), m_nodes.begin() + m_levels[0]};

        auto it = RANGES::lower_bound(levelRange, hash);
        if (it == levelRange.end() || *it != hash) [[unlikely]]
            BOOST_THROW_EXCEPTION(std::invalid_argument{"Not found hash in merkle!"});

        auto index = indexAlign(it - RANGES::begin(levelRange));  // Align
        auto start = levelRange.begin() + index;
        auto end = (static_cast<size_t>(levelRange.end() - start) < width) ? levelRange.end() :
                                                                             start + width;
        // auto end = std::min(start + width, levelRange.end());

        ProofType proof;
        proof.hashes.reserve(m_levels.size() * width);
        proof.levels.reserve(m_levels.size());

        proof.hashes.insert(proof.hashes.end(), start, end);
        proof.levels.push_back(end - start);

        // Query next level hashes
        for (auto depth : RANGES::views::iota(1u, m_levels.size()))
        {
            auto length = m_levels[depth];
            index = indexAlign(index / width);
            levelRange = RANGES::subrange<decltype(levelRange.end())>{
                levelRange.end(), levelRange.end() + length};

            start = levelRange.begin() + index;
            // end = std::min(start + width, levelRange.end());
            end = (levelRange.end() - start <
                      (RANGES::range_difference_t<decltype(levelRange)>)width) ?
                      levelRange.end() :
                      start + width;

            assert(levelRange.end() <= m_nodes.end());
            proof.hashes.insert(proof.hashes.end(), start, end);
            proof.levels.push_back(end - start);
        }

        return proof;
    }

    HashType root() const
    {
        if (empty()) [[unlikely]]
            BOOST_THROW_EXCEPTION(std::runtime_error{"Empty merkle!"});

        return *m_nodes.rbegin();
    }

    void import(InputRange<HashType> auto&& input)
    {
        if (std::empty(input)) [[unlikely]]
            BOOST_THROW_EXCEPTION(std::invalid_argument{"Empty input"});

        if (!empty()) [[unlikely]]
            BOOST_THROW_EXCEPTION(std::invalid_argument{"Merkle already imported"});

        auto inputSize = RANGES::size(input);
        m_nodes.resize(getNodeSize(inputSize));

        std::copy_n(RANGES::begin(input), inputSize, m_nodes.begin());
        std::sort(m_nodes.begin(), m_nodes.begin() + inputSize);

        auto inputRange =
            RANGES::subrange<decltype(m_nodes.begin())>{m_nodes.begin(), m_nodes.begin()};
        m_levels.push_back(inputSize);
        while (inputSize > 1)  // Ignore only root
        {
            inputRange = {inputRange.end(), inputRange.end() + inputSize};
            assert(inputRange.end() <= m_nodes.end());

            inputSize = calculateLevelHashes(inputRange,
                RANGES::subrange<decltype(inputRange.end())>{inputRange.end(), m_nodes.end()});
            m_levels.push_back(inputSize);
        }
    }

    auto indexAlign(std::integral auto index) const { return index - ((index + width) % width); }

    void clear()
    {
        m_nodes.clear();
        m_levels.clear();
    }

    auto empty() const { return m_nodes.empty() || m_levels.empty(); }

    std::vector<HashType> m_nodes;
    std::vector<typename decltype(m_nodes)::size_type> m_levels;

private:
    auto getNodeSize(std::integral auto inputSize) const
    {
        auto nodeSize = inputSize;
        while (inputSize > 1)
        {
            inputSize = getLevelSize(inputSize);
            nodeSize += inputSize;
        }

        return nodeSize;
    }

    auto getLevelSize(std::integral auto inputSize) const
    {
        return inputSize == 1 ? 0 : (inputSize + (width - 1)) / width;
    }

    size_t calculateLevelHashes(
        InputRange<HashType> auto&& input, OutputRange<HashType> auto&& output) const
    {
        auto inputSize = RANGES::size(input);
        [[maybe_unused]] auto outputSize = RANGES::size(output);
        auto expectOutputSize = getLevelSize(inputSize);

        assert(inputSize > 0);
        assert(outputSize >= expectOutputSize);

        HasherType hasher;
        for (decltype(inputSize) i = 0; i < inputSize; i += width)
        {
            for (auto j = i; j < i + width && j < inputSize; ++j)
            {
                hasher.update(input[j]);
            }
            auto outputOffset = i / width;
            assert(outputOffset < outputSize);

            hasher.final(output[outputOffset]);
        }

        return expectOutputSize;
    }
};

}  // namespace bcos::tool::merkle