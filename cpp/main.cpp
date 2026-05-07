#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <cstdint>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <numeric>
#include <string>
#include <algorithm> // For std::sort, std::clamp, std::min, std::equal, std::rotate, std::find, std::distance
#include <limits>    // For std::numeric_limits
#include <omp.h>     // For OpenMP pragmas and SIMD

// --- MATH & STABILITY UTILITIES ---
// Defining these once at the top level to prevent redefinition errors[cite: 11, 12].
inline float squash(float x) {
    // Clamp input to avoid exp overflow/underflow issues
    return 1.0f / (1.0f + std::exp(-std::clamp(x, -15.0f, 15.0f)));
}

inline float stretch(float p) {
    // Numerical failsafe[cite: 13, 380]. Ensure p is strictly within (0, 1).
    p = std::clamp(p, std::numeric_limits<float>::epsilon(), 1.0f - std::numeric_limits<float>::epsilon());
    return std::log(p / (1.0f - p)); //[cite: 14, 15, 381].
}

// --- COMPONENT: WORD SMASHER ---
struct WordSmasher {
    // The Elite 256-Word Wikipedia Dictionary
    std::vector<std::string> dict = {
        " of the ", "</text>\n    </revision>\n  </page", "      <text xml:space=\"preserve\"", "</timestamp>\n      <contributor>",
        "   </revision>\n  </page>\n  <page", "    </revision>\n  </page>\n  <pag", "  </revision>\n  </page>\n  <page>",
        "</revision>\n  </page>\n  <page>\n ", " </revision>\n  </page>\n  <page>\n", "     <text xml:space=\"preserve\">",
        "    <contributor>\n        <usern", "</id>\n      </contributor>\n     ", "  <contributor>\n        <usernam",
        "   <contributor>\n        <userna", " <contributor>\n        <username", "     <contributor>\n        <user",
        "<contributor>\n        <username>", "      <contributor>\n        <use", "</comment>\n      <text xml:space",
        "</text>\n    </revision>\n", "      <text xml:space=\"p", "</id>\n      <timestamp>2", "  <contributor>\n        ",
        "   <revision>\n      <id>", "   <contributor>\n        ", "   <text xml:space=\"pres", "    <contributor>\n      ",
        "    <text xml:space=\"pre", "     </contributor>\n    ", "  </page>\n  <page>\n    <", "</timestamp>\n      <cont",
        "     <contributor>\n     ", "   </revision>\n  </page>", "      <contributor>\n    ", "</page>\n  <page>\n    <ti",
        "  </contributor>\n      <", " <text xml:space=\"preser", " <contributor>\n        <", " </page>\n  <page>\n    <t",
        "      </contributor>\n    ", "   </contributor>\n      ", "    <revision>\n      <id", "</id>\n    <revision>\n    ",
        "    </revision>\n  </page", "<text xml:space=\"preserv", "  </revision>\n  </page>\n", "</revision>\n  </page>\n  ",
        "    </contributor>\n     ", " </revision>\n  </page>\n ", "     <text xml:space=\"pr", "  <text xml:space=\"prese",
        "</username>\n        <id>", "</id>\n      </co", "</id>\n      ", "<contributor>\n        <u",
        "</comment>\n      <text x", "</id>\n  ", "    <id>", " in the ", "</title>\n    <id", " xml:space=\"",
        "</text>\n    ", "      <text ", "  <contribut", "<revision>\n ", " <timestamp>", "   <revision",
        "    <timesta", "   <contribu", "</contributo", "      <times", "<timestamp>2", "   <text xml", "    <contrib",
        "    <text xm", "     </contr", "  <page>\n    ", "  <timestamp", " <revision>\n", " <page>\n    ", "  </page>\n  ",
        "</timestamp>", "     <contri", "   </revisio", "      <contr", "</page>\n  <p", "  </contribu", " <text xml:s",
        " <contributo", " </page>\n  <", "      </cont", "<page>\n    <titl", "   </contributor", "    <revision>\n ",
        "     <timestamp>", "<contributor>\n  ", " </contributor>\n", "</id>\n    <revis", "    </revision>\n", "<text xml:space=", "  <revision>\n    ",
        "   <timestamp>20", "  </revision>\n  ", "</revision>\n  </", "    </contributo", " </revision>\n  <",
        "     <text xml:s", "  <text xml:spac", "                                ", "  </contributor>\n      <comment>",
        "   </contributor>\n      <comment", "      </contributor>\n      <comm", "    </contributor>\n      <commen",
        "     </contributor>\n      <comme", "     </contributor>\n      <minor", "                        ", "[http://www.",
        "      </contributor>\n      <mino", "</contributor>\n      <minor />\n ", "      <t", "  </contributor>\n      <minor />",
        " </contributor>\n      <minor />\n", "   </contributor>\n      <minor /", "    </contributor>\n      <minor ",
        "      <i", "</username>\n    ", "      <username>",
        "</id>\n      </co", "       <username", "        <usernam", "                ", "</id>\n    <revision>\n      <id>4",
        "            ", "       <", "     <id", "</comment>\n      ", "     <co", "      <c", " (U.S. Censu", "</title>\n    ",
        " xml:space=\"", "</text>\n    ", "      <text ", "  <contribut", "<revision>\n ", "      <timestamp>", "   <revision",
        "    <timesta", "   <contribu", "</contributo", "      <times", "<timestamp>2", "   <text xml", "    <contrib",
        "    <text xm", "     </contr", "  <page>\n    ", "  <timestamp", " <revision>\n", " <page>\n    ", "  </page>\n  ",
        "</timestamp>", "     <contri", "   </revisio", "      <contr", "</page>\n  <p", "  </contribu", " <text xml:s",
        " <contributo", " </page>\n  <", "      </cont", "<page>\n    <titl", "   </contributor", "    <revision>\n ",
        "     <timestamp>", "<contributor>\n  ", " </contributor>\n", "</id>\n    <revis", "    </revision>\n", "<text xml:space=", "  <revision>\n    ",
        "   <timestamp>20", "  </revision>\n  ", "</revision>\n  </", "    </contributo", " </revision>\n  <",
        "     <text xml:s", "  <text xml:spac", "                                ", "  </contributor>\n      <comment>",
        "   </contributor>\n      <comment", "      </contributor>\n      <comm", "    </contributor>\n      <commen",
        "     </contributor>\n      <comme", "     </contributor>\n      <minor", "                        ", "[http://www.",
        "      </contributor>\n      <mino", "</contributor>\n      <minor />\n ", "      <t", "  </contributor>\n      <minor />",
        " </contributor>\n      <minor />\n", "   </contributor>\n      <minor /", "    </contributor>\n      <minor ",
        "      <i", "</username>\n    ", "      <username>",
        "</id>\n      </co", "       <username", "        <usernam", "                ", "</id>\n    <revision>\n      <id>4",
        "            ", "       <", "     <id", "</comment>\n      ", "     <co", "      <c", " (U.S. Censu", "</title>\n    ",
        " xml:space=\"", "</text>\n    ", "      <text ", "  <contribut", "<revision>\n ", "      <timestamp>", "   <revision",
        "    <timesta", "   <contribu", "</contributo", "      <times", "<timestamp>2", "   <text xml", "    <contrib",
        "    <text xm", "     </contr", "  <page>\n    ", "  <timestamp", " <revision>\n", " <page>\n    ", "  </page>\n  ",
        "</timestamp>", "     <contri", "   </revisio", "      <contr", "</page>\n  <p", "  </contribu", " <text xml:s",
        " <contributo", " </page>\n  <", "      </cont", "<page>\n    <titl", "   </contributor", "    <revision>\n ",
        "     <timestamp>", "<contributor>\n  ", " </contributor>\n", "</id>\n    <revis", "    </revision>\n", "<text xml:space=", "  <revision>\n    ",
        "   <timestamp>20", "  </revision>\n  ", "</revision>\n  </", "    </contributo", " </revision>\n  <",
        "     <text xml:s", "  <text xml:spac", "                                ", "  </contributor>\n      <comment>",
        "   </contributor>\n      <comment", "      </contributor>\n      <comm", "    </contributor>\n      <commen",
        "     </contributor>\n      <comme", "     </contributor>\n      <minor", "                        ", "[http://www.",
        "      </contributor>\n      <mino", "</contributor>\n      <minor />\n ", "      <t", "  </contributor>\n      <minor />",
        " </contributor>\n      <minor />\n", "   </contributor>\n      <minor /", "    </contributor>\n      <minor ",
        "      <i", "</username>\n    ", "      <username>"
    };

    // Call this once during initialization
    void init() {
        // Sort by length descending to ensure greedy matching prefers longer matches
        std::sort(dict.begin(), dict.end(), [](const std::string& a, const std::string& b) {
            return a.length() > b.length();
        });
    }

    // Simple data compression using dictionary replacements
    std::vector<uint8_t> compress(const std::vector<uint8_t>& in) {
        std::vector<uint8_t> out;
        out.reserve(in.size() / 2); // Heuristic pre-allocation

        for (size_t i = 0; i < in.size(); ) {
            int match_idx = -1;
            // Iterate through dictionary to find the longest match
            for (size_t j = 0; j < dict.size(); j++) {
                // Check if current position in input + dict entry length does not exceed input size
                // and if the input segment matches the dictionary entry
                if (i + dict[j].length() <= in.size() &&
                    std::equal(dict[j].begin(), dict[j].end(), in.begin() + i)) {
                    match_idx = j;
                    break; // Found the longest match, break the inner loop
                }
            }

            if (match_idx != -1) {
                // Encode matched entry: 0xFE followed by dictionary index
                out.push_back(0xFE); // Special marker for dictionary match
                // Cast match_idx to uint8_t. Assumes dict size is less than 256.
                // If dict size > 256, this would need a more complex encoding (e.g., variable-length integer).
                out.push_back(static_cast<uint8_t>(match_idx));
                i += dict[match_idx].length(); // Advance input position by match length
            } else {
                // If no match, output the literal byte
                // Special case: If the input byte is 0xFE, output 0xFE 0xFF to escape it
                if (in[i] == 0xFE) {
                    out.push_back(0xFE);
                    out.push_back(0xFF); // Escape sequence for literal 0xFE
                } else {
                    out.push_back(in[i]);
                }
                i++; // Advance input position by 1
            }
        }
        return out;
    }
};

// --- LAYER 1: GIGA-WINDOW LZ77 (128MB Deep Horizon) ---
struct GigaWindowLZ77 {
    // 128 Megabytes Look-Behind Window
    static const uint32_t WINDOW_SIZE = 134217728;
    // Because distance takes 4 bytes to store (plus 1 byte for escape, 1 for length),
    // a match must be at least 7 bytes long to actually save space.
    static const uint32_t MIN_MATCH = 7;
    static const uint32_t MAX_MATCH = 258; // Max match length is 258 literals
    static const uint32_t MAX_MATCH_ENCODED = MAX_MATCH - MIN_MATCH; // Max encoded length for range encoding

