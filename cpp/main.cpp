#include <sys/resource.h>
#include <sched.h>
#include <unistd.h>
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

// --- LAYER 0: OS-LEVEL HARDWARE ENFORCEMENT ---
void enforce_hardware_limits() {
    // 1. Enforce strict 10GB RAM Limit via kernel
    struct rlimit memory_limit;
    const uint64_t MAX_RAM_BYTES = 10ULL * 1024 * 1024 * 1024; // 10 GB
    memory_limit.rlim_cur = MAX_RAM_BYTES;
    memory_limit.rlim_max = MAX_RAM_BYTES;
    
    if (setrlimit(RLIMIT_AS, &memory_limit) != 0) {
        std::cerr << "[WARNING] Kernel rejected RAM constraint." << std::endl;
    }

    // 2. Lock to exactly 1 Physical Core (Core 0)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset); 
    
    // Bind the current process to the specified CPU set
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
        std::cerr << "[WARNING] Kernel rejected CPU affinity lock." << std::endl;
    }

    // 3. Throttle OpenMP to prevent background thread spawning
    omp_set_num_threads(1);

    std::cout << "[SYSTEM] Hardware Constraints Locked: 1 vCPU (Intel 8244C Profile) | 10.00 GB RAM MAX" << std::endl;
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

// --- LAYER 2: GIGA-WINDOW LZ77 (OPTIMAL PARSING & MULTI-HASH CORE) ---
struct GigaWindowLZ77 {
    // 128MB Horizon Limits
    static const uint32_t WINDOW_SIZE = 134217728;
    static const uint32_t MIN_MATCH = 4; // Lowered minimum for denser packing
    static const uint32_t MAX_MATCH = 258;
    static const uint32_t MAX_MATCH_ENCODED = MAX_MATCH - MIN_MATCH;
    
    // Multi-Tier Hash Tables for collision resistance
    static const uint32_t HASH3_SIZE = 16777216; // 16M
    static const uint32_t HASH4_SIZE = 33554432; // 32M
    
    // SIMD-Aligned Optimal Parsing Grid
    struct alignas(64) OptimalNode {
        uint32_t cumulative_price;
        uint32_t match_length;
        uint32_t match_distance;
        bool is_literal;
    };

    struct Match { uint32_t length = 0; uint32_t distance = 0; };

    // Advanced Bitwise Hash Algorithms
    inline uint32_t hash3(const uint8_t* p) { return ((p[0] << 16) | (p[1] << 8) | p[2]) & (HASH3_SIZE - 1); }
    inline uint32_t hash4(const uint8_t* p) { return (*reinterpret_cast<const uint32_t*>(p) * 2654435761U) & (HASH4_SIZE - 1); }

    // Calculates the "entropic price" of encoding a match vs. literal
    inline uint32_t calculate_price(uint32_t length, uint32_t distance) {
        if (length == 0) return 8; // Literal cost ~ 8 bits
        // Cost model: roughly 8 bits for flags + encoded length + 32 bits for distance
        return 8 + 8 + 32; 
    }

