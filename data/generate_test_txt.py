import random
import os
import argparse


def generate_complex_text():
    # Base paragraph templates with varied topics
    paragraphs = [
        "The morning sun cast long shadows across the bustling metropolis as commuters hurried through the labyrinthine streets, each person carrying their own unique story of dreams, aspirations, and daily struggles. Traffic lights orchestrated the urban symphony while vendors called out their wares from every corner, creating a tapestry of human experience that stretched from the gleaming skyscrapers to the humble storefronts below. In this concrete jungle, technology and tradition danced together in an endless waltz of progress and preservation.",

        "Deep within the ancient forest, where towering oaks had stood sentinel for centuries, a delicate ecosystem thrived in perfect harmony. Shafts of golden sunlight filtered through the emerald canopy, illuminating dewdrops that clung to spider webs like nature's own jewelry. The forest floor carpeted with fallen leaves told stories of countless seasons, while woodland creatures went about their timeless rituals, unaware of the modern world that pressed ever closer to their sanctuary.",

        "The laboratory hummed with quiet intensity as researchers worked tirelessly on breakthrough discoveries that could reshape humanity's understanding of the universe. Microscopes revealed hidden worlds within worlds, while complex equations filled whiteboards with the language of scientific inquiry. Each experiment built upon generations of accumulated knowledge, pushing the boundaries of what was thought possible and opening new frontiers for future exploration.",

        "Across the vast ocean, waves crashed against weathered cliffs where seabirds nested in precarious harmony with the elements. The salt-tinged air carried stories from distant shores, while tides followed their ancient rhythm, indifferent to human concerns yet intimately connected to the cycles that governed all life on Earth. Fishing vessels dotted the horizon like scattered thoughts on the mind of the sea.",

        "In the heart of the old city, cobblestone streets wound between buildings that had witnessed centuries of human drama. Cafes with outdoor seating invited contemplation, while street musicians added melody to the urban conversation. History layered upon history created a palimpsest of culture, where each generation had left its mark while building upon the foundations laid by those who came before.",

        "The mountain peak rose majestically above the clouds, offering a perspective that humbled all who reached its summit. Alpine flowers bloomed in defiance of the harsh conditions, while eagles soared on thermals that carried them effortlessly across the valley below. Here, in this realm of stone and sky, the essential truths of existence seemed clearer, stripped of the complications that cluttered life at lower elevations.",

        "Within the pages of the ancient library, knowledge accumulated like snowflakes in a winter storm, each volume contributing to an ever-growing monument to human curiosity and learning. Scholars bent over manuscripts, their fingers tracing words that connected them across centuries to minds that had grappled with similar questions about existence, meaning, and the nature of reality itself.",

        "The spacecraft drifted silently through the void, its instruments recording data from regions of space that human eyes had never seen. Stars that had burned for billions of years served as waypoints on humanity's greatest journey, while the vast distances between celestial bodies put into perspective both the grandeur and the fragility of life on Earth.",

        "In the artist's studio, creativity flowed like water finding its natural course, transforming raw materials into expressions of the human spirit that would outlive their creator. Brushstrokes captured fleeting moments of inspiration, while sculptures emerged from stone as if they had always been waiting to be discovered rather than created.",

        "The small farming community nestled in the valley represented a way of life that connected its inhabitants directly to the rhythms of the seasons and the generosity of the earth. Generations of families had worked the same fields, each adding their own innovations while maintaining the essential wisdom that made sustainable agriculture possible in an ever-changing world."
    ]

    # Sentence fragments to add variety
    connectors = [
        " Furthermore, the intricate relationships between ",
        " Meanwhile, across the landscape, ",
        " However, beneath the surface, ",
        " Nevertheless, the profound implications of ",
        " Subsequently, the remarkable discovery that ",
        " Consequently, the delicate balance between ",
        " Moreover, the unexpected emergence of ",
        " Indeed, the fascinating interplay of ",
        " Therefore, the complex dynamics of ",
        " Additionally, the remarkable phenomenon of "
    ]

    topics = [
        "quantum mechanics and classical physics",
        "digital innovation and traditional craftsmanship",
        "urban development and environmental conservation",
        "artificial intelligence and human creativity",
        "global connectivity and local community",
        "scientific progress and ethical considerations",
        "economic growth and social equity",
        "technological advancement and cultural preservation",
        "individual expression and collective responsibility",
        "natural selection and conscious evolution"
    ]

    endings = [
        " continues to shape our understanding of the world around us.",
        " reveals new possibilities for future generations to explore.",
        " demonstrates the remarkable adaptability of human nature.",
        " challenges our preconceptions about reality and existence.",
        " offers hope for solving some of humanity's greatest challenges.",
        " illustrates the interconnectedness of all living systems.",
        " provides insight into the fundamental forces that govern our universe.",
        " reminds us of our responsibility as stewards of this planet.",
        " opens new avenues for creative expression and innovation.",
        " bridges the gap between scientific knowledge and practical wisdom."
    ]

    # Generate a complex paragraph by combining elements
    base = random.choice(paragraphs)
    connector = random.choice(connectors)
    topic = random.choice(topics)
    ending = random.choice(endings)

    return base + connector + topic + ending

