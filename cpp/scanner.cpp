#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <algorithm>
#include <string_view>
#include <cstdint> // <--- The missing link!

struct Candidate {
    std::string text;
    long long savings;
};

int main() {
    std::cout << "[SYSTEM] Initializing Aura-Scanner..." << std::endl;
    
    // 1. Load enwik9 into memory
    std::ifstream file("enwik9", std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "[ERROR] enwik9 not found! Ensure it's in the same folder." << std::endl;
        return 1;
    }
    
    std::streamsize sz = file.tellg();
    file.seekg(0);
    std::vector<char> data(sz); // Swapped to char for smoother string_view work
    file.read(data.data(), sz);

    std::string_view full_text(data.data(), data.size());
    std::unordered_map<std::string_view, int> freq_map;

    std::cout << "[SCAN] Identifying space-savers (Sampling 5% of file)..." << std::endl;

    // 2. Scan for patterns (Lengths 8 to 32)
    for (int len : {8, 12, 16, 24, 32}) {
        for (size_t i = 0; i + len < data.size(); i += 20) { 
            char start = full_text[i];
            // Filter for high-value Wikipedia/XML markers
            if (start == '<' || start == '[' || start == ' ' || start == '{') {
                std::string_view s = full_text.substr(i, len);
                freq_map[s]++;
            }
        }
    }

    // 3. Score and Sort
    std::vector<Candidate> list;
    for (auto const& [view, freq] : freq_map) {
        if (freq < 10) continue;
        // Savings = Frequency * (Length - 2 marker bytes)
        long long score = (long long)freq * (view.length() - 2);
        list.push_back({std::string(view), score});
    }

    std::sort(list.begin(), list.end(), [](const Candidate& a, const Candidate& b) {
        return a.savings > b.savings;
    });

    // 4. Output as C++ Code
    std::cout << "\n--- TOP 256 DICTIONARY ENTRIES ---\n" << std::endl;
    std::cout << "std::vector<std::string> dict = {" << std::endl;
    for (int i = 0; i < 256 && i < list.size(); i++) {
        std::string cleaned = "";
        for(char c : list[i].text) {
            if (c == '\"') cleaned += "\\\"";
            else if (c == '\\') cleaned += "\\\\";
            else if (c == '\n') cleaned += "\\n";
            else if (c < 32 || c > 126) cleaned += " "; 
            else cleaned += c;
        }
        std::cout << "    \"" << cleaned << "\"," << std::endl;
    }
    std::cout << "};" << std::endl;

    return 0;
}