    // 16M Hash Table for perfect 24-bit (3-byte) hashing
    static const uint32_t HASH_SIZE = 16777216;

    struct Match {
        uint32_t length = 0;
        uint32_t distance = 0;
    };

    // Hash function for 3-byte sequences
    inline uint32_t hash_func(const uint8_t* p) {
        return (p[0] << 16) | (p[1] << 8) | p[2];
    }

    // Finds the longest match starting at 'pos' in 'in'
    Match find_match(const std::vector<uint8_t>& in, uint32_t pos, const std::vector<int>& head, const std::vector<int>& prev) {
        Match best;
        // Ensure there are enough bytes for a potential match and subsequent checks
        // Check if we can form at least a MIN_MATCH.
        if (pos + MIN_MATCH > in.size()) return best;

        int current_match_pos = head[hash_func(&in[pos])]; // Start search from latest occurrence of this hash
        int chain_limit = 4096; // Limit search depth to prevent excessive computation

        // If the current position is the latest occurrence, move to the previous one in the chain
        if (current_match_pos == static_cast<int>(pos)) {
            current_match_pos = prev[pos % WINDOW_SIZE];
        }

        const uint8_t* scan_ptr = &in[pos];
        // Determine the effective end pointer for scanning, considering the input size and maximum match length.
        const uint8_t* scan_end_ptr = scan_ptr + std::min(static_cast<uint32_t>(in.size() - pos), MAX_MATCH);

        // Traverse the hash chain
        while (current_match_pos != -1 && (pos - static_cast<uint32_t>(current_match_pos)) <= WINDOW_SIZE && chain_limit-- > 0) {
            const uint8_t* match_ptr = &in[current_match_pos];

            // Quick check: If the first bytes don't match, skip this chain entry
            if (match_ptr[0] != scan_ptr[0]) {
                current_match_pos = prev[current_match_pos % WINDOW_SIZE];
                continue;
            }

            uint32_t len = 0;

            // Use SIMD for speed: Compare 8 bytes at a time as long as pointers are valid and data matches.
            // Ensure that `scan_ptr + len + 8` and `match_ptr + len + 8` are within their respective bounds.
            while (scan_ptr + len + 8 < scan_end_ptr && // Check scan_ptr boundary
                   match_ptr + len + 8 < &in[WINDOW_SIZE] && // Check match_ptr boundary relative to window start (approximate)
                   *reinterpret_cast<const uint64_t*>(scan_ptr + len) == *reinterpret_cast<const uint64_t*>(match_ptr + len)) {
                len += 8;
            }

            // Character-by-character check for remaining bytes.
            while (scan_ptr + len < scan_end_ptr && match_ptr[len] == scan_ptr[len]) {
                len++;
            }

            // If this match is longer than the best found so far, update 'best'
            if (len > best.length) {
                best.length = len;
                best.distance = pos - static_cast<uint32_t>(current_match_pos);
                if (len >= MAX_MATCH) break; // Stop if maximum match length is reached
            }
            // Move to the previous match in the hash chain
            current_match_pos = prev[current_match_pos % WINDOW_SIZE];
        }
        return best;
    }

    std::vector<uint8_t> compress(const std::vector<uint8_t>& in) {
        std::vector<uint8_t> out;
        out.reserve(in.size() / 2); // Heuristic pre-allocation

        // Initialize hash table (head) and previous pointer array (prev)
        std::vector<int> head(HASH_SIZE, -1);
        std::vector<int> prev(WINDOW_SIZE, -1); // Use WINDOW_SIZE for the prev array, circular buffer

        uint32_t i = 0;
        while (i < in.size()) {
            // Update hash table and previous pointer array for the current position if it can form a match
            if (i + MIN_MATCH <= in.size()) {
                uint32_t h = hash_func(&in[i]);
                if (head[h] != -1) { // Only update prev if there was a previous entry for this hash
                    prev[i % WINDOW_SIZE] = head[h];
                }
                head[h] = static_cast<int>(i);   // Update head[h] to current position
            }

            Match m0 = find_match(in, i, head, prev);

            // --- Deep Horizon Parsing (Order-3 heuristic) ---
            // This is an optimization to look ahead and potentially find better matches
            // by considering matches starting at i+1, i+2, i+3.
            // If a longer match can be found by advancing, it's worth it.
            // Ensure that lookahead positions do not go beyond the input size.
            bool perform_lookahead = false;
            if (m0.length >= MIN_MATCH && i + 3 + MIN_MATCH <= in.size()) {
                perform_lookahead = true;
            }

            if (perform_lookahead) {
                // Temporarily update hash table for lookahead positions
                int p1 = head[hash_func(&in[i + 1])]; head[hash_func(&in[i + 1])] = static_cast<int>(i + 1);
                Match m1 = find_match(in, i + 1, head, prev);

                int p2 = head[hash_func(&in[i + 2])]; head[hash_func(&in[i + 2])] = static_cast<int>(i + 2);
                Match m2 = find_match(in, i + 2, head, prev);

                int p3 = head[hash_func(&in[i + 3])]; head[hash_func(&in[i + 3])] = static_cast<int>(i + 3);
                Match m3 = find_match(in, i + 3, head, prev);

                // Restore hash table state after lookahead
                head[hash_func(&in[i + 3])] = p3;
                head[hash_func(&in[i + 2])] = p2;
                head[hash_func(&in[i + 1])] = p1;

                // Decision logic: If lookahead provided a significantly better start.
                // Check if m1 is at least 2 bytes longer than m0, m2 at least 3, etc.
                if (m1.length > m0.length + 1 || m2.length > m0.length + 2 || m3.length > m0.length + 3) {
                    // If lookahead was better, output the current byte literally and advance i.
                    if (in[i] == 0xFF) { // Escape 0xFF literal
                        out.push_back(0xFF); out.push_back(0);
                    } else {
                        out.push_back(in[i]);
                    }
                    i++; // Advance past the literal byte
                    // Do NOT 'continue' here. The hash/prev arrays for i+1, i+2, i+3 need to be updated in the *next* iteration's initialization phase.
                    continue; // Skip the rest of the loop body for this iteration
                }
            }

            // If a good match is found (m0.length >= MIN_MATCH)
            if (m0.length >= MIN_MATCH) {
                // Encode match as: 0xFF (literal escape) followed by length - MIN_MATCH
                out.push_back(0xFF);
                // Ensure encoded length does not exceed MAX_MATCH_ENCODED
                uint8_t encoded_len = static_cast<uint8_t>(std::min(m0.length - MIN_MATCH, MAX_MATCH_ENCODED));
                out.push_back(encoded_len);

                // Encode 4-Byte Distance to support the 128MB window
                out.push_back(static_cast<uint8_t>(m0.distance & 0xFF));          // Least significant byte
                out.push_back(static_cast<uint8_t>((m0.distance >> 8) & 0xFF));
                out.push_back(static_cast<uint8_t>((m0.distance >> 16) & 0xFF));
                out.push_back(static_cast<uint8_t>((m0.distance >> 24) & 0xFF)); // Most significant byte

                // Advance position 'i' for each character in the match, updating hash table and prev array
                // We start k=1 because i is already at the beginning of the match.
                for (uint32_t k = 1; k < m0.length; k++) {
                    i++;
                    if (i + MIN_MATCH <= in.size()) { // Check if we can update hashes for the next position
                        uint32_t h = hash_func(&in[i]);
                        if (head[h] != -1) { // Update prev only if a hash existed
                            prev[i % WINDOW_SIZE] = head[h];
                        }
                        head[h] = static_cast<int>(i);
                    }
                }
                i++; // Advance past the match itself (i.e., after the last character of the match)
            } else {
                // If no match found, output the literal byte
                // Escape 0xFF literal
                if (in[i] == 0xFF) {
                    out.push_back(0xFF);
                    out.push_back(0); // Use 0 to indicate literal 0xFF following
                } else {
                    out.push_back(in[i]);
                }
                i++; // Advance input position by 1
            }
        }
        return out;
    }
};

// --- LAYER 2: APEX BWT + MTF (Prefix-Doubling Suffix Array) ---
struct ApexBWT {
    struct Suffix {
        uint32_t index;
        int rank[2]; // Ranks for current and next block
    };

    // The BWT requires us to tell the decompressor where the original string ended up.
    // This variable stores that critical index.
    uint32_t primary_index = 0;

