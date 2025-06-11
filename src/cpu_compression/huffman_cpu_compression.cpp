#include <iostream>
#include <fstream>
#include <unordered_map>
#include <queue>
#include <vector>
#include <bitset>
#include <chrono>
#include <cstring>
#include <functional>
#include <iomanip>


/**
 * @file huffman_cpu_compression.cpp
 * @brief CPU-only Huffman compression implementation using modern C++
 *
 * This file provides a complete, self-contained Huffman compression system
 * that runs entirely on the CPU using modern C++ features and STL containers.
 *
 * Key differences from GPU version:
 * - Uses STL containers (priority_queue, unordered_map, string) for simplicity
 * - Tree serialization for self-contained compressed files
 * - Direct bit manipulation using bitset
 * - Memory management with RAII and smart cleanup
 *
 * Output file format:
 * 1. Original file size (8 bytes)
 * 2. Serialized Huffman tree (variable length)
 * 3. Tree end marker ('*')
 * 4. Padding information (1 byte)
 * 5. Compressed data (variable length)
 *
 * This format enables decompression without external metadata files.
 */

using namespace std;
using namespace chrono;

/*=============================================================================
 * CORE DATA STRUCTURES
 *=============================================================================*/

/**
 * @struct node
 * @brief Binary tree node for Huffman tree construction and serialization
 *
 * This structure serves dual purposes:
 * 1. During construction: holds character frequencies for priority queue
 * 2. After construction: forms the binary tree for code generation
 *
 * Design features:
 * - character: The byte value this leaf represents (0 for internal nodes)
 * - frequency: Count of occurrences (used for tree construction priority)
 * - left/right: Child pointers forming the binary tree structure
 *
 * Memory management: Uses raw pointers with explicit cleanup via lambda
 */
struct node {
    char character; // Character value (meaningful only for leaf nodes)
    int frequency; // Occurrence count (drives tree construction order)
    node *left; // Left child pointer (0-bit path)
    node *right; // Right child pointer (1-bit path)

    // Constructor for leaf nodes (character + frequency)
    node(const char character, const int frequency) : character(character), frequency(frequency), left(nullptr),
                                                      right(nullptr) {}

    // Constructor for internal nodes (frequency only)
    explicit node(const int f) : character(0), frequency(f), left(nullptr), right(nullptr) {}
};

/**
 * @struct compare
 * @brief Priority queue comparator for Huffman tree construction
 *
 * Implements the priority logic for building optimal Huffman trees:
 * 1. Primary: Lower frequency = higher priority (min-heap behavior)
 * 2. Secondary: Lower character value = higher priority (deterministic ordering)
 *
 * The secondary comparison ensures reproducible tree structures when
 * multiple characters have identical frequencies, which is important
 * for testing and verification purposes.
 */
struct compare {
    bool operator()(const node *node_1, const node *node_2) const {
        // If frequencies are equal, use character value for deterministic ordering
        if (node_1->frequency == node_2->frequency) {
            return node_1->character > node_2->character;
        }
        // Primary comparison: lower frequency = higher priority
        return node_1->frequency > node_2->frequency;
    }
};

/*=============================================================================
 * HUFFMAN ALGORITHM FUNCTIONS
 *=============================================================================*/

/**
 * @brief Recursively generates Huffman codes by traversing the completed tree
 * @param root Current node in the tree traversal
 * @param code Accumulated bit sequence from root to current node
 * @param codes Output map storing character→bit_sequence mappings
 *
 * This function performs depth-first traversal to generate optimal bit codes:
 * - Left traversal appends '0' to the current code
 * - Right traversal appends '1' to the current code
 * - Leaf nodes store their complete code in the output map
 *
 * Special case handling:
 * - Single character files: assigns code "0" (minimum 1-bit code required)
 * - Multiple characters: natural tree traversal determines code lengths
 *
 * The resulting codes have the prefix property: no code is a prefix of another,
 * enabling unambiguous decoding during decompression.
 */