def create_large_text_file(filename="dummy.txt", target_size_gb=1.0):
    target_size_bytes = int(target_size_gb * 1024 * 1024 * 1024)  # Convert GB to bytes
    current_size = 0
    paragraph_count = 0

    print(f"Generating complex text file: {filename}")
    print(f"Target size: {target_size_gb:.2f} GB ({target_size_bytes:,} bytes)")

    with open(filename, 'w', encoding='utf-8') as f:
        while current_size < target_size_bytes:
            # Generate a complex paragraph
            paragraph = generate_complex_text()

            # Add some variation in paragraph structure
            if paragraph_count % 3 == 0:
                paragraph = paragraph + "\n\n" + generate_complex_text()

            # Add paragraph breaks
            full_paragraph = paragraph + "\n\n"

            # Check if adding this paragraph would exceed our target
            paragraph_bytes = len(full_paragraph.encode('utf-8'))
            if current_size + paragraph_bytes > target_size_bytes:
                # Add a partial paragraph to get close to target size
                remaining_bytes = target_size_bytes - current_size
                partial_text = full_paragraph[:remaining_bytes//2]  # Rough estimate
                f.write(partial_text)
                current_size += len(partial_text.encode('utf-8'))
                break

            f.write(full_paragraph)
            current_size += paragraph_bytes
            paragraph_count += 1

            # Progress indicator
            if paragraph_count % 1000 == 0:
                progress = (current_size / target_size_bytes) * 100
                print(f"\rProgress: {progress:.1f}% ({current_size:,} bytes)", end='', flush=True)

    # Get final file size
    final_size = os.path.getsize(filename)
    final_size_gb = final_size / (1024 * 1024 * 1024)

    print(f"\n\nFile generation complete!")
    print(f"Filename: {filename}")
    print(f"Final size: {final_size_gb:.3f} GB ({final_size:,} bytes)")
    print(f"Paragraphs generated: {paragraph_count}")

def parse_arguments():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Generate a large text file with complex, varied content.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
  Examples:
  python script.py                                 # Default: 1GB file named 'dummy.txt'
  python script.py -s 2.5                          # Generate 2.5GB file
  python script.py -f my_large_file.txt            # Custom filename
  python script.py -s 0.5 -f small_test.txt        # 0.5GB file with custom name
  python script.py --size 1.8 --filename data.txt  # Long form arguments
  """
    )

    parser.add_argument(
        '-f', '--filename',
        default='dummy.txt',
        help='Output filename (default: dummy.txt)'
    )

    parser.add_argument(
        '-s', '--size',
        type=float,
        default=1.0,
        help='Target file size in GB (default: 1.0)'
    )

    parser.add_argument(
        '--seed',
        type=int,
        help='Random seed for reproducible output (optional)'
    )

    return parser.parse_args()

if __name__ == "__main__":
    args = parse_arguments()

    # Set random seed if provided
    if args.seed:
        random.seed(args.seed)
        print(f"Using random seed: {args.seed}")

    # Validate arguments
    if args.size <= 0:
        print("Error: Size must be greater than 0")
        exit(1)

    if args.size > 10:
        response = input(f"Warning: You're about to create a {args.size}GB file. Continue? (y/N): ")
        if response.lower() != 'y':
            print("Operation cancelled.")
            exit(0)

    create_large_text_file(args.filename, args.size)