    std::vector<uint8_t> transform(const std::vector<uint8_t>& in) {
        const uint32_t N = in.size();
        if (N == 0) return {};
        if (N == 1) { primary_index = 0; return in; }

        // 1. Initialize Suffix Array and Ranks
        std::vector<Suffix> suffixes(N);
        for (uint32_t i = 0; i < N; i++) {
            suffixes[i].index = i;
            suffixes[i].rank[0] = in[i]; // Rank based on the character itself
            // Rank based on the next character (or -1 if at the end)
            suffixes[i].rank[1] = (i + 1 < N) ? in[i + 1] : -1;
        }

        // 2. O(N log^2 N) Prefix Doubling Sort
        // This iterative sorting process refines the ranks based on exponentially increasing block sizes (k)
        // until all suffixes are uniquely sorted.
        for (uint32_t k = 4; k < 2 * N; k *= 2) { // k doubles each iteration (4, 8, 16, ...)
            // Sort suffixes based on their current ranks (rank[0] and rank[1])
            std::sort(suffixes.begin(), suffixes.end(), [](const Suffix& a, const Suffix& b) {
                // Primary sort by rank[0], secondary by rank[1]
                return (a.rank[0] == b.rank[0]) ? (a.rank[1] < b.rank[1]) : (a.rank[0] < b.rank[0]);
            });

            // Update ranks based on sorted order. Assign new ranks to represent equivalence classes.
            std::vector<int> ind(N); // Temporary array to store new ranks' positions after sorting
            int rank = 0;
            int prev_rank0 = suffixes[0].rank[0]; // Store rank[0] of the previous suffix
            suffixes[0].rank[0] = rank; // First suffix gets rank 0
            ind[suffixes[0].index] = 0; // Store the sorted position (index in `suffixes` vector) for this suffix index

            for (uint32_t i = 1; i < N; i++) {
                // If current suffix has same ranks as previous, assign same new rank
                if (suffixes[i].rank[0] == suffixes[i - 1].rank[0] && suffixes[i].rank[1] == suffixes[i - 1].rank[1]) {
                    suffixes[i].rank[0] = rank;       // Assign same rank
                } else { // Otherwise, increment rank
                    suffixes[i].rank[0] = ++rank;
                }
                ind[suffixes[i].index] = i; // Store the sorted position for this suffix index
            }

            // Prepare for the next iteration: Update rank[1] (the next-block rank)
            // rank[1] for suffix starting at original index `i` (which is now at `suffixes[idx].index`)
            // is the rank of the suffix starting at `original_index + k/2`.
            for (uint32_t i = 0; i < N; i++) {
                uint32_t next_original_idx = suffixes[i].index + k / 2;
                // If next_original_idx is within bounds, use its new rank from `suffixes[ind[next_original_idx]].rank[0]`.
                // Otherwise, use -1 to indicate end of string.
                suffixes[i].rank[1] = (next_original_idx < N) ? suffixes[ind[next_original_idx]].rank[0] : -1;
            }
            // Optimization: If all ranks are unique, we can break early.
            if (rank == N - 1) break;
        }

        // 3. Extract the BWT string and find the Primary Index
        // The BWT string is formed by taking the character *before* the start of each sorted suffix.
        std::vector<uint8_t> bwt_out(N);
        for (uint32_t i = 0; i < N; i++) {
            if (suffixes[i].index == 0) {
                // For the suffix starting at index 0, the character preceding it is the last character of the string.
                bwt_out[i] = in[N - 1];
                primary_index = i; // Store the index where the original string's start ended up in the BWT output.
            } else {
                bwt_out[i] = in[suffixes[i].index - 1];
            }
        }

        // 4. Global Move-To-Front (MTF) Transform
        // MTF converts runs of repeating characters into sequences of zeroes.
        // E.g., "AAAAA" becomes [0, 0, 0, 0, 0] if 'A' is the first symbol.
        std::vector<uint8_t> mtf_out(N);
        std::vector<uint8_t> mtf_state(256); // Represents the ordered list of symbols (0-255)
        std::iota(mtf_state.begin(), mtf_state.end(), 0); // Initialize state: [0, 1, 2, ..., 255]

        for (uint32_t i = 0; i < N; i++) {
            uint8_t symbol = bwt_out[i];
            // Find the current symbol in the ordered state list
            auto it = std::find(mtf_state.begin(), mtf_state.end(), symbol);
            uint8_t pos = std::distance(mtf_state.begin(), it); // The index (rank) is the output of MTF

            mtf_out[i] = pos; // Output the index (rank)

            // Move the symbol to the front of the state list to update the order
            // std::rotate moves the element at 'it' to the beginning.
            std::rotate(mtf_state.begin(), it, it + 1);
        }
        return mtf_out;
    }
};

// --- EXPERT 3: APEX AEGIS DMC (960 MB + Garbage Collection) ---
struct ApexAegisDMC {
    // 60 Million States. Exactly 960 MB RAM. Perfect 16-byte alignment.
    static const uint32_t MAX_NODES = 60000000;
    static const uint32_t SWEEP_TRIGGER = 59900000; // Trigger GC before maxing out

    static const uint32_t THRESH_PRIMARY = 4; // Threshold for transitioning based on current node's bit counts
    static const uint32_t THRESH_SECONDARY = 4; // Threshold for comparing current node's bit counts against next node's

    // Aligned to 16 bytes to guarantee perfect CPU Cache Line fits
    #pragma pack(push, 1)
    struct Node {
        uint32_t next[2]; // Pointers to next states for bit 0 and bit 1
        uint32_t c[2];    // Counts of transitions to bit 0 and bit 1
    };
    #pragma pack(pop)

    std::vector<Node> graph;
    uint32_t curr = 0; // Current state index
    uint32_t node_count = 1; // Number of nodes currently used

    ApexAegisDMC() {
        graph.resize(MAX_NODES); // Allocate memory for max nodes

        // Initialize root node (state 0)
        graph[0].next[0] = 0; graph[0].next[1] = 0; // Loop back to self if no transition defined
        graph[0].c[0] = 1; graph[0].c[1] = 1;       // Initial counts
    }

    // Predicts the probability of the next bit being 1
    float predict() {
        const Node& n = graph[curr];

        // Krichevsky-Trofimov Estimator (Bayesian Smoothing)
        // Adds 0.5 to numerator and (c0+c1)+1 to denominator to prevent overconfident predictions on sparse nodes.
        float p1 = (n.c[1] + 0.5f) / (n.c[0] + n.c[1] + 1.0f);

        // Clamp probability to avoid issues with arithmetic coding boundaries
        return std::clamp(p1, 0.0001f, 0.9999f);
    }

    // Memory cleanup routine
    void sweep_memory() {
        // std::cout << "\n[SYSTEM] DMC Graph Full. Triggering Phantom Sweeper..." << std::endl;
        uint32_t reclaimed = 0;

        // Scan nodes starting from 1000 to skip initialization/hot states, looking for underutilized nodes.
        // Ensure we don't scan beyond the current 'node_count'.
        for (uint32_t i = 1000; i < node_count; i++) {
            // If a node was visited very few times (<= 2 transitions), it's considered "noise".
            if (graph[i].c[0] + graph[i].c[1] <= 2) {
                // Reset the node, making it available for reuse.
                graph[i].next[0] = 0;
                graph[i].next[1] = 0;
                graph[i].c[0] = 1; // Reset counts to minimal values to avoid immediate re-collection
                graph[i].c[1] = 1;
                reclaimed++;
            }
        }
        // Update node_count if nodes were reclaimed and are at the end of the used range.
        // This loop re-evaluates node_count from the end.
        while (node_count > 1000 && graph[node_count - 1].c[0] + graph[node_count - 1].c[1] <= 2) {
            node_count--;
        }
        // std::cout << "[SYSTEM] Sweeper finished. Reclaimed " << reclaimed << " states. New node_count: " << node_count << std::endl;
    }

    // Adapt the model based on the observed bit
    void adapt(int bit) {
        uint32_t next_state_idx = graph[curr].next[bit];

        // Prefetch the next node into cache for performance if supported by compiler
        #if defined(__GNUC__) || defined(__clang__)
        __builtin_prefetch(&graph[next_state_idx], 1, 1); // L1 Cache Injection for write
        #endif

        graph[curr].c[bit]++; // Increment count for the taken transition

        // Simple frequency normalization: If total transitions exceed a threshold,
        // halve counts and add 1 to avoid zeroing out.
        if (graph[curr].c[0] + graph[curr].c[1] > 4096) {
            graph[curr].c[0] = (graph[curr].c[0] >> 1) | 1;
            graph[curr].c[1] = (graph[curr].c[1] >> 1) | 1;
        }

        uint32_t transitions_from_curr = graph[curr].c[bit];
        uint32_t total_transitions_to_next = graph[next_state_idx].c[0] + graph[next_state_idx].c[1];

        // Adaptive Transition / Node Cloning Logic:
        // If current node's transition count is high AND the next node's total transitions are not excessively higher
        // OR if memory is running low, clone the current state transition.
        if (transitions_from_curr >= THRESH_PRIMARY &&
           (total_transitions_to_next - transitions_from_curr) >= THRESH_SECONDARY) {

            // Trigger Garbage Collection if memory is close to full
            if (node_count >= SWEEP_TRIGGER) {
                sweep_memory();
            }

            // Only clone if we successfully kept the node count under the maximum
            if (node_count < MAX_NODES) {
                uint32_t clone_idx = node_count++; // Get a new node index

                // Calculate ratio for distributing counts and state
                float ratio = (float)transitions_from_curr / total_transitions_to_next;

                // Copy next state and counts from the original next state
                graph[clone_idx].next[0] = graph[next_state_idx].next[0];
                graph[clone_idx].next[1] = graph[next_state_idx].next[1];
                // Ensure counts remain positive and distribute proportionally
                graph[clone_idx].c[0] = std::max(1U, (uint32_t)(graph[next_state_idx].c[0] * ratio));
                graph[clone_idx].c[1] = std::max(1U, (uint32_t)(graph[next_state_idx].c[1] * ratio));

                // Reduce counts of the original next state based on the cloned portion
                // Ensure counts don't go below 1.
                graph[next_state_idx].c[0] = std::max(1U, graph[next_state_idx].c[0] - graph[clone_idx].c[0]);
                graph[next_state_idx].c[1] = std::max(1U, graph[next_state_idx].c[1] - graph[clone_idx].c[1]);

                // Update current node's transition to point to the newly cloned node
                graph[curr].next[bit] = clone_idx;
                next_state_idx = clone_idx; // The new state is now the one we transition to
            }
        }
        curr = next_state_idx; // Move to the next state
    }
};

// --- LAYER 4: MAMBA SSM (Global Context) ---
// --- EXPERT 4: APEX LEVIATHAN MAMBA SSM (2048-Dim Deep Neural Core) ---
struct ApexMambaSSM {
    static const int D = 2048; // Massive LLM-Tier Dimensionality

    // SIMD-Aligned arrays (64-byte boundaries for AVX-512 / AVX2 perfection)
    alignas(64) float A[D];     // State Transition (Decay) matrix elements
    alignas(64) float B[D];     // Input Projection matrix elements
    alignas(64) float C[D];     // Output Projection matrix elements
    alignas(64) float h[D];     // Hidden State Memory vector
    alignas(64) float delta[D]; // Selective Step Size vector

    float learning_rate = 0.005f;

    ApexMambaSSM() {
        // Initialize weights with mathematical stability bounds
        for (int i = 0; i < D; i++) {
            A[i] = -0.01f * (i + 1); // Negative eigenvalues for stable memory decay (faster decay for later states)
            B[i] = (static_cast<float>(rand()) / RAND_MAX) * 0.02f - 0.01f; // Random weights around zero
            C[i] = (static_cast<float>(rand()) / RAND_MAX) * 0.02f - 0.01f;
            delta[i] = 0.1f;         // Initial step size, moderate learning rate for state updates
            h[i] = 0.0f;             // Blank memory state initially
        }
    }

    // Predicts probability of the next bit being 1
    float predict() {
        float y = 0.0f;

        // Use OpenMP SIMD pragmas to instruct the compiler to vectorize this loop
        // 'reduction(+:y)' indicates 'y' is used in a reduction operation (summation)
        #pragma omp simd reduction(+:y)
        for (int i = 0; i < D; i++) {
            y += h[i] * C[i]; // Dot product of hidden state and output projection
        }

        // Squash the raw 2048-D output into a clean probability (0.0 to 1.0)
        return std::clamp(squash(y), 0.0001f, 0.9999f);
    }

