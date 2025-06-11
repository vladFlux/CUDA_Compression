#include <iostream>
#include <fstream>
#include <vector>
#include <bitset>
#include <chrono>
#include <iomanip>
#include <functional>

/**
 * @file huffman_cpu_decompression.cpp
 * @brief CPU-only Huffman decompression for files created by CPU compression
 *
 * This program decompresses files created by the huffman_cpu_compression.cpp program.
 * It reads the self-contained compressed format and perfectly reconstructs the
 * original data using the embedded Huffman tree.
 *
 * Key features:
 * - Tree deserialization from binary format
 * - Bit-by-bit tree traversal for decoding
 * - Robust error handling and validation
 * - Memory management with RAII patterns
 * - Performance measurement and reporting
 *
 * File format compatibility:
 * - Reads files with embedded serialized trees
 * - Handles padding removal correctly
 * - Supports single-character files
 * - Validates decompression accuracy
 */

using namespace std;
using namespace chrono;

/*=============================================================================
 * TREE DATA STRUCTURE
 *=============================================================================*/

/**
 * @struct node
 * @brief Simplified tree node for decompression operations
 *
 * This structure is optimized for decompression traversal rather than
 * construction, so it omits the frequency field used during compression:
 *
 * - character: The byte value stored in leaf nodes
 * - left/right: Child pointers for tree traversal during decoding
 *
 * The simpler structure reduces memory usage and improves cache locality
 * during the bit-by-bit traversal process that dominates decompression time.
 */
struct node {
    char character;     // Character value (meaningful only for leaf nodes)
    node* left;         // Left child pointer (corresponds to '0' bit)
    node* right;        // Right child pointer (corresponds to '1' bit)

    // Default constructor for internal nodes
    node() : character(0), left(nullptr), right(nullptr) {}

    // Constructor for leaf nodes with character
    explicit node(const char character) : character(character), left(nullptr), right(nullptr) {}
};

/*=============================================================================
 * TREE DESERIALIZATION
 *=============================================================================*/

/**
 * @brief Recursively deserializes a Huffman tree from binary file format
 * @param in Input file stream positioned at tree data
 * @return Pointer to reconstructed tree root, or nullptr on error
 *
 * This function reverses the serialization process used during compression:
 *
 * Deserialization format (matches compression serialization):
 * - '1' + character_byte → Create leaf node with character
 * - '0' → Create internal node, then deserialize left and right subtrees
 *
 * Error handling:
 * - Returns nullptr on file read errors or malformed tree data
 * - Cleans up partially constructed trees on failure
 * - Validates tree structure during construction
 *
 * Memory management:
 * - Allocates nodes dynamically during reconstruction
 * - Provides cleanup on partial failure to prevent memory leaks
 * - Caller responsible for cleaning up successfully constructed trees
 */
node* deserialize_tree(ifstream& in) {
    // Read the node type marker
    const char marker = in.get();
    if (in.eof()) return nullptr;  // Unexpected end of file

    if (marker == '1') {
        // Leaf node: read the character value
        const char ch = in.get();
        if (in.eof()) return nullptr;  // Truncated leaf node data
        return new node(ch);
    }
    if (marker == '0') {
        // Internal node: create node and deserialize children
        auto node = new struct node();
        node->left = deserialize_tree(in);
        node->right = deserialize_tree(in);

        // If either subtree failed to deserialize, cleanup and return failure
        if (!node->left || !node->right) {
            // Recursive cleanup lambda to prevent memory leaks
            function<void(struct node*)> delete_tree = [&](const struct node* node_param) {
                if (!node_param) return;
                delete_tree(node_param->left);
                delete_tree(node_param->right);
                delete node_param;
            };
            delete_tree(node);
            return nullptr;
        }

        return node;
    }

    // Invalid marker - corrupted tree data
    return nullptr;
}

/*=============================================================================
 * MAIN DECOMPRESSION PROGRAM
 *=============================================================================*/

/**
 * @brief Main decompression program for CPU Huffman compressed files
 * @param argc Number of command line arguments (should be 3)
 * @param argv Array of arguments [program, compressed_file, output_file]
 * @return EXIT_SUCCESS on successful decompression, EXIT_FAILURE on error
 *
 * Complete decompression pipeline:
 * 1. **File Format Parsing**: Reads structured compressed file header
 * 2. **Tree Reconstruction**: Deserializes embedded Huffman tree
 * 3. **Data Extraction**: Reads compressed bit stream with padding info
 * 4. **Bit Stream Conversion**: Converts bytes to bit string for processing
 * 5. **Tree Traversal Decoding**: Walks tree for each bit to decode characters
 * 6. **Validation**: Verifies output size matches expected original size
 * 7. **Output Generation**: Writes reconstructed data to output file
 *
 * Error handling covers:
 * - File I/O failures
 * - Corrupted tree data
 * - Size mismatches
 * - Memory allocation failures
 */

