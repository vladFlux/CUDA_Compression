#include <iostream>
#include <fstream>
#include <unordered_map>
#include <queue>
#include <vector>
#include <bitset>
#include <chrono>
#include <cstring>
#include <iomanip>

using namespace std;
using namespace chrono;

struct Node {
    char ch;
    int freq;
    Node* left;
    Node* right;

    Node(char c, int f) : ch(c), freq(f), left(nullptr), right(nullptr) {}
    Node(int f) : ch(0), freq(f), left(nullptr), right(nullptr) {}
};

struct Compare {
    bool operator()(Node* a, Node* b) {
        if (a->freq == b->freq) {
            return a->ch > b->ch;
        }
        return a->freq > b->freq;
    }
};

void generateCodes(Node* root, string code, unordered_map<char, string>& codes) {
    if (!root) return;

    if (!root->left && !root->right) {
        codes[root->ch] = code.empty() ? "0" : code;
        return;
    }

    generateCodes(root->left, code + "0", codes);
    generateCodes(root->right, code + "1", codes);
}

void serializeTree(Node* root, ofstream& out) {
    if (!root) {
        out.put('0');
        return;
    }

    if (!root->left && !root->right) {
        out.put('1');
        out.put(root->ch);
    } else {
        out.put('0');
        serializeTree(root->left, out);
        serializeTree(root->right, out);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << endl;
        return 1;
    }

    auto start = high_resolution_clock::now();

    // Read input file
    ifstream inFile(argv[1], ios::binary);
    if (!inFile) {
        cerr << "Error: Cannot open input file " << argv[1] << endl;
        return 1;
    }

    string content((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();

    if (content.empty()) {
        cerr << "Error: Input file is empty" << endl;
        return 1;
    }

    // Count frequencies
    unordered_map<char, int> freq;
    for (char c : content) {
        freq[c]++;
    }

    // Build Huffman tree
    priority_queue<Node*, vector<Node*>, Compare> pq;
    for (auto& p : freq) {
        pq.push(new Node(p.first, p.second));
    }

    while (pq.size() > 1) {
        Node* right = pq.top(); pq.pop();
        Node* left = pq.top(); pq.pop();

        Node* merged = new Node(left->freq + right->freq);
        merged->left = left;
        merged->right = right;
        pq.push(merged);
    }

    Node* root = pq.top();

    // Generate codes
    unordered_map<char, string> codes;
    if (freq.size() == 1) {
        codes[root->ch] = "0";
    } else {
        generateCodes(root, "", codes);
    }

    // Write compressed file
    ofstream outFile(argv[2], ios::binary);
    if (!outFile) {
        cerr << "Error: Cannot create output file " << argv[2] << endl;
        return 1;
    }

    // Write original file size
    size_t originalSize = content.size();
    outFile.write(reinterpret_cast<const char*>(&originalSize), sizeof(originalSize));

    // Serialize and write tree
    serializeTree(root, outFile);
    outFile.put('*'); // Tree end marker

    // Encode and write data
    string encoded = "";
    for (char c : content) {
        encoded += codes[c];
    }

    // Pad to byte boundary
    int padding = 8 - (encoded.length() % 8);
    if (padding != 8) {
        encoded += string(padding, '0');
    }
    outFile.put(padding);

    // Write encoded data
    for (size_t i = 0; i < encoded.length(); i += 8) {
        bitset<8> byte(encoded.substr(i, 8));
        outFile.put(static_cast<char>(byte.to_ulong()));
    }

    outFile.close();

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    cout << "Compression completed successfully!" << endl;
    cout << "Original size: " << originalSize << " bytes" << endl;
    cout << "Compressed size: " << ifstream(argv[2], ios::ate | ios::binary).tellg() << " bytes" << endl;
    cout << "Compression ratio: " << fixed << setprecision(2)
         << (1.0 - (double)ifstream(argv[2], ios::ate | ios::binary).tellg() / originalSize) * 100 << "%" << endl;
    cout << "Execution time: " << duration.count() << " ms" << endl;

    return 0;
}