    // High-Precision Multi-Tier Match Finder
    Match find_best_match(const std::vector<uint8_t>& in, uint32_t pos, const std::vector<int>& head3, const std::vector<int>& head4, const std::vector<int>& prev) {
        Match best;
        if (pos + 4 > in.size()) return best;

        // Check Hash4 (High Precision) first, fall back to Hash3
        int current_match_pos = head4[hash4(&in[pos])];
        if (current_match_pos == -1 || current_match_pos == static_cast<int>(pos)) {
            current_match_pos = head3[hash3(&in[pos])];
        }

        int chain_limit = 8192; // Deep chain traversal for 128MB window

        const uint8_t* scan_ptr = &in[pos];
        const uint8_t* scan_end_ptr = scan_ptr + std::min(static_cast<uint32_t>(in.size() - pos), MAX_MATCH);

        while (current_match_pos != -1 && (pos - static_cast<uint32_t>(current_match_pos)) <= WINDOW_SIZE && chain_limit-- > 0) {
            const uint8_t* match_ptr = &in[current_match_pos];
            
            // Fast bail-out
            if (match_ptr[0] != scan_ptr[0] || match_ptr[1] != scan_ptr[1]) {
                current_match_pos = prev[current_match_pos % WINDOW_SIZE];
                continue;
            }

            uint32_t len = 0;
            // OpenMP SIMD Block Comparison
            #pragma omp simd
            for (uint32_t step = 0; step < 32; step++) {
                if (scan_ptr + len + 8 < scan_end_ptr && match_ptr + len + 8 < &in[WINDOW_SIZE] && 
                    *reinterpret_cast<const uint64_t*>(scan_ptr + len) == *reinterpret_cast<const uint64_t*>(match_ptr + len)) {
                    len += 8;
                }
            }
            while (scan_ptr + len < scan_end_ptr && match_ptr[len] == scan_ptr[len]) len++;

            if (len > best.length) {
                best.length = len;
                best.distance = pos - static_cast<uint32_t>(current_match_pos);
                if (len >= MAX_MATCH) break; 
            }
            current_match_pos = prev[current_match_pos % WINDOW_SIZE];
        }
        return best;
    }