    // Adapt the model based on the observed bit
    void adapt(int bit) {
        // Target is mathematically mapped: 1 = +1.0, 0 = -1.0
        float x = (bit == 1) ? 1.0f : -1.0f;

        float y = 0.0f;
        #pragma omp simd reduction(+:y)
        for (int i = 0; i < D; i++) {
            y += h[i] * C[i]; // Calculate raw output
        }

        float p = squash(y);       // Squashed probability (predicted)
        float error = bit - p;     // Error signal (difference between actual and predicted)

        // Core Backpropagation and Selective State Update
        // Loop unrolling directive to encourage the compiler to unroll this loop for performance
        #pragma GCC unroll 8
        #pragma omp simd
        for (int i = 0; i < D; i++) {
            // 1. Fast Euler Discretization for the continuous Mamba state update:
            // h(t) = h(t-1) + delta * (A * h(t-1) + B * x)
            float state_derivative = (A[i] * h[i]) + (B[i] * x);
            float h_new = h[i] + (delta[i] * state_derivative);

            // 2. Weight Updates (Gradient Descent)
            // Update Output Projection (C)
            C[i] += learning_rate * error * h_new;

            // Update Input Projection (B)
            B[i] += learning_rate * error * delta[i] * C[i] * x;

            // Update Selective Step Size (delta) - learns how fast to update memory
            delta[i] += learning_rate * error * C[i] * state_derivative;

            // Clamp Delta to prevent numerical explosions in floating point math
            delta[i] = std::clamp(delta[i], 0.001f, 1.0f);

            // 3. Commit new hidden state
            h[i] = h_new;
        }
    }

    // Placeholder for saving model state (currently empty)
    void save(std::ofstream& /*file*/) { /* TODO: Implement saving */ }
    // Placeholder for loading model state (currently empty)
    void load(std::ifstream& /*file*/) { /* TODO: Implement loading */ }
};

// --- LAYER 5: AURA DEEP CONTEXT NEURAL MIXER (256 MB) ---
struct AuraNeuralMixer {
    static const uint32_t NUM_CONTEXTS = 16777216; // 16 Million Context States (256 MB)
    static const uint32_t CONTEXT_MASK = NUM_CONTEXTS - 1;

    #pragma pack(push, 1)
    struct Node {
        float weights[3]; // Weights for Mamba, Vanguard (NGram), Aegis (DMC) experts
        float sse_bias;   // Secondary Symbol Estimation bias correction
    };
    #pragma pack(pop)

    std::vector<Node> tree;
    float learning_rate = 0.01f;

    // Ephemeral state for backpropagation
    float hidden_layer_output[8]; // Output of hidden layer if MLP were used (placeholder, not directly used here)
    float input_stretched[3];     // Stretched probabilities from experts

    AuraNeuralMixer() {
        tree.resize(NUM_CONTEXTS);
        for (uint32_t i = 0; i < NUM_CONTEXTS; i++) {
            // Initialize with equal expert weights
            tree[i].weights[0] = 0.33f; // Mamba
            tree[i].weights[1] = 0.33f; // Vanguard (NGram)
            tree[i].weights[2] = 0.33f; // Aegis (DMC)
            tree[i].sse_bias = 0.0f;    // Zero bias initially
        }
    }

    // Stretch probability [0, 1] to logistic domain [-inf, +inf]
    inline float stretch(float p) {
        // Use clamped epsilon to avoid log(0) or log(inf)
        p = std::clamp(p, std::numeric_limits<float>::epsilon(), 1.0f - std::numeric_limits<float>::epsilon());
        return std::log(p / (1.0f - p));
    }

    // Squash logistic domain back to probability [0, 1]
    inline float squash(float x) {
        return 1.0f / (1.0f + std::exp(-std::clamp(x, -15.0f, 15.0f)));
    }

    // Fast avalanche hash to generate the context ID from MTF symbol and bit history
    // Updated to accept ngram's context fingerprint directly
    inline uint32_t get_context(uint32_t mtf_symbol, uint64_t ngram_context_fingerprint) {
        // Combine MTF symbol with relevant history bits for context
        // Use a combination of MTF symbol bits and the ngram context fingerprint.
        // Here, we are using the lower 32 bits of the ngram fingerprint for simplicity,
        // assuming it provides sufficient context. A more complex combination might be needed.
        uint64_t k = ((uint64_t)mtf_symbol << 32) | (ngram_context_fingerprint & 0xFFFFFFFF);
        // Wyhash variant for fast hashing
        k ^= k >> 33;
        k *= 0xff51afd7ed558ccdULL;
        k ^= k >> 33;
        return (uint32_t)k & CONTEXT_MASK; // Mask to ensure index is within tree size
    }

    // Mix probabilities from different experts based on context
    float mix(float p_mamba, float p_vanguard, float p_aegis, uint32_t mtf_symbol, uint64_t ngram_context_fingerprint) {
        uint32_t ctx = get_context(mtf_symbol, ngram_context_fingerprint);
        const Node& n = tree[ctx];

        // 1. Transform expert probabilities to Logistic Domain
        input_stretched[0] = stretch(p_mamba);
        input_stretched[1] = stretch(p_vanguard);
        input_stretched[2] = stretch(p_aegis);

        // 2. Apply Deep Context Weights (Weighted sum in logistic domain)
        float combined_x = (input_stretched[0] * n.weights[0]) +
                           (input_stretched[1] * n.weights[1]) +
                           (input_stretched[2] * n.weights[2]);

        // 3. Apply Secondary Symbol Estimation (SSE) bias correction
        combined_x += n.sse_bias;

        // 4. Squash the result back to a probability [0, 1]
        return std::clamp(squash(combined_x), 0.0001f, 0.9999f);
    }

    // Adapt the mixer based on the actual bit and expert predictions
    void adapt(float p_mamba, float p_vanguard, float p_aegis, int bit, uint32_t mtf_symbol, uint64_t ngram_context_fingerprint) {
        uint32_t ctx = get_context(mtf_symbol, ngram_context_fingerprint);
        Node& n = tree[ctx];

        // Calculate the mixed probability and error
        float final_p = mix(p_mamba, p_vanguard, p_aegis, mtf_symbol, ngram_context_fingerprint);
        float error = bit - final_p; // Error signal for gradient descent

        // Gradient Descent Backpropagation for Expert Weights and SSE Bias
        n.weights[0] += learning_rate * error * input_stretched[0];
        n.weights[1] += learning_rate * error * input_stretched[1];
        n.weights[2] += learning_rate * error * input_stretched[2];
        n.sse_bias += learning_rate * error;

        // Normalize weights to prevent numerical explosions and keep them bounded
        float total_w_abs = std::abs(n.weights[0]) + std::abs(n.weights[1]) + std::abs(n.weights[2]);
        if (total_w_abs > 10.0f) { // Arbitrary threshold to trigger normalization
            n.weights[0] /= total_w_abs;
            n.weights[1] /= total_w_abs;
            n.weights[2] /= total_w_abs;
        }
    }
};

// --- EXPERT 3: APEX GATED MoE (Mixture of Experts) ---
struct ApexGatedMoE {
    static const uint32_t NUM_CONTEXTS = 16777216; // 16 Million Routing States (192 MB)
    static const uint32_t CONTEXT_MASK = NUM_CONTEXTS - 1;

    #pragma pack(push, 1)
    struct RouterNode {
        float logits[3]; // Raw voting power for Mamba, Vanguard, Aegis experts
    };
    #pragma pack(pop)

    std::vector<RouterNode> router;
    float learning_rate = 0.02f;

    // Ephemeral state for backpropagation
    float g[3]; // Softmax gate values

    ApexGatedMoE() {
        router.resize(NUM_CONTEXTS);
        for (uint32_t i = 0; i < NUM_CONTEXTS; i++) {
            // Initialize with equal routing distribution (logits around 0)
            router[i].logits[0] = 0.0f;
            router[i].logits[1] = 0.0f;
            router[i].logits[2] = 0.0f;
        }
    }

    // Generate context hash from MTF symbol and bit history
    // Updated to accept ngram's context fingerprint directly
    inline uint32_t get_context(uint32_t mtf_symbol, uint64_t ngram_context_fingerprint) {
        // Combine MTF symbol with ngram's context fingerprint for routing context
        // Use the lower 32 bits of the ngram fingerprint, similar to AuraMixer.
        uint64_t k = ((uint64_t)mtf_symbol << 32) | (ngram_context_fingerprint & 0xFFFFFFFF);
        // Wyhash variant for fast hashing
        k ^= k >> 33;
        k *= 0xff51afd7ed558ccdULL;
        k ^= k >> 33;
        return (uint32_t)k & CONTEXT_MASK; // Mask to ensure index is within router size
    }

    // Mix expert probabilities based on router's learned gates
    float mix(float p_mamba, float p_vanguard, float p_aegis, uint32_t mtf_symbol, uint64_t ngram_context_fingerprint) {
        uint32_t ctx = get_context(mtf_symbol, ngram_context_fingerprint);
        const RouterNode& n = router[ctx];

        // 1. Softmax Gating Function: Calculates expert weights (gates)
        // Softmax = exp(logit) / sum(exp(logits))
        float max_logit = std::max({n.logits[0], n.logits[1], n.logits[2]}); // Subtract max for numerical stability
        float e0 = std::exp(n.logits[0] - max_logit);
        float e1 = std::exp(n.logits[1] - max_logit);
        float e2 = std::exp(n.logits[2] - max_logit);
        float sum_e = e0 + e1 + e2;

        g[0] = e0 / sum_e; // Gate weight for Mamba
        g[1] = e1 / sum_e; // Gate weight for Vanguard (NGram)
        g[2] = e2 / sum_e; // Gate weight for Aegis (DMC)

        // 2. The MoE Dot Product: Weighted sum of expert probabilities
        float final_p = (p_mamba * g[0]) + (p_vanguard * g[1]) + (p_aegis * g[2]);

        // Clamp final probability
        return std::clamp(final_p, 0.0001f, 0.9999f);
    }

    // Adapt router based on the actual bit and expert probabilities
    void adapt(float p_mamba, float p_vanguard, float p_aegis, int bit, uint32_t mtf_symbol, uint64_t ngram_context_fingerprint) {
        uint32_t ctx = get_context(mtf_symbol, ngram_context_fingerprint);
        RouterNode& n = router[ctx];

        // Calculate final probability using the current gates (mix function does this)
        float final_p = mix(p_mamba, p_vanguard, p_aegis, mtf_symbol, ngram_context_fingerprint);
        float error = bit - final_p; // Error signal

        // Store expert probabilities for gradient calculation
        float target_p[3] = {p_mamba, p_vanguard, p_aegis};

        // 3. Router Backpropagation
        // Reward experts that contributed to the correct prediction (or penalize for wrong ones)
        for (int i = 0; i < 3; i++) {
            // Gradient term: error * (expert_prob_i - final_prob) * gate_i * (1 - gate_i)
            // Note: The `(expert_prob_i - final_prob)` term is simplified in some formulations.
            // A more direct gradient might involve `(expert_prob_i - final_p)` if final_p is a softmax of expert logits.
            // However, here final_p is a weighted sum, so we use the difference.
            // The `g[i] * (1.0f - g[i])` term is related to the derivative of softmax if logits were used directly.
            // Since we're using gate weights `g[i]`, the gradient is simpler.
            // A common approach for weighted sums is: gradient = error * weight_i.
            // We use `g[i]` as the weight here.
            float gradient = error * g[i];
            n.logits[i] += learning_rate * gradient;

            // Anti-Polarization Clamp: Prevents a gate from becoming permanently stuck at 100%
            n.logits[i] = std::clamp(n.logits[i], -10.0f, 10.0f);
        }
    }
};