void generate_codes(const node *root, const string &code, unordered_map<char, string> &codes) {
    if (!root) return;

    // Check if this is a leaf node (contains actual character)
    if (!root->left && !root->right) {
        // Handle edge case: single character file needs at least 1-bit code
        codes[root->character] = code.empty() ? "0" : code;
        return;
    }

    // Recursive traversal: left = '0', right = '1'
    generate_codes(root->left, code + "0", codes);
    generate_codes(root->right, code + "1", codes);
}

/**
 * @brief Serializes the Huffman tree structure to the output file
 * @param root Current node being serialized
 * @param output File stream to write serialized tree data
 *
 * Creates a compact binary representation of the tree structure that can be
 * embedded in the compressed file for self-contained decompression:
 *
 * Serialization format:
 * - Leaf node: '1' + character_byte
 * - Internal node: '0' + left_subtree + right_subtree
 * - Empty node: '0' (should not occur in valid trees)
 *
 * This pre-order traversal format enables efficient tree reconstruction
 * during decompression without requiring separate metadata files.
 *
 * The serialized tree size is typically much smaller than storing a
 * frequency table, especially for files with many unique characters.
 */
void serialize_tree(const node *root, ofstream &output) {
    if (!root) {
        output.put('0'); // Null node marker (shouldn't occur in valid trees)
        return;
    }

    if (!root->left && !root->right) {
        // Leaf node: write marker + character
        output.put('1');
        output.put(root->character);
    } else {
        // Internal node: write marker + serialize children
        output.put('0');
        serialize_tree(root->left, output);
        serialize_tree(root->right, output);
    }
}

/*=============================================================================
 * MAIN COMPRESSION PROGRAM
 *=============================================================================*/

/**
 * @brief Main compression program implementing complete Huffman compression
 * @param argc Number of command line arguments (should be 3)
 * @param argv Array of arguments [program, input_file, output_file]
 * @return EXIT_SUCCESS on successful compression, EXIT_FAILURE on error
 *
 * Complete compression pipeline:
 * 1. **File Input**: Reads entire input file into memory
 * 2. **Frequency Analysis**: Counts occurrence of each byte value
 * 3. **Tree Construction**: Builds optimal Huffman tree using priority queue
 * 4. **Code Generation**: Creates bit sequences for each character
 * 5. **File Output**: Writes structured compressed file with embedded tree
 * 6. **Performance Reporting**: Times the compression process
 *
 * The output file is completely self-contained - no external metadata
 * files are needed for decompression. The embedded tree structure
 * enables the decompressor to reconstruct the exact same codes.
 */