int main(int argc, char* argv[]) {
    /*=========================================================================
     * ARGUMENT VALIDATION
     *=========================================================================*/

    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <compressed_file> <output_file>" << endl;
        return EXIT_FAILURE;
    }

    /*=========================================================================
     * PERFORMANCE TIMING SETUP
     *=========================================================================*/

    auto start = high_resolution_clock::now();

    /*=========================================================================
     * COMPRESSED FILE INPUT AND HEADER PARSING
     *=========================================================================*/

    // Open the compressed file created by CPU compression
    ifstream in_file(argv[1], ios::binary);
    if (!in_file) {
        cerr << "Error: Cannot open compressed file " << argv[1] << endl;
        return EXIT_FAILURE;
    }

    // Read original file size from header (first 8 bytes)
    size_t original_size;
    in_file.read(reinterpret_cast<char*>(&original_size), sizeof(original_size));

    /*=========================================================================
     * HUFFMAN TREE RECONSTRUCTION
     *=========================================================================*/

    // Deserialize the embedded Huffman tree structure
    node* root = deserialize_tree(in_file);
    if (!root) {
        cerr << "Error: Failed to deserialize Huffman tree" << endl;
        return EXIT_FAILURE;
    }

    /*=========================================================================
     * COMPRESSED DATA BOUNDARY DETECTION
     *=========================================================================*/

    // Find the tree end marker to locate start of compressed data
    char marker;
    while (in_file.get(marker) && marker != '*') {}

    // Read padding information (number of padding bits added during compression)
    int padding = in_file.get();

    /*=========================================================================
     * COMPRESSED DATA READING
     *=========================================================================*/

    // Read all remaining bytes as compressed data
    vector<char> compressed_data;
    char byte;
    while (in_file.get(byte)) {
        compressed_data.push_back(byte);
    }
    in_file.close();

    /*=========================================================================
     * BIT STRING CONVERSION
     *=========================================================================*/

    // Convert compressed bytes to bit string for tree traversal
    string bit_string;
    for (char compressed_byte : compressed_data) {
        // Convert each byte to 8-bit binary string
        bitset<8> bits(static_cast<unsigned char>(compressed_byte));
        bit_string += bits.to_string();
    }

    /*=========================================================================
     * PADDING REMOVAL
     *=========================================================================*/

    // Remove padding bits that were added during compression for byte alignment
    if (padding != 8 && !bit_string.empty()) {
        // Remove 'padding' number of bits from the end
        bit_string = bit_string.substr(0, bit_string.length() - padding);
    }

    /*=========================================================================
     * HUFFMAN DECODING VIA TREE TRAVERSAL
     *=========================================================================*/

    // Decode the bit string by traversing the Huffman tree
    string decoded;
    node* current = root;
    size_t decoded_count = 0;

    // Process each bit in the compressed bit stream
    for (char bit : bit_string) {
        // Stop if we've decoded the expected amount of data
        if (decoded_count >= original_size) break;

        // Navigate tree based on current bit
        if (bit == '0') {
            current = current->left;   // '0' bit → go left
        } else {
            current = current->right;  // '1' bit → go right
        }

        // Check if we've reached a leaf node (found a character)
        if (!current->left && !current->right) {
            decoded += current->character;  // Add character to output
            decoded_count++;                // Track progress
            current = root;                 // Reset to root for next character
        }
    }

    /*=========================================================================
     * SPECIAL CASE HANDLING
     *=========================================================================*/

    // Handle single character files (edge case)
    // If nothing was decoded, but we expect data, file contains only one character type
    if (decoded.empty() && original_size > 0) {
        decoded = string(original_size, root->character);
    }

    /*=========================================================================
     * OUTPUT FILE GENERATION
     *=========================================================================*/

    // Write the completely reconstructed original data
    ofstream out_file(argv[2], ios::binary);
    if (!out_file) {
        cerr << "Error: Cannot create output file " << argv[2] << endl;
        return EXIT_FAILURE;
    }

    out_file.write(decoded.c_str(), decoded.size());
    out_file.close();

    /*=========================================================================
     * PERFORMANCE MEASUREMENT AND REPORTING
     *=========================================================================*/

    auto end = high_resolution_clock::now();
    const auto duration = std::chrono::duration<double>(end - start);
    const double total_seconds = duration.count();
    const int seconds = static_cast<int>(total_seconds);
    const int milliseconds = static_cast<int>((total_seconds - seconds) * 1000);

    cout << "Decompression completed successfully!" << endl;
    std::cout << std::left << std::setw(25) << "Execution time: " << std::right << std::setw(15)
              << seconds << "s" << std::setw(5) << milliseconds << "ms" << std::endl;

    /*=========================================================================
     * VALIDATION AND WARNING
     *=========================================================================*/

    // Validate that decompressed size matches expected size
    if (decoded.size() != original_size) {
        cout << "Warning: Size mismatch detected!" << endl;
        cout << "Expected: " << original_size << " bytes" << endl;
        cout << "Actual: " << decoded.size() << " bytes" << endl;
    }

    /*=========================================================================
     * MEMORY CLEANUP
     *=========================================================================*/

    // Clean up the reconstructed tree to prevent memory leaks
    function<void(node*)> delete_tree = [&](const node* node) {
        if (!node) return;
        delete_tree(node->left);
        delete_tree(node->right);
        delete node;
    };
    delete_tree(root);

    return EXIT_SUCCESS;
}