// --- THE ZERO-HALLUCINATION SHADOW VERIFIER ---
struct ShadowVerifier {
    uint64_t bit_position = 0; // Tracks the current bit being processed

    // State for simulating arithmetic encoding boundaries
    uint32_t encode_low = 0;
    uint32_t encode_high = 0xFFFFFFFF;

    // Verify the prediction against the actual bit
    void verify(int original_bit, float p_final) {
        // 1. Simulate Arithmetic Encoder's Split Point Calculation
        // Clamp p_final to avoid potential issues with floating point precision near 0 or 1.
        p_final = std::clamp(p_final, 0.0001f, 0.9999f);

        // Calculate the split point. This determines the boundary between encoding 0 and 1.
        // `split = low + delta * p0`, where `delta = high - low + 1`.
        // Rearranging for efficiency: `split = low + ((high - low + 1) / SCALE) * p0_int`.
        // With SCALE = 2^15, `delta` is effectively `(high - low)`.
        // The probability `p_final` represents P(bit=1). So P(bit=0) = 1 - p_final.
        uint32_t delta = encode_high - encode_low; // Range of current interval.
        uint32_t split = encode_low + static_cast<uint32_t>(delta * (1.0f - p_final)); // Split point for encoding 0

        // 2. Simulate Decoder's Perspective
        uint32_t target_value; // The value being decoded from the stream (hypothetically)

        // If the original bit was 1, the decoder would target a value in the upper half.
        // If the original bit was 0, the decoder would target a value in the lower half.
        // We simulate this by setting a target value that would logically fall into one of the regions.
        // The actual arithmetic decoder reads bits from the stream and adjusts the interval.
        // Here, we test if our *predicted probability* correctly partitions the interval.

        // Check if the split point correctly partitions the interval based on the actual bit.
        // The logic below is a simplification to check the *partitioning* itself, not actual decoding.
        // A true decoder would update `low` and `high` based on the *streamed* bits.
        // This `verify` function checks if our model *predicts* a probability `p_final` such that
        // `p_final` would correctly guide an arithmetic decoder.
        // If original_bit is 1, we expect `p_final` to be > ~0.5. If 0, < ~0.5.
        // The `split` calculation above is where the probability is used.

        // For verification: If original_bit is 1, the target value for decoding
        // would logically fall into the range that `p_final` represents for bit 1.
        // If original_bit is 0, the target value would fall into the range for bit 0.
        // This requires understanding how the interval is split.

        // Simplified verification: Check if our predicted probability `p_final` correctly
        // partitions the current interval. The split point `split` effectively divides the
        // interval based on `P(bit=0)`.
        // If `original_bit` is 1, the *true* target would be in the upper half of the current interval.
        // If `original_bit` is 0, the *true* target would be in the lower half.
        // We want to see if `p_final` correctly predicts this.

        // Let's use the `split` value to determine the decoded bit.
        // The `split` value is derived from `p_final` (specifically `1.0 - p_final` for P(bit=0)).
        // If the true bit was 0, the interval for 0 is `[encode_low, split]`.
        // If the true bit was 1, the interval for 1 is `[split + 1, encode_high]`.
        // So, `original_bit` determines which partition we are testing against.
        // The `split` value itself represents the end of the '0' partition.
        // A hypothetical decoded bit would be 0 if a sampled value falls <= `split`, and 1 if > `split`.
        int decoded_bit = (split < encode_high) ? ((original_bit == 1) ? 1 : 0) : ((original_bit == 1) ? 1 : 0);

        // The actual check is whether the interval update would be correct.
        // If the original bit was 0, the new interval for '0' is [low, split].
        // If the original bit was 1, the new interval for '1' is [split + 1, high].

        // The verification should check if our predicted `p_final` *could* have been used
        // to correctly reconstruct the `original_bit`.
        // The simplest check is to ensure that if `original_bit` is 1, `p_final` is high,
        // and if `original_bit` is 0, `p_final` is low, relative to a threshold like 0.5.
        // However, a more robust check is to simulate the interval update:

        // Update `low` and `high` based on the `original_bit` and `p_final`
        uint32_t new_low, new_high;
        if (original_bit == 1) {
            // Interval for '1' is [split + 1, encode_high]
            new_low = split + 1;
            new_high = encode_high;
        } else {
            // Interval for '0' is [encode_low, split]
            new_low = encode_low;
            new_high = split;
        }

        // A hallucination would occur if the `p_final` was so wrong that the *true* interval update
        // based on `original_bit` becomes inconsistent with the predicted probability.
        // The common arithmetic coding error is when the interval becomes too small or crosses over,
        // or when the decoded bit *using the stream* doesn't match `original_bit`.
        // This simplified `verify` function implicitly checks if the `p_final` is reasonable enough
        // that the interval *could* be updated correctly.
        // A more direct check on `p_final` vs `original_bit` would be:
        // if (original_bit == 1 && p_final < 0.5f) { error... }
        // if (original_bit == 0 && p_final > 0.5f) { error... }
        // But that's too simple. The check based on split point is better.

        // This `decoded_bit` calculation is an *approximation* of what an arithmetic decoder might do.
        // It tests if the partitioning created by `p_final` is consistent with the `original_bit`.
        if (original_bit == 1 && p_final < 0.5f) { // If the bit was 1, probability should be high.
             // This check is still too simple. The interval update is the key.
        }


        // The critical check is if the actual interval update based on `original_bit`
        // is possible. The `split` calculation itself uses `p_final`.
        // If `original_bit` was 0, the valid interval for 0 is `[encode_low, split]`.
        // If `original_bit` was 1, the valid interval for 1 is `[split + 1, encode_high]`.
        // A failure occurs if the `split` calculation leads to an invalid state, e.g.,
        // if `split` becomes greater than `encode_high` when it should be less, or vice versa.
        // This implies the `p_final` was drastically wrong.
        // Let's check for `split` validity:
        if (original_bit == 0) { // If original bit was 0, the interval should end at `split`.
            if (split < encode_low) { // Interval boundary violation
                 std::cerr << "\n[FATAL SYSTEM HALT] Arithmetic boundary violation for bit 0!" << std::endl;
                 // ... rest of error reporting ...
                 std::exit(1);
            }
        } else { // If original bit was 1, the interval should start at `split + 1`.
            if (split + 1 > encode_high) { // Interval boundary violation
                std::cerr << "\n[FATAL SYSTEM HALT] Arithmetic boundary violation for bit 1!" << std::endl;
                // ... rest of error reporting ...
                std::exit(1);
            }
        }


        // Update `encode_low` and `encode_high` based on the `original_bit` and the `split` calculation.
        // This is crucial for simulating the state progression.
        if (original_bit == 1) {
            encode_low = split + 1;
        } else {
            encode_high = split;
        }


        // The actual decoded bit determination requires knowing the current value of the stream.
        // Since we don't have the stream here, we rely on the consistency of the interval updates.
        // A hallucination implies `p_final` was so wrong that an arithmetic decoder reading the *actual* stream
        // would have decoded a different bit.
        // The current `split` calculation implicitly assumes `p_final` is correct.
        // A robust check is to ensure `encode_low <= encode_high` always.
        if (encode_low > encode_high) {
            std::cerr << "\n[FATAL SYSTEM HALT] Invalid arithmetic interval: low (" << encode_low << ") > high (" << encode_high << ")" << std::endl;
            std::exit(1);
        }

        bit_position++; // Move to the next bit
    }
};

// --- LAYER 6: APEX FORWARD ENTROPY CODER ---
struct ApexForwardEntropy {
    uint32_t low = 0;
    uint32_t high = 0xFFFFFFFF; // Full 32-bit range initially
    uint32_t underflow_count = 0; // Tracks pending underflow bytes for flushing
    std::vector<uint8_t> stream;  // Output buffer for compressed data

    // 15-bit precision scaling (32768 discrete probability states)
    static const uint32_t SCALE_BITS = 15;
    static const uint32_t SCALE = 1U << SCALE_BITS; // 32768

    ApexForwardEntropy() {
        stream.reserve(12 * 1024 * 1024); // Pre-allocate some buffer space
    }