int main(int argc, char *argv[]) {
    /*=========================================================================
     * ARGUMENT VALIDATION
     *=========================================================================*/

    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << endl;
        return EXIT_FAILURE;
    }

    /*=========================================================================
     * PERFORMANCE TIMING SETUP
     *=========================================================================*/

    auto start = high_resolution_clock::now();

    /*=========================================================================
     * FILE INPUT AND VALIDATION
     *=========================================================================*/

    // Read entire input file into memory using iterators
    ifstream input_file(argv[1], ios::binary);
    if (!input_file) {
        cerr << "Error: Cannot open input file " << argv[1] << endl;
        return EXIT_FAILURE;
    }

    // Load complete file content into string for processing
    string content((istreambuf_iterator(input_file)), istreambuf_iterator<char>());
    input_file.close();

    // Validate non-empty input
    if (content.empty()) {
        cerr << "Error: Input file is empty" << endl;
        return EXIT_FAILURE;
    }

    /*=========================================================================
     * FREQUENCY ANALYSIS
     *=========================================================================*/

    // Count frequency of each character using hash map for O(1) access
    unordered_map<char, int> frequency;
    for (char character: content) {
        frequency[character]++;
    }

    /*=========================================================================
     * HUFFMAN TREE CONSTRUCTION
     *=========================================================================*/

    // Build Huffman tree using priority queue (min-heap by frequency)
    priority_queue<node *, vector<node *>, compare> priority_queue;

    // Create leaf nodes for each unique character
    for (auto &[character, frequency]: frequency) {
        priority_queue.push(new node(character, frequency));
    }

    // Combine nodes until only root remains
    // This implements the classic Huffman algorithm
    while (priority_queue.size() > 1) {
        // Extract two nodes with the lowest frequencies
        node *right = priority_queue.top();
        priority_queue.pop();
        node *left = priority_queue.top();
        priority_queue.pop();

        // Create new internal node with combined frequency
        auto merged = new node(left->frequency + right->frequency);
        merged->left = left;
        merged->right = right;

        // Insert back into priority queue
        priority_queue.push(merged);
    }

    // The remaining node is the root of the completed Huffman tree
    node *root = priority_queue.top();

    /*=========================================================================
     * HUFFMAN CODE GENERATION
     *=========================================================================*/

    // Generate optimal bit codes for each character
    unordered_map<char, string> codes;
    if (frequency.size() == 1) {
        // Special case: single character file requires at least 1-bit code
        codes[root->character] = "0";
    } else {
        // Normal case: generate codes via tree traversal
        generate_codes(root, "", codes);
    }

    /*=========================================================================
     * COMPRESSED FILE OUTPUT SETUP
     *=========================================================================*/

    // Create output file for writing compressed data
    ofstream out_file(argv[2], ios::binary);
    if (!out_file) {
        cerr << "Error: Cannot create output file " << argv[2] << endl;

        // Clean up allocated tree memory before exit
        function<void(node *)> delete_tree = [&](const node *node) {
            if (!node) return;
            delete_tree(node->left);
            delete_tree(node->right);
            delete node;
        };
        delete_tree(root);

        return EXIT_FAILURE;
    }

    /*=========================================================================
     * COMPRESSED FILE HEADER WRITING
     *=========================================================================*/

    // Write original file size for decompression buffer allocation
    size_t original_size = content.size();
    out_file.write(reinterpret_cast<const char *>(&original_size), sizeof(original_size));

    // Write serialized tree structure for decompression
    serialize_tree(root, out_file);
    out_file.put('*'); // Tree end marker for parsing during decompression

    /*=========================================================================
     * DATA ENCODING AND COMPRESSION
     *=========================================================================*/

    // Encode entire file content using generated Huffman codes
    string encoded;
    for (char character: content) {
        encoded += codes[character];
    }

    // Pad encoded bit string to byte boundary
    int padding = 8 - (encoded.length() % 8);
    if (padding != 8) {
        encoded += string(padding, '0'); // Add padding zeros
    }
    out_file.put(padding); // Store padding amount for decompression

    /*=========================================================================
     * BINARY DATA WRITING
     *=========================================================================*/

    // Convert encoded bit string to bytes and write to file
    for (size_t index = 0; index < encoded.length(); index += 8) {
        // Extract 8-bit chunk and convert to byte
        bitset<8> byte(encoded.substr(index, 8));
        out_file.put(static_cast<char>(byte.to_ulong()));
    }

    out_file.close();

    /*=========================================================================
     * PERFORMANCE MEASUREMENT AND REPORTING
     *=========================================================================*/

    auto end = high_resolution_clock::now();
    const auto duration = std::chrono::duration<double>(end - start);
    const double total_seconds = duration.count();
    const int seconds = static_cast<int>(total_seconds);
    const int milliseconds = static_cast<int>((total_seconds - seconds) * 1000);

    cout << "CPU Compression completed successfully!" << endl;
    std::cout << std::left << std::setw(25) << "Execution time: " << std::right << std::setw(15) <<
            seconds << "s" << std::setw(5) << milliseconds << "ms" << std::endl;

    /*=========================================================================
     * MEMORY CLEANUP
     *=========================================================================*/

    // Clean up dynamically allocated tree memory using recursive lambda
    function<void(node *)> delete_tree = [&](const node *node) {
        if (!node) return;
        delete_tree(node->left);
        delete_tree(node->right);
        delete node;
    };
    delete_tree(root);

    return EXIT_SUCCESS;
}