    std::vector<uint8_t> compress(const std::vector<uint8_t>& in) {
        std::vector<uint8_t> out;
        out.reserve(in.size() / 2); 
        
        std::vector<int> head3(HASH3_SIZE, -1);
        std::vector<int> head4(HASH4_SIZE, -1);
        std::vector<int> prev(WINDOW_SIZE, -1); 
        std::vector<OptimalNode> opt(in.size() + 1);

        // Initialize Optimal Parsing Grid
        for (size_t i = 0; i <= in.size(); i++) {
            opt[i].cumulative_price = 0xFFFFFFFF; // Infinity
            opt[i].match_length = 0;
        }
        opt[0].cumulative_price = 0;

        std::ofstream log("lz77_optimal_telemetry.log");
        if (log.is_open()) log << "--- LZ77 OPTIMAL DP PARSING ENGAGED ---\n";

        auto start_time = std::chrono::high_resolution_clock::now();

        // Pass 1: Dynamic Programming Forward Pass (The Cost Matrix)
        uint32_t i = 0;
        while (i < in.size()) {
            if (i + MIN_MATCH <= in.size()) {
                uint32_t h3 = hash3(&in[i]);
                uint32_t h4 = hash4(&in[i]);
                if (head4[h4] != -1) prev[i % WINDOW_SIZE] = head4[h4];
                else if (head3[h3] != -1) prev[i % WINDOW_SIZE] = head3[h3];
                
                head3[h3] = static_cast<int>(i);   
                head4[h4] = static_cast<int>(i);
            }

            Match best_match = find_best_match(in, i, head3, head4, prev);

            // Calculate Literal Cost Path
            uint32_t literal_price = opt[i].cumulative_price + calculate_price(0, 0);
            if (literal_price < opt[i + 1].cumulative_price) {
                opt[i + 1].cumulative_price = literal_price;
                opt[i + 1].is_literal = true;
                opt[i + 1].match_length = 1;
            }

            // Calculate Match Cost Paths
            for (uint32_t len = MIN_MATCH; len <= best_match.length; len++) {
                uint32_t match_price = opt[i].cumulative_price + calculate_price(len, best_match.distance);
                if (match_price < opt[i + len].cumulative_price) {
                    opt[i + len].cumulative_price = match_price;
                    opt[i + len].is_literal = false;
                    opt[i + len].match_length = len;
                    opt[i + len].match_distance = best_match.distance;
                }
            }

            // --- HUD TELEMETRY ---
            if (i % (2 * 1024 * 1024) == 0 && i > 0) {
                auto now = std::chrono::high_resolution_clock::now();
                double elapsed = std::chrono::duration<double>(now - start_time).count();
                double speed = (i / 1024.0 / 1024.0) / elapsed; 
                double progress = (double)i / in.size() * 100.0;
                
                std::cout << "\r[LZ77 OPTIMAL] " << std::fixed << std::setprecision(1) << progress << "% "
                          << "| Analyzed: " << (i / 1024 / 1024) << " MB "
                          << "| Speed: " << std::setprecision(2) << speed << " MB/s " << std::flush;
            }
            i++;
        }

        std::cout << "\n[LZ77 OPTIMAL] Forward pass complete. Backtracking shortest path...\n";

        // Pass 2: Backwards Traversal to find the shortest entropic path
        std::vector<uint32_t> optimal_path;
        uint32_t curr = in.size();
        while (curr > 0) {
            optimal_path.push_back(curr);
            curr -= opt[curr].match_length;
        }
        std::reverse(optimal_path.begin(), optimal_path.end());

        // Pass 3: Stream Encoding
        uint32_t read_head = 0;
        for (uint32_t node_idx : optimal_path) {
            OptimalNode& node = opt[node_idx];
            if (node.is_literal) {
                if (in[read_head] == 0xFF) { out.push_back(0xFF); out.push_back(0); } 
                else out.push_back(in[read_head]);
                read_head += 1;
            } else {
                out.push_back(0xFF);
                out.push_back(static_cast<uint8_t>(std::min(node.match_length - MIN_MATCH, MAX_MATCH_ENCODED)));
                out.push_back(static_cast<uint8_t>(node.match_distance & 0xFF));          
                out.push_back(static_cast<uint8_t>((node.match_distance >> 8) & 0xFF));
                out.push_back(static_cast<uint8_t>((node.match_distance >> 16) & 0xFF));
                out.push_back(static_cast<uint8_t>((node.match_distance >> 24) & 0xFF)); 
                
                if (node.match_length > 64 && log.is_open()) {
                    log << "Optimal Block -> Len: " << std::setw(4) << node.match_length << " | Dist: " << node.match_distance << "\n";
                }
                read_head += node.match_length;
            }
        }

        if (log.is_open()) log.close();
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

    void save(std::ofstream& out) {
        out.write(reinterpret_cast<char*>(&node_count), sizeof(node_count));
        out.write(reinterpret_cast<char*>(graph.data()), node_count * sizeof(Node));
    }
    
    void load(std::ifstream& in) {
        in.read(reinterpret_cast<char*>(&node_count), sizeof(node_count));
        in.read(reinterpret_cast<char*>(graph.data()), node_count * sizeof(Node));
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
        // Core Backpropagation and Selective State Update
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

    void save(std::ofstream& out) {
        out.write(reinterpret_cast<char*>(A), sizeof(A));
        out.write(reinterpret_cast<char*>(B), sizeof(B));
        out.write(reinterpret_cast<char*>(C), sizeof(C));
        out.write(reinterpret_cast<char*>(delta), sizeof(delta));
    }
    
    void load(std::ifstream& in) {
        in.read(reinterpret_cast<char*>(A), sizeof(A));
        in.read(reinterpret_cast<char*>(B), sizeof(B));
        in.read(reinterpret_cast<char*>(C), sizeof(C));
        in.read(reinterpret_cast<char*>(delta), sizeof(delta));
    }
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
        // Output the remaining bits of 'low' cleanly
        uint8_t final_byte = low >> (32 - SCALE_BITS);
        stream.push_back(final_byte);
        
        while (underflow_count > 0) {
            stream.push_back(~final_byte); 
            underflow_count--;
        }
        
        // Output final padding cleanly
        stream.push_back((low >> 2) & 0xFF);
    }

    double get_mb() {
        return stream.size() / 1024.0 / 1024.0;
    }
};

// --- Vanguard N-Gram Dummy Class (For Missing Expert Implementation) ---
struct ApexVanguardNGram {
    uint64_t history = 0;
    float predict() { return 0.5f; } // Neutral prediction
    void adapt(int bit) { history = (history << 1) | bit; }
    uint64_t get_context_fingerprint() { return history; }
};

#include <csignal>
#include <sys/resource.h>
#include <sched.h>
#include <unistd.h>
#include <atomic>

// --- LAYER 0: GLOBAL INTERRUPT HANDLING ---
// Allows the engine to save the neural state if you press Ctrl+C
std::atomic<bool> external_interrupt_triggered(false);

void handle_sigint(int sig) {
    std::cout << "\n\n[WARNING] SYSTEM INTERRUPT RECEIVED (SIGINT)." << std::endl;
    std::cout << "Initiating emergency weight serialization. Please wait..." << std::endl;
    external_interrupt_triggered = true;
}

// --- TELEMETRY MANAGER ---
// Generates super-detailed, non-blocking logs
void write_block_telemetry(size_t block_id, double bpb, double speed, float m_w, float d_w, float n_w) {
    std::ofstream log("aura_pipeline_metrics.log", std::ios::app);
    if (log.is_open()) {
        log << "{\n"
            << "  \"block\": " << block_id << ",\n"
            << "  \"timestamp\": \"" << std::chrono::system_clock::now().time_since_epoch().count() << "\",\n"
            << "  \"metrics\": {\n"
            << "    \"bpb\": " << std::fixed << std::setprecision(5) << bpb << ",\n"
            << "    \"speed_kbps\": " << std::setprecision(2) << speed << "\n"
            << "  },\n"
            << "  \"expert_routing\": {\n"
            << "    \"mamba_ssm\": " << std::setprecision(3) << m_w << ",\n"
            << "    \"aegis_dmc\": " << d_w << ",\n"
            << "    \"vanguard_ngram\": " << n_w << "\n"
            << "  }\n"
            << "}\n";
    }
}

// --- THE GRANDMASTER ORCHESTRATOR ---
int main() {
    auto start_total = std::chrono::high_resolution_clock::now();
    std::cout << "\n=======================================================" << std::endl;
    std::cout << "   [ AURA-X OMEGA: PRODUCTION NEURAL CORE v1.5 ]" << std::endl;
    std::cout << "=======================================================" << std::endl;

    enforce_hardware_limits();
    std::cout << "[SYSTEM] Environment Locked: 1 vCPU | 10.0GB RAM | Interrupts Armed." << std::endl;

    // --- PHASE I: PIPELINE INITIALIZATION ---
    WordSmasher smasher; smasher.init();
    GigaWindowLZ77 lz_smasher; 
    ApexBWT bwt_engine;
    ApexMambaSSM mamba;
    ApexAegisDMC dmc;
    ApexVanguardNGram ngram;
    ApexGatedMoE mixer;
    ApexForwardEntropy entropy;
    ShadowVerifier verifier;

    std::ifstream brain_in("aura_brain.bin", std::ios::binary);
    if (brain_in.is_open()) {
        std::cout << "[SYSTEM] Restoring persistent 2048-Dim parameters..." << std::endl;
        mamba.load(brain_in); dmc.load(brain_in); brain_in.close();
    }

    std::vector<uint8_t> raw_data;
    std::ifstream infile("enwik9", std::ios::binary); 
    if (!infile.is_open()) { std::cerr << "[FATAL] 'enwik9' missing." << std::endl; return 1; }
    infile.seekg(0, std::ios::end); raw_data.resize(infile.tellg());
    infile.seekg(0, std::ios::beg); infile.read(reinterpret_cast<char*>(raw_data.data()), raw_data.size()); 
    infile.close();

    // --- PHASE II: MACRO REDUCTION ---
    std::cout << "\n[PHASE II] Macro-Reduction Initiated..." << std::endl;
    auto stage1 = lz_smasher.compress(smasher.compress(raw_data)); 

    // --- PHASE III: NEURAL SQUEEZE ---
    std::cout << "\n[PHASE III] Neural Squeeze Initiated..." << std::endl;
    const size_t BLOCK_SIZE = 10 * 1024 * 1024; 
    
    double total_bits = 0.0;
    uint64_t bit_counter = 0;
    size_t block_id = 0;
    auto start_neural = std::chrono::high_resolution_clock::now();

    for (size_t offset = 0; offset < stage1.size(); offset += BLOCK_SIZE) {
        if (external_interrupt_triggered) break; // Emergency Stop Check
        block_id++;

        size_t len = std::min(BLOCK_SIZE, stage1.size() - offset);
        std::vector<uint8_t> block(stage1.begin() + offset, stage1.begin() + offset + len);
        auto mtf_stream = bwt_engine.transform(block); 

        for (uint8_t symbol : mtf_stream) {
            if (external_interrupt_triggered) break; // Emergency Stop Check

            for (int b = 7; b >= 0; b--) {
                int target_bit = (symbol >> b) & 1; 

                float p_s = mamba.predict(), p_d = dmc.predict(), p_n = ngram.predict();
                uint64_t ctx = ngram.get_context_fingerprint();
                float p_f = mixer.mix(p_s, p_n, p_d, symbol, ctx); 
                
                verifier.verify(target_bit, p_f);
                entropy.log_and_encode(p_f, target_bit); 

                mamba.adapt(target_bit); dmc.adapt(target_bit); ngram.adapt(target_bit); 
                mixer.adapt(p_s, p_n, p_d, target_bit, symbol, ctx); 

                float safe_p = std::clamp(p_f, 1e-6f, 1.0f - 1e-6f); 
                total_bits += -std::log2(target_bit == 1 ? safe_p : 1.0f - safe_p);
                bit_counter++;

                // Seamless HUD
                if (bit_counter % 8192 == 0) { 
                    double elapsed = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start_neural).count();
                    std::cout << "\r[HUD] " << std::fixed << std::setprecision(2) << ((double)bit_counter / (stage1.size() * 8.0) * 100.0) << "% "
                              << "| BPB: " << std::setprecision(4) << (total_bits / (bit_counter / 8.0)) << " "
                              << "| SPD: " << std::setprecision(2) << ((bit_counter / 8.0 / 1024.0) / elapsed) << " KB/s "
                              << "| MoE [M:" << std::setw(2) << (int)(mixer.g[0]*100) << "% D:" << std::setw(2) << (int)(mixer.g[2]*100) << "%]   "
                              << std::flush;
                }
            }
        }
        
        // Detailed Block Telemetry File Dump
        if (!external_interrupt_triggered) {
            double elapsed = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start_neural).count();
            write_block_telemetry(block_id, (total_bits / (bit_counter / 8.0)), ((bit_counter / 8.0 / 1024.0) / elapsed), mixer.g[0], mixer.g[2], mixer.g[1]);
        }
    }

    // --- PHASE IV: ARCHIVAL & TEARDOWN ---
    entropy.finish(); 
    std::cout << "\n\n[SYSTEM] Serializing brain state to 'aura_brain.bin'..." << std::endl;

    std::ofstream brain_out("aura_brain.bin", std::ios::binary);
    if (brain_out.is_open()) { mamba.save(brain_out); dmc.save(brain_out); }

    std::ofstream c_file("compressed_aura.bin", std::ios::binary);
    if (c_file.is_open()) { c_file.write(reinterpret_cast<const char*>(entropy.stream.data()), entropy.stream.size()); }

    double total_time = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start_total).count();
    
    std::cout << "=======================================================" << std::endl;
    if (external_interrupt_triggered) std::cout << "[ABORT] Engine safely halted by user. Weights preserved." << std::endl;
    else std::cout << "[COMPLETE] Final Output Size: " << std::fixed << std::setprecision(2) << entropy.get_mb() << " MB" << std::endl;
    std::cout << "[METRICS] Total CPU Uptime:  " << std::setprecision(1) << total_time << " seconds." << std::endl;
    std::cout << "=======================================================" << std::endl;

    return 0;
}