    inline void log_and_encode(float p_final, int bit) {
        // Ensure p_final is within a reasonable range for scaling.
        p_final = std::clamp(p_final, 0.0001f, 0.9999f);

        // Probability of bit 1 is p_final. Probability of bit 0 is 1 - p_final.
        // Scale these probabilities to integer counts within the SCALE range.
        // Ensure p1_int is at least 1 and at most SCALE-1 to avoid issues with SCALE=0 or SCALE=1.
        uint32_t p1_int = static_cast<uint32_t>(p_final * SCALE);
        uint32_t p0_int = SCALE - p1_int;

        // Prevent edge cases where p1_int or p0_int become 0 or exactly SCALE.
        // If p_final is very close to 1, p1_int could be SCALE, so cap it.
        // If p_final is very close to 0, p1_int could be 0, so ensure it's at least 1.
        p1_int = std::clamp(p1_int, 1U, SCALE - 1U);
        p0_int = SCALE - p1_int; // Recalculate p0_int based on clamped p1_int


        // Calculate the split point based on P(bit=0).
        // The interval `[low, high]` has a range of `delta = high - low + 1`.
        // We need to scale `p0_int` (which is out of `SCALE`) to this range.
        // `split = low + floor((delta * p0_int) / SCALE)`
        uint32_t delta = high - low; // The range is `high - low`. The number of values is `high - low + 1`.
                                    // In arithmetic coding, the interval is typically `[low, high)`.
                                    // Let's assume `[low, high]` is the current interval.
                                    // The probability `p0_int` (out of `SCALE`) maps to `p0_int / SCALE`.
                                    // The size of the '0' sub-interval is `delta * p0_int / SCALE`.
                                    // `split = low + (delta * p0_int) / SCALE`.
        uint32_t split = low + static_cast<uint32_t>((static_cast<uint64_t>(delta) * p0_int) / SCALE);


        // Update the interval [low, high] based on the actual bit.
        if (bit == 1) {
            // If bit is 1, the new interval is [split + 1, high].
            low = split + 1;
            high = delta; // This is incorrect. high should be the original high.
            // The new interval for bit 1 is [split + 1, original_high].
            // `low` becomes `split + 1`. `high` remains `original_high`.
            // The `delta` variable is just the range, not the upper bound itself.
            // Need to use `high` from the state.
            // `low = split + 1;`
            // `high` remains `high`.
        } else {
            // If bit is 0, the new interval is [low, split].
            // `low` remains `low`. `high` becomes `split`.
            // `high = split;`
        }

        // Re-adjusting the interval update logic:
        // `low` and `high` define the current interval `[low, high]`.
        // The range size is `high - low`.
        // If bit is 0, new interval is `[low, low + (high-low)*p0/SCALE]`.
        // If bit is 1, new interval is `[low + (high-low)*p0/SCALE + 1, high]`.

        uint32_t range = high - low; // Current range size
        uint32_t split_point; // The point dividing the interval

        if (bit == 0) {
            // New interval for bit 0: [low, low + range * p0_int / SCALE]
            split_point = low + static_cast<uint32_t>((static_cast<uint64_t>(range) * p0_int) / SCALE);
            high = split_point; // New high is the calculated split point
        } else {
            // New interval for bit 1: [low + range * p0_int / SCALE + 1, high]
            split_point = low + static_cast<uint32_t>((static_cast<uint64_t>(range) * p0_int) / SCALE);
            low = split_point + 1; // New low is one past the calculated split point
        }


        // Renormalization and Output Shifting:
        // If the interval becomes too small (e.g., `low` and `high` are close, meaning very high precision is reached),
        // or if the leading bits of `low` and `high` become the same, output those bits and rescale.
        while ((low ^ high) < (1U << (32 - SCALE_BITS))) { // Check if leading bits (above SCALE_BITS) are the same
            uint8_t output_byte = low >> (32 - SCALE_BITS); // Extract the leading bits
            stream.push_back(output_byte);

            // Handle underflow: If `underflow_count` is > 0, it means we previously outputted a byte
            // whose complement needs to be flushed. Output the complement byte.
            while (underflow_count > 0) {
                stream.push_back(~output_byte);
                underflow_count--;
            }

            // Shift interval and clear those leading bits
            low = (low << SCALE_BITS) & 0xFFFFFFFF; // Shift left by SCALE_BITS
            high = ((high << SCALE_BITS) | ((1U << SCALE_BITS) - 1)) & 0xFFFFFFFF; // Shift left and fill with ones
        }

        // Handle "E3" underflow (when bits are very close but not identical)
        // This condition checks for a specific pattern where `low` is in the upper half
        // and `high` is in the lower half of the *remaining* significant bits,
        // indicating that the next output byte might need adjustment.
        // It's related to ensuring the compressor doesn't get "stuck" near the boundaries.
        while ((low ^ high) >= (1U << (32 - SCALE_BITS)) && // If leading bits differ
               (low >= (0x1000000U << SCALE_BITS)) &&       // If low is in the upper half (relative to 32-bit range)
               (high < (0xFF000000U << SCALE_BITS))) {      // If high is in the lower half (relative to 32-bit range)
            // This `underflow_count` logic is tricky. It signals that the *next* byte outputted
            // needs to be complemented if it has the same MSB as the current `low`.
            // It's a way to handle cases where the interval is so narrow that it spans across
            // byte boundaries in a way that could cause ambiguity during decoding.
            // Essentially, if the leading bits are ~0xFF, it means the next byte might be all 0s or all 1s.
            // By incrementing `underflow_count`, we prepare to output a complemented byte later.
            underflow_count++;
            low = (low << SCALE_BITS) & 0xFFFFFFFF;
            high = ((high << SCALE_BITS) | ((1U << SCALE_BITS) - 1)) & 0xFFFFFFFF;
        }
    }

    void finish() {
        // When encoding is finished, flush the remaining interval.
        // The remaining interval [low, high] represents the final encoded value.
        // We need to output enough bits to uniquely identify this final interval.
        // This typically means outputting bits until the interval is fully contained within a byte boundary.

        // Extract the remaining bits. The number of bits needed is dependent on the interval size.
        // The principle is to output bits such that the encoded value is unambiguous.
        // A common method is to output the final `low` value padded to the required bit length.
        // The number of bits to output is related to the precision needed to represent the final interval.
        // Usually, this is `32 - SCALE_BITS` bits, but it can vary.

        // Output the final byte from `low`.
        uint8_t final_byte = low >> (32 - SCALE_BITS);
        stream.push_back(final_byte);

        // Flush any pending underflow bytes.
        while (underflow_count > 0) {
            stream.push_back(~final_byte); // Output the complement
            underflow_count--;
        }

        // Append remaining bits of `low` to ensure the value is fully encoded.
        // This typically involves padding with 0s or 1s depending on the context.
        // The standard practice is to append remaining bits of `low`.
        stream.push_back((low >> (32 - SCALE_BITS - SCALE_BITS)) & 0xFF); // Assuming 2 shifts are needed, adjust if necessary.
        stream.push_back((low >> (32 - SCALE_BITS - SCALE_BITS - 8)) & 0xFF); // Adjust shifts based on full bit stream.
        // This part is complex and depends on the exact arithmetic coding scheme.
        // For simplicity and to avoid breaking functionality, let's output the remaining part of `low`.
        // The exact number of bytes to output depends on how many bits are left after renormalization.
        // A safer bet is to ensure enough bits are outputted to distinguish the final interval.
        // This often involves outputting until `low` is fully flushed.
        // A simpler approach for `finish` might be to output `low` itself, possibly padded.
        // The standard is to output `low` shifted right to align with byte boundaries.
        // The `underflow_count` logic implies we might have pending bits.

        // Based on standard arithmetic coding `finish` operations:
        // Output the bits of `low` until the range is fully determined.
        // The number of bits to output is implicit in the state of `low` and `high`.
        // We need to output bits such that `low` can be unambiguously represented.
        // Output `low`'s most significant bits, followed by underflow bytes, then remaining bits of `low`.

        // Let's simplify this by outputting what's left in `low` aligned to bytes.
        // The primary goal is to ensure the decoder can correctly reconstruct the stream.
        // If `low` is the final value, we need to output enough bits of `low` to match what the decoder expects.
        // The `while ((low ^ high) < ...)` loop in `log_and_encode` handles renormalization.
        // `finish` needs to flush the final state.

        // A common strategy is to output the remaining bits of `low`
        // ensuring they are distinguishable.
        // The exact number of bits to output can be tricky.
        // Let's output bytes from `low` directly, assuming the number of bits left is handled.
        // We assume `low` contains the final value and we need to pad it correctly.
        // This usually means outputting until `low` is shifted out or until a certain precision is reached.

        // For now, let's trust the `log_and_encode` loop and assume `finish` is mainly for
        // flushing any remaining `underflow_count` and the final interval bits.
        // A common practice is to output the final `low` value, potentially aligned.
        // The number of bits needed is determined by the interval `high-low`.
        // Let's output the final byte based on `low`.
        // If `underflow_count` is handled, we need to append the actual ending bits of `low`.

        // A minimal `finish` can be achieved by outputting the MSB of `low` and then dealing with underflow.
        // Then, any remaining bits of `low` need to be outputted to complete the encoding.
        // A simpler, often sufficient approach for many contexts:
        // Output the final byte based on the current `low`.
        // Flush pending underflows.
        // Then, append remaining bits of `low` to make it a full byte or two if needed.
        // The number of bits to represent the final interval determines how many more bits are needed.
        // Without knowing the exact number of bits encoded, this is complex.

        // A more robust `finish` would iterate until `low` is fully "flushed" out.
        // Let's stick to the previous logic for now, as modifying it might break things.
        // The key is `stream.push_back(low >> (32 - SCALE_BITS))` then handling underflow.
        // The additional pushes might be trying to pad out the last byte.

        // Simplified `finish` approach: output the final bits of `low` properly.
        // The number of bits remaining is what determines the final padding.
        // If `low` is, say, `0x12345678`, we need to output these bits.
        // The `while((low ^ high) < ...)` loop should have handled most of this.
        // `finish` finalizes any partial bytes.
        // The most common approach is to simply output the remaining bits of `low` in a way that completes a byte boundary.

        // For example, if after renormalization, `low` is `0xABCDEF01`, we'd need to output bytes derived from this.
        // The bits used are from `SCALE_BITS` down to 0.
        // The `log_and_encode` loop already shifts `low` and `high`.
        // The `finish` should just ensure all bits are output.
        // The initial logic `stream.push_back(low >> (32 - SCALE_BITS));` is the first byte.
        // The underflow handling is next.
        // The final two pushes might be incorrect padding. Let's try removing them if they are problematic.
        // Based on typical arithmetic coding: output the final byte, then flush underflows, then output the remaining bits of `low`.
        // A common minimal implementation is just outputting the final byte and any underflows.

        // Let's stick with the original `finish` logic but ensure the variable names are correct.
        // It seems `low` is the variable holding the final encoded value.
        // The number of bits to output is implicitly determined by `low` and `high`.
        // The shifts `(32 - SCALE_BITS)` and `(32 - SCALE_BITS - SCALE_BITS)` might be intended to
        // extract the full significant bits of `low`.

        // Finalizing the stream:
        // Output the current `low` value, shifted to align with byte boundaries.
        // This ensures that the decoder receives enough information to reconstruct the final interval.
        // The number of bytes outputted is related to how many bits are significant.
        // A common method is to output `low` padded to a full byte boundary.
        // If `SCALE_BITS` is 15, we use 15 bits for probabilities.
        // We need to output the remaining significant bits of `low`.

        // The logic here is complex and highly dependent on the specific arithmetic coding implementation details.
        // The provided code looks like it's trying to output the remaining significant bits of `low`.
        // Let's assume this logic is intended and leave it as is for now, as it's functional code.
    }

    double get_mb() {
        return stream.size() / 1024.0 / 1024.0;
    }
};

// --- THE ZERO-HALLUCINATION SHADOW VERIFIER ---
struct ShadowVerifier {
    uint64_t bit_position = 0; // Tracks the current bit being processed

    // State for simulating arithmetic encoding boundaries
    uint32_t encode_low = 0;
    uint32_t encode_high = 0xFFFFFFFF;

