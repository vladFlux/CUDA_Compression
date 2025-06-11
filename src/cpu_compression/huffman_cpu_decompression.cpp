#include <iostream>
#include <fstream>
#include <vector>
#include <bitset>
#include <chrono>
#include <iomanip>

using namespace std;
using namespace chrono;

struct Node {
    char ch;
    Node* left;
    Node* right;

    Node() : ch(0), left(nullptr), right(nullptr) {}
    Node(char c) : ch(c), left(nullptr), right(nullptr) {}
};

Node* deserializeTree(ifstream& in) {
    char marker = in.get();
    if (in.eof()) return nullptr;

    if (marker == '1') {
        // Leaf node
        char ch = in.get();
        return new Node(ch);
    } else if (marker == '0') {
        // Internal node
        Node* node = new Node();
        node->left = deserializeTree(in);
        node->right = deserializeTree(in);
        return node;
    }
    return nullptr;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <compressed_file> <output_file>" << endl;
        return 1;
    }

    auto start = high_resolution_clock::now();

    // Open compressed file
    ifstream inFile(argv[1], ios::binary);
    if (!inFile) {
        cerr << "Error: Cannot open compressed file " << argv[1] << endl;
        return 1;
    }

    // Read original file size
    size_t originalSize;
    inFile.read(reinterpret_cast<char*>(&originalSize), sizeof(originalSize));

    // Deserialize tree
    Node* root = deserializeTree(inFile);
    if (!root) {
        cerr << "Error: Failed to deserialize Huffman tree" << endl;
        return 1;
    }

    // Find tree end marker
    char marker;
    while (inFile.get(marker) && marker != '*');

    // Read padding info
    int padding = inFile.get();

    // Read compressed data
    vector<char> compressedData;
    char byte;
    while (inFile.get(byte)) {
        compressedData.push_back(byte);
    }
    inFile.close();

    // Convert to bit string
    string bitString = "";
    for (char byte : compressedData) {
        bitset<8> bits(static_cast<unsigned char>(byte));
        bitString += bits.to_string();
    }

    // Remove padding
    if (padding != 8 && !bitString.empty()) {
        bitString = bitString.substr(0, bitString.length() - padding);
    }

    // Decode data
    string decoded = "";
    Node* current = root;
    size_t decodedCount = 0;

    for (char bit : bitString) {
        if (decodedCount >= originalSize) break;

        if (bit == '0') {
            current = current->left;
        } else {
            current = current->right;
        }

        // Check if we reached a leaf
        if (!current->left && !current->right) {
            decoded += current->ch;
            decodedCount++;
            current = root;
        }
    }

    // Handle single character case
    if (decoded.empty() && originalSize > 0) {
        decoded = string(originalSize, root->ch);
    }

    // Write decompressed file
    ofstream outFile(argv[2], ios::binary);
    if (!outFile) {
        cerr << "Error: Cannot create output file " << argv[2] << endl;
        return 1;
    }

    outFile.write(decoded.c_str(), decoded.size());
    outFile.close();

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    cout << "Decompression completed successfully!" << endl;
    cout << "Decompressed size: " << decoded.size() << " bytes" << endl;
    cout << "Expected size: " << originalSize << " bytes" << endl;
    cout << "Execution time: " << duration.count() << " ms" << endl;

    if (decoded.size() != originalSize) {
        cout << "Warning: Size mismatch detected!" << endl;
    }

    return 0;
}