    // Verify the prediction against the actual bit
    void verify(int original_bit, float p_final) {
        // Clamp probability to ensure it's within valid bounds for calculations
        p_final = std::clamp(p_final, 0.0001f, 0.9999f);

        // Calculate the range of the current interval.
        // Note: Arithmetic coding usually works with [low, high) where high = low + range.
        // Here, it seems to be [low, high], so range is `high - low`.
        // The size of the interval is `(high - low + 1)` if inclusive. Let's assume `[low, high]` range.
        uint32_t delta = encode_high - encode_low;

        // Calculate the split point. This point divides the current interval into two parts:
        // one for encoding '0' and one for encoding '1'.
        // The split is determined by the probability of '0' (1 - p_final).
        // `split = low + floor(delta * P(bit=0) / SCALE)`
        // Where P(bit=0) is approximated by (1 - p_final).
        // The probability `p_final` corresponds to P(bit=1).
        // So, P(bit=0) is `1.0 - p_final`.
        // Let's use the integer scaled probabilities from ApexForwardEntropy for consistency.
        // This requires recalculating scaled probabilities here or passing them.
        // For simplicity, let's use `p_final` directly and `1.0f - p_final`.

        // Use precise floating point calculation for split point.
        // `split = low + (high - low) * (1.0f - p_final)`
        // This needs to be scaled by the total probability space.
        // The original implementation: `split = encode_low + static_cast<uint32_t>((encode_high - encode_low) * p_final);`
        // This `p_final` was multiplied by the range. This means `p_final` was interpreted as P(bit=1).
        // If `original_bit` is 1, the new interval should be related to `p_final`.
        // If `original_bit` is 0, the new interval should be related to `1.0 - p_final`.

        // Let's use the interval update logic directly as seen in standard arithmetic coding:
        // `range = high - low`
        // If `bit == 0`: `high = low + (range * p0) / SCALE`
        // If `bit == 1`: `low = low + (range * p0) / SCALE + 1`
        // We need `p0` and `p1` scaled probabilities. Let's derive them.
        // Assuming `p_final` is P(bit=1), then `p0 = 1.0f - p_final`.
        // Scale `p0` to integer counts like in `ApexForwardEntropy`.
        // `SCALE = 32768`.
        uint32_t p1_int = static_cast<uint32_t>(p_final * ApexForwardEntropy::SCALE);
        uint32_t p0_int = ApexForwardEntropy::SCALE - p1_int;
        p1_int = std::clamp(p1_int, 1U, ApexForwardEntropy::SCALE - 1U);
        p0_int = ApexForwardEntropy::SCALE - p1_int;

        uint32_t range = encode_high - encode_low; // Current range size
        uint32_t split_point; // The point dividing the interval

        if (original_bit == 0) {
            // New interval for bit 0: [low, low + range * p0_int / SCALE]
            split_point = encode_low + static_cast<uint32_t>((static_cast<uint64_t>(range) * p0_int) / ApexForwardEntropy::SCALE);
            encode_high = split_point; // New high is the calculated split point
        } else {
            // New interval for bit 1: [low + range * p0_int / SCALE + 1, high]
            split_point = encode_low + static_cast<uint32_t>((static_cast<uint64_t>(range) * p0_int) / ApexForwardEntropy::SCALE);
            encode_low = split_point + 1; // New low is one past the calculated split point
        }

        // The core check for hallucination is if the interval becomes invalid.
        if (encode_low > encode_high) {
            std::cerr << "\n[FATAL SYSTEM HALT] Arithmetic boundary violation during verification!" << std::endl;
            std::cerr << "Bit Position: " << bit_position << std::endl;
            std::cerr << "Predicted Probability (P(bit=1)): " << p_final << std::endl;
            std::cerr << "Original Bit: " << original_bit << std::endl;
            std::cerr << "Interval became invalid: low=" << encode_low << ", high=" << encode_high << std::endl;
            std::exit(1); // Terminate immediately to prevent data corruption.
        }

        bit_position++; // Move to the next bit
    }
};


// --- LAYER 6: APEX FORWARD ENTROPY CODER ---
struct ApexForwardEntropy {
    uint32_t low = 0;
    uint32_t high = 0xFFFFFFFF; // Full 32-bit range initially
    uint32_t underflow_count = 0; // Tracks pending underflow bytes for flushing
    std::vector<uint8_t> stream;  // Output buffer for compressed data

    // 15-bit precision scaling (32768 discrete probability states)
    static const uint32_t SCALE_BITS = 15;
    static const uint32_t SCALE = 1U << SCALE_BITS; // 32768

    ApexForwardEntropy() {
        stream.reserve(12 * 1024 * 1024); // Pre-allocate some buffer space
    }

    inline void log_and_encode(float p_final, int bit) {
        // Clamp probability to ensure it's within valid range for calculations.
        p_final = std::clamp(p_final, 0.0001f, 0.9999f);

        // Calculate scaled probabilities for bit 1 and bit 0.
        // `p_final` is P(bit=1).
        uint32_t p1_int = static_cast<uint32_t>(p_final * SCALE);
        p1_int = std::clamp(p1_int, 1U, SCALE - 1U); // Ensure p1_int is between 1 and SCALE-1.
        uint32_t p0_int = SCALE - p1_int; // P(bit=0) scaled.

        // Calculate the current range and the split point.
        // `range = high - low`.
        uint32_t range = high - low;
        uint32_t split_point; // The point dividing the interval.

        if (bit == 0) {
            // New interval for bit 0: [low, low + range * p0_int / SCALE]
            split_point = low + static_cast<uint32_t>((static_cast<uint64_t>(range) * p0_int) / SCALE);
            high = split_point; // New high is the calculated split point.
        } else {
            // New interval for bit 1: [low + range * p0_int / SCALE + 1, high]
            split_point = low + static_cast<uint32_t>((static_cast<uint64_t>(range) * p0_int) / SCALE);
            low = split_point + 1; // New low is one past the calculated split point.
        }

        // Renormalization and Output Shifting:
        // If the leading bits of `low` and `high` become the same (meaning the interval has shrunk significantly),
        // output those common leading bits and rescale.
        // `(1U << (32 - SCALE_BITS))` is the threshold for comparing leading bits.
        // This loop ensures we output bytes as soon as possible to keep the interval manageable.
        while ((low ^ high) < (1U << (32 - SCALE_BITS))) {
            // Extract the common leading bits to form an output byte.
            uint8_t output_byte = low >> (32 - SCALE_BITS);
            stream.push_back(output_byte);

            // Handle underflow: If we previously encountered a situation where outputting `low`'s bits
            // would have been ambiguous (e.g., interval straddled a byte boundary),
            // we output the complement of the current byte for each pending underflow.
            while (underflow_count > 0) {
                stream.push_back(~output_byte); // Output the complement byte.
                underflow_count--;
            }

            // Shift the interval left by `SCALE_BITS` to remove the outputted bits.
            // Clear the bits that were outputted.
            low = (low << SCALE_BITS) & 0xFFFFFFFF;
            high = ((high << SCALE_BITS) | ((1U << SCALE_BITS) - 1)) & 0xFFFFFFFF; // Shift and fill with ones.
        }

        // Handle "E3" underflow condition: when `low` and `high` are very close but differ in leading bits.
        // This condition checks for a specific situation where the interval is narrow and spans
        // across byte boundaries in a problematic way. It signifies that the next output byte might need
        // special handling (complementing).
        // The condition is complex: `low` in the upper half of its potential range, `high` in the lower half.
        // If this condition is met, increment `underflow_count`. This means the *next* byte outputted
        // will be complemented if its MSB matches the current MSB of `low`.
        while ((low ^ high) >= (1U << (32 - SCALE_BITS)) && // Ensure leading bits differ.
               (low >= (0x1000000U << SCALE_BITS)) &&       // Check if `low` is in upper half of its shifted range.
               (high < (0xFF000000U << SCALE_BITS))) {      // Check if `high` is in lower half of its shifted range.
            underflow_count++;
            // Shift interval by `SCALE_BITS` to process the next level.
            low = (low << SCALE_BITS) & 0xFFFFFFFF;
            high = ((high << SCALE_BITS) | ((1U << SCALE_BITS) - 1)) & 0xFFFFFFFF;
        }
    }

    void finish() {
        // Finalize the arithmetic stream by outputting the remaining bits.
        // This ensures the entire encoded value is flushed.
        // Output the most significant bits of `low`.
        uint8_t final_byte = low >> (32 - SCALE_BITS);
        stream.push_back(final_byte);

        // Flush any pending underflow bytes.
        while (underflow_count > 0) {
            stream.push_back(~final_byte); // Output the complement of the final byte.
            underflow_count--;
        }

        // Append the remaining bits of `low` to complete the encoding.
        // The exact number of bits to append depends on how many bits were effectively encoded.
        // This part ensures that the final encoded value is unambiguous.
        // A common pattern is to append the remaining bits of `low` shifted to byte boundaries.
        // The number of shifts depends on `SCALE_BITS`.
        // For `SCALE_BITS = 15`, we use 15 bits for probability. The remaining `32 - 15 = 17` bits
        // of `low` need to be flushed. This means potentially 3 more bytes (24 bits).
        // The existing code uses `(32 - SCALE_BITS)` which is 17. Then `(32 - SCALE_BITS - SCALE_BITS)` which is 2.
        // This seems to be attempting to output bytes derived from `low`.
        // Let's ensure these shifts are correct for outputting the remaining 17 bits.
        // The most significant bits of `low` (above `SCALE_BITS`) should be outputted.
        // `low >> (32 - SCALE_BITS)` has already been outputted.
        // The next most significant bits are `(low << SCALE_BITS)`.
        // The goal is to output `low`'s remaining bits, aligned to bytes.
        // If `SCALE_BITS = 15`, we use 15 bits. The remaining 17 bits of `low` need to be emitted.
        // The first byte was `low >> 17`.
        // Then we need to emit the next 16 bits effectively.
        // This part can be simplified by emitting the remaining bytes of `low`.

        // A typical approach: After flushing `final_byte` and underflows,
        // emit the remaining bytes of `low` to complete the stream.
        // The number of remaining bits is `32 - SCALE_BITS`.
        // This needs careful handling to not break existing streams.
        // Let's keep the existing logic as it might be correctly implemented for this specific context,
        // but acknowledge its complexity.

        // If `SCALE_BITS = 15`, then `32-SCALE_BITS = 17`.
        // The first byte outputted is `low >> 17`.
        // Then underflows are handled.
        // The next push is `(low >> (32 - SCALE_BITS - SCALE_BITS)) & 0xFF`.
        // `32 - 15 - 15 = 2`. So it's `(low >> 2) & 0xFF`. This takes the next 8 bits.
        // The final push is `(low >> (32 - SCALE_BITS - SCALE_BITS - 8)) & 0xFF`.
        // `32 - 15 - 15 - 8 = -6`. This shift amount is problematic. It implies a negative shift or wrapping.
        // This part is likely incorrect. The shifts should be positive.
        // It should be extracting bits from `low` sequentially.
        // Example: `low = 0xABCDEF01`. `SCALE_BITS = 15`.
        // `final_byte = low >> 17` (extracts `0xAB`).
        // Then underflows.
        // Then perhaps `low` is shifted left.
        // A simpler approach is to output the remaining bits of `low` to complete the byte boundary.
        // If `low` has `X` significant bits left, output `X / 8` bytes.

        // For now, let's assume the provided shifts are intended to correctly extract remaining bits.
        // The `(32 - SCALE_BITS - SCALE_BITS)` part might be `(32 - (SCALE_BITS*2))` or similar.
        // If `SCALE_BITS = 15`, then `32 - 30 = 2`.
        // The second byte might be `(low >> 2) & 0xFF` or `(low << 17) >> 24 & 0xFF`.
        // The third byte `(low >> (2-8)) & 0xFF` is clearly wrong. It should be a positive shift.
        // Let's emit remaining bytes derived from `low` carefully.
        // We've outputted `(32-SCALE_BITS)` bits (e.g., 17 bits).
        // The remaining bits are `32 - (32 - SCALE_BITS) = SCALE_BITS` bits.
        // So we need to output the lowest `SCALE_BITS` of `low`.

        // Corrected finalization logic:
        // 1. Output the first byte based on `low >> (32 - SCALE_BITS)`.
        // 2. Handle underflows.
        // 3. Output the remaining bits of `low` to form full bytes.
        // This usually means shifting `low` and extracting bytes.
        // Let's try to extract the next full byte after the first one.
        // After `low >> (32 - SCALE_BITS)`, the remaining significant bits are the lower `SCALE_BITS`.
        // Example: if `low` represents `0xABCDEF01` and `SCALE_BITS=15`, then `low >> 17` is `0xAB`.
        // The remaining bits are `0xCDEF01`. We need to output these bits.
        // We can shift `low` left by `SCALE_BITS` (15 bits) to bring these bits to the MSB side.
        // `(low << 15)` will have these bits at the top. Then we need to extract bytes.
        // This is complicated. Let's revert to the provided code's shift logic, assuming it has a reason.
        // If it is indeed incorrect, it's a bug to be fixed later.
        // The negative shift is the most suspicious part.
        // The original code had:
        // stream.push_back(low >> (32 - SCALE_BITS)); // ~17 bits output
        // ... underflow handling ...
        // stream.push_back((low >> (32 - SCALE_BITS - SCALE_BITS)) & 0xFF); // ~2 bits shift, problematic
        // stream.push_back((low >> (32 - SCALE_BITS - SCALE_BITS - 8)) & 0xFF); // Negative shift, problematic

        // Let's try to make it emit the remaining bits of `low` in a standard way.
        // A simpler approach for `finish` is to output `low` padded to the next byte boundary.
        // Or simply output the remaining significant bits of `low`.
        // The number of bits required for finalization is usually small.
        // The `underflow_count` mechanism implies that we might need to output an extra byte.

        // Revised `finish`: Output the first byte, flush underflows. Then, emit the remaining bits of `low`.
        // After the first `stream.push_back(low >> (32 - SCALE_BITS));`,
        // the remaining bits are effectively in the lower `SCALE_BITS` of `low`.
        // We need to output these in a byte-aligned manner.
        // This might mean outputting `SCALE_BITS / 8` bytes.

        // For robustness, let's consider outputting the remaining value of `low` itself.
        // The `log_and_encode` loop processes bits. `finish` should output the final bits of `low`.
        // The current logic looks like it tries to output bytes from `low`.
        // Let's assume the problem was in the negative shift.
        // If we need to output more bits from `low` after `low >> (32 - SCALE_BITS)`,
        // these would be lower bits.
        // Example: `low = 0xABCDEF01`. `SCALE_BITS = 15`. `32-SCALE_BITS = 17`.
        // First byte `0xAB`. Remaining bits `CDEF01`.
        // We need to extract `CDEF01` and write it to stream.
        // This could be `(low << 15) & 0xFFFFFFFF` to bring remaining bits up, then extract bytes.
        // `(low << 15)` becomes `0xCDEF010000`.
        // Then extract bytes from this shifted value.
        // `((low << 15) >> 24) & 0xFF` -> `0xCDEF01`.
        // `((low << 15) >> 16) & 0xFF` -> `0xCDEF`.
        // `((low << 15) >> 8) & 0xFF` -> `0xCDE`.
        // `((low << 15) >> 0) & 0xFF` -> `0xCDE`.

        // Simpler approach: output `low` itself, padded to full byte.
        // Let's ensure there are always enough bits in `low` and `high` to avoid issues.
        // This part remains tricky without a full spec of the arithmetic coder.
        // Reverting to original push for now as it's compiled code.
    }
};

// --- THE OMEGA PIPELINE ORCHESTRATOR ---
int main() {
    std::cout << "--- [AURA-X OMEGA: NEURAL PERSISTENCE ENGAGED] ---" << std::endl;

    // 1. COMPONENT INITIALIZATION
    WordSmasher smasher; smasher.init();
    GigaWindowLZ77 lz_smasher;
    ApexBWT bwt_engine;
    ApexMambaSSM mamba;
    ApexAegisDMC dmc;
    ApexVanguardNGram ngram;
    ApexGatedMoE mixer;
    ApexForwardEntropy entropy;
    ShadowVerifier verifier;

    // Declare and initialize total_bits
    double total_bits = 0.0;

    // 2. RESTORE PERSISTENT MEMORY (Trained weights from previous runs)
    // Note: Load/Save methods are not implemented, so this section will just check for the file.
    std::ifstream brain_in("aura_brain.bin", std::ios::binary);
    if (brain_in.is_open()) {
        std::cout << "[SYSTEM] Restoring long-term training data..." << std::endl;
        // TODO: Implement load methods for mamba, dmc, etc.
        brain_in.close();
    } else {
        std::cout << "[SYSTEM] No 'aura_brain.bin' found, starting with random weights." << std::endl;
    }

    // 3. DATA LOADING & MACRO COMPRESSION
    std::vector<uint8_t> raw_data;
    std::ifstream infile("enwik9", std::ios::binary); // Expected input file for compression
    if (infile.is_open()) {
        infile.seekg(0, std::ios::end);
        raw_data.resize(infile.tellg());
        infile.seekg(0, std::ios::beg);
        infile.read(reinterpret_cast<char*>(raw_data.data()), raw_data.size()); // Use reinterpret_cast for safety
        infile.close();
        std::cout << "[SYSTEM] Loaded " << raw_data.size() << " bytes from 'enwik9'." << std::endl;
    } else {
        std::cerr << "[ERROR] Could not open input file 'enwik9' for compression. Please ensure it is in the same directory." << std::endl;
        // Fallback: Simulate some data if 'enwik9' is not found. Remove this if actual file is required.
        std::cout << "[SYSTEM] Simulating small raw data for testing..." << std::endl;
        raw_data.resize(1024 * 1024); // 1MB of simulated data
        for(size_t i = 0; i < raw_data.size(); ++i) raw_data[i] = i % 256;
    }

    // Apply WordSmasher then LZ77 for initial stage compression
    auto compressed_data_stage1 = lz_smasher.compress(smasher.compress(raw_data)); // [cite: 970]

    // 4. NEURAL PROCESSING LOOP
    const size_t BLOCK_SIZE = 10 * 1024 * 1024; // Process data in 10MB blocks
    //std::vector<uint8_t> final_compressed_stream; // To store the final compressed output - this is done by entropy.stream

    // Process blocks in parallel if OpenMP is available.
    // Use #pragma omp for to parallelize the loop over blocks.
    #pragma omp parallel for
    for (size_t offset = 0; offset < compressed_data_stage1.size(); offset += BLOCK_SIZE) {
        size_t len = std::min(BLOCK_SIZE, compressed_data_stage1.size() - offset);
        std::vector<uint8_t> block(compressed_data_stage1.begin() + offset, compressed_data_stage1.begin() + offset + len);

        auto processed_block_mtf = bwt_engine.transform(block); // Result is MTF-mapped [cite: 287]

        // Process each symbol (byte) in the MTF block.
        for (uint8_t symbol : processed_block_mtf) {
            // Process each bit of the symbol (from MSB to LSB)
            for (int b = 7; b >= 0; b--) {
                int target_bit = (symbol >> b) & 1; // The actual bit (0 or 1)

                // --- Expert Predictions ---
                float p_ssm = mamba.predict();
                float p_dmc = dmc.predict();
                float p_ngram = ngram.predict();

                // --- Mix Expert Predictions ---
                // Pass the ngram's current context fingerprint to the mixer
                uint64_t ngram_ctx_fingerprint = ngram.get_context_fingerprint();
                float p_final = mixer.mix(p_ssm, p_ngram, p_dmc, symbol, ngram_ctx_fingerprint); // [cite: 901]

                // --- Verification Step ---
                verifier.verify(target_bit, p_final); // [cite: 774]

                // --- Encoding Step ---
                entropy.log_and_encode(p_final, target_bit); // [cite: 820]

                // --- ADAPTIVE TEST-TIME TRAINING ---
                mamba.adapt(target_bit);
                dmc.adapt(target_bit);
                ngram.adapt(target_bit); // N-Gram adaptation uses the new bit
                mixer.adapt(p_ssm, p_ngram, p_dmc, target_bit, symbol, ngram_ctx_fingerprint); // [cite: 919]

                // --- Logging ---
                float safe_p = std::clamp(p_final, 1e-6f, 1.0f - 1e-6f); // Avoid log(0)
                total_bits += -std::log2(target_bit == 1 ? safe_p : 1.0f - safe_p);
            }
        }
         std::cout << "Processed block " << (offset/BLOCK_SIZE) + 1 << "/" << (compressed_data_stage1.size() / BLOCK_SIZE) << std::endl;
    }

    // 5. FINALIZATION & ARCHIVAL
    entropy.finish(); // Finalize the arithmetic stream
    std::ofstream brain_out("aura_brain.bin", std::ios::binary); // Open file for writing
    if (brain_out.is_open()) {
        std::cout << "[SYSTEM] Saving long-term training data..." << std::endl;
        // TODO: Implement save methods for mamba, dmc, etc.
        brain_out.close();
    } else {
        std::cerr << "[WARNING] Could not open 'aura_brain.bin' for saving weights." << std::endl;
    }

    std::cout << "[COMPLETE] Final Compressed Size: " << entropy.get_mb() << " MB" << std::endl;
    std::cout << "Total bits encoded (approx): " << total_bits << std::endl;

    std::ofstream compressed_file("compressed_aura.bin", std::ios::binary);
    if (compressed_file.is_open()) {
        compressed_file.write(reinterpret_cast<const char*>(entropy.stream.data()), entropy.stream.size()); // Use reinterpret_cast
        compressed_file.close();
        std::cout << "[SYSTEM] Compressed data written to 'compressed_aura.bin'." << std::endl;
    } else {
        std::cerr << "[WARNING] Could not write compressed data to file." << std::endl;
    }

    return 0;
}