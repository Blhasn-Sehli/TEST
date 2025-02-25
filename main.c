#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <limits.h>

#define INF INT_MAX

#define CELL_SIZE 35
#define MAX_POINTS 1500 // 1 pour début, 1 pour fin, 100 pour les mots (par exemple) exactement dans le calcul de la distance !!!!

typedef struct Node
{
    int x, y;
    struct Node **neighbors;
    int neighbor_count;
    char letter;
    bool visited;
    bool is_part_of_word;
} Node;

typedef struct
{
    const char *word; // Le mot en question
    int startX;       // Coordonnée X de départ
    int startY;       // Coordonnée Y de départ
    int endX;
    int endY;
    int direction; // 0 pour horizontal, 1 pour vertical
    int length;    // Longueur du mot
} WordPosition;

typedef struct
{
    Node **nodes;
    int node_count;
    Node *start;
    Node *end;
} Graph;

typedef struct
{
    int x, y;
    int score;
    char path[200]; // Path to store collected letters
} Player;

typedef struct PriorityQueue
{
    Node *nodes[MAX_POINTS];
    int distances[MAX_POINTS];
    int size;
} PriorityQueue;

// Create a node
Node *create_node(int x, int y)
{
    Node *node = (Node *)malloc(sizeof(Node));
    if (!node)
    {
        printf("Memory allocation error for node.\n");
        exit(1);
    }
    node->x = x;
    node->y = y;
    node->neighbor_count = 0;
    node->neighbors = (Node **)malloc(8 * sizeof(Node *));
    node->letter = ' '; // Initialize as empty space
    node->visited = false;
    node->is_part_of_word = false;

    if (!node->neighbors)
    {
        printf("Memory allocation error for neighbors.\n");
        free(node);
        exit(1);
    }
    return node;
}

// Create graph
Graph *create_graph(int GRID_SIZE)
{
    Graph *graph = (Graph *)malloc(sizeof(Graph));
    graph->nodes = (Node **)malloc(GRID_SIZE * GRID_SIZE * sizeof(Node *));
    graph->node_count = 0;
    graph->start = NULL;
    graph->end = NULL;
    return graph;
}

// Add node to graph
void add_node(Graph *graph, Node *node)
{
    graph->nodes[graph->node_count++] = node;
}

// Add an edge
void add_edge(Node *node1, Node *node2)
{
    // Avoid self-loops
    if (node1 == node2)
    {
        // printf("Skipping self-loop at (%d, %d)\n", node1->x, node1->y);
        return;
    }

    // Check if node2 is already in node1's neighbors
    int already_connected = 0;
    for (int i = 0; i < node1->neighbor_count; i++)
    {
        if (node1->neighbors[i] == node2)
        {
            already_connected = 1;
            break;
        }
    }

    // Check if node1 is already in node2's neighbors
    for (int i = 0; i < node2->neighbor_count; i++)
    {
        if (node2->neighbors[i] == node1)
        {
            already_connected = 1;
            break;
        }
    }

    // If already connected, skip adding the edge
    if (already_connected)
    {
        // printf("Edge between (%d, %d) and (%d, %d) already exists. Skipping.\n",
        //        node1->x, node1->y, node2->x, node2->y);
        return;
    }

    // Add node2 to node1's neighbors
    node1->neighbors = (Node **)realloc(node1->neighbors, (node1->neighbor_count + 1) * sizeof(Node *));
    node1->neighbors[node1->neighbor_count++] = node2;

    // Add node1 to node2's neighbors
    node2->neighbors = (Node **)realloc(node2->neighbors, (node2->neighbor_count + 1) * sizeof(Node *));
    node2->neighbors[node2->neighbor_count++] = node1;

    // printf("Added edge between (%d, %d) and (%d, %d)\n", node1->x, node1->y, node2->x, node2->y);
}

void print_neighbors(Graph *graph, int GRID_SIZE)
{
    for (int x = 0; x < GRID_SIZE; x++)
    {
        for (int y = 0; y < GRID_SIZE; y++)
        {
            Node *node = graph->nodes[x * GRID_SIZE + y];
            printf("Node (%d, %d) Letter %c has %d neighbors: ", x, y, node->letter, node->neighbor_count);
            for (int i = 0; i < node->neighbor_count; i++)
            {
                printf("(%d, %d) ", node->neighbors[i]->x, node->neighbors[i]->y);
            }
            printf("\n");
        }
    }
}

// Initialize the graph
void initialize_graph(Graph *graph, int GRID_SIZE)
{

    for (int x = 0; x < GRID_SIZE; x++)
    {
        for (int y = 0; y < GRID_SIZE; y++)
        {
            Node *node = create_node(x, y);
            add_node(graph, node);
        }
    }

    for (int x = 0; x < GRID_SIZE; x++)
    {
        for (int y = 0; y < GRID_SIZE; y++)
        {
            Node *node = graph->nodes[x * GRID_SIZE + y];

            if (x > 0)
                add_edge(node, graph->nodes[(x - 1) * GRID_SIZE + y]);
            if (x < GRID_SIZE - 1)
                add_edge(node, graph->nodes[(x + 1) * GRID_SIZE + y]);
            if (y > 0)
                add_edge(node, graph->nodes[x * GRID_SIZE + (y - 1)]);
            if (y < GRID_SIZE - 1)
                add_edge(node, graph->nodes[x * GRID_SIZE + (y + 1)]);

            // Diagonales
            if (x > 0 && y > 0)
                add_edge(node, graph->nodes[(x - 1) * GRID_SIZE + (y - 1)]); // haut-gauche
            if (x > 0 && y < GRID_SIZE - 1)
                add_edge(node, graph->nodes[(x - 1) * GRID_SIZE + (y + 1)]); // haut-droit
            if (x < GRID_SIZE - 1 && y > 0)
                add_edge(node, graph->nodes[(x + 1) * GRID_SIZE + (y - 1)]); // bas-gauche
            if (x < GRID_SIZE - 1 && y < GRID_SIZE - 1)
                add_edge(node, graph->nodes[(x + 1) * GRID_SIZE + (y + 1)]); // bas-droit
        }
    }
}

// Set start and end points
void set_start_end(Graph *graph)
{
    Node *valid_nodes[graph->node_count]; // Array to store valid nodes
    int valid_count = 0;

    // Collect all valid nodes (not walls, empty spaces, or part of a word)
    for (int i = 0; i < graph->node_count; i++)
    {
        // printf("Node letter: %c\n and is_part_of_word: %d\n", graph->nodes[i]->letter, graph->nodes[i]->is_part_of_word);
        if (graph->nodes[i]->letter != '#' && graph->nodes[i]->letter != ' ' && !graph->nodes[i]->is_part_of_word)
        {
            valid_nodes[valid_count++] = graph->nodes[i];
        }
    }

    // Ensure there are at least 2 valid nodes (start and end)
    if (valid_count < 2)
    {
        // printf("Error: Not enough valid nodes for start and end!\n");
        return;
    }

    // Select start position randomly from valid nodes
    graph->start = valid_nodes[rand() % valid_count];

    // Select end position ensuring minimum distance of 5
    do
    {
        graph->end = valid_nodes[rand() % valid_count];
    } while (graph->end == graph->start ||
             abs(graph->end->x - graph->start->x) + abs(graph->end->y - graph->start->y) < 5);

    printf("Start: (%d, %d), End: (%d, %d)\n", graph->start->x, graph->start->y, graph->end->x, graph->end->y);
}

// Charge un dictionnaire de mots depuis un fichier
int load_words(const char *filename, char words[][20], int max_words)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        printf("Erreur : impossible d'ouvrir le fichier %s\n", filename);
        return 0;
    }

    int count = 0;
    while (fscanf(file, "%19s", words[count]) == 1 && count < max_words)
    {
        count++;
    }

    fclose(file);
    return count;
}

int can_place_word(Graph *graph, const char *word, int x, int y, int horizontal, int GRID_SIZE)
{
    int len = strlen(word);
    if (horizontal)
    {
        if (y + len > GRID_SIZE)
            return 0;
        for (int i = 0; i < len; i++)
        {
            Node *node = graph->nodes[x * GRID_SIZE + y + i];
            if (node->letter != ' ' && node->letter != word[i])
                return 0;
        }
    }
    else
    {
        if (x + len > GRID_SIZE)
            return 0;
        for (int i = 0; i < len; i++)
        {
            Node *node = graph->nodes[(x + i) * GRID_SIZE + y];
            if (node->letter != ' ' && node->letter != word[i])
                return 0;
        }
    }
    return 1;
}

int try_place_word(Graph *graph, const char *word, WordPosition *word_positions, int *word_count, int GRID_SIZE)
{
    int len = strlen(word);
    int attempts = 100;

    while (attempts-- > 0)
    {
        int horizontal = rand() % 2;
        int x = rand() % GRID_SIZE;
        int y = rand() % GRID_SIZE;

        if (can_place_word(graph, word, x, y, horizontal, GRID_SIZE))
        {
            // Place the word on the grid
            for (int i = 0; i < len; i++)
            {
                Node *node = graph->nodes[(horizontal ? x : x + i) * GRID_SIZE + (horizontal ? y + i : y)];
                node->letter = word[i];
                node->is_part_of_word = true;
            }

            // Save the position and information of the word
            word_positions[*word_count].word = word;
            word_positions[*word_count].direction = horizontal;
            word_positions[*word_count].length = len;

            // Store the start position (x, y) of the word
            word_positions[*word_count].startX = x;
            word_positions[*word_count].startY = y;

            // Calculate the end position based on direction and word length
            if (horizontal)
            {
                word_positions[*word_count].endX = x;
                word_positions[*word_count].endY = y + len - 1;
            }
            else
            {
                word_positions[*word_count].endX = x + len - 1;
                word_positions[*word_count].endY = y;
            }

            // Increment the word count
            (*word_count)++;

            return 1;
        }
    }
    return 0;
}

void place_words(Graph *graph, const char *words[], WordPosition *word_positions, int *word_count, int word_count_total, int GRID_SIZE)
{
    for (int i = 0; i < word_count_total; i++)
    {
        if (!try_place_word(graph, words[i], word_positions, word_count, GRID_SIZE))
        {
            printf("⚠️ Impossible de placer le mot: %s\n", words[i]);
        }
    }
}

// Initialize the player
void initialize_player(Player *player, Graph *graph)
{
    player->x = graph->start->x;
    player->y = graph->start->y;
    player->score = 0;
}

// Move the player
// Update the move_player function
void move_player(Player *player, Graph *graph, int dx, int dy, int GRID_SIZE)
{
    int new_x = player->x + dx;
    int new_y = player->y + dy;

    for (int i = 0; i < graph->node_count; i++)
    {
        Node *node = graph->nodes[i];
        if (node->x == new_x && node->y == new_y && node->letter != '#')
        {
            player->x = new_x;
            player->y = new_y;
            Node *currentNode = graph->nodes[player->x * GRID_SIZE + player->y];
            currentNode->visited = true;
            // Check if the player collects a letter
            if (node->letter != ' ')
            {
                // player->score += 10;  // Increase score when collecting a letter
                // node->letter = ' ';
                // add the letter to the player path
                player->path[strlen(player->path)] = node->letter;
            }
            break;
        }
    }
}

// Function to remove an edge between two nodes
void remove_edge(Node *node1, Node *node2)
{
    for (int i = 0; i < node1->neighbor_count; i++)
    {
        if (node1->neighbors[i] == node2)
        {
            // Shift neighbors left to remove the connection
            for (int j = i; j < node1->neighbor_count - 1; j++)
            {
                node1->neighbors[j] = node1->neighbors[j + 1];
            }
            node1->neighbor_count--;
            break;
        }
    }

    for (int i = 0; i < node2->neighbor_count; i++)
    {
        if (node2->neighbors[i] == node1)
        {
            for (int j = i; j < node2->neighbor_count - 1; j++)
            {
                node2->neighbors[j] = node2->neighbors[j + 1];
            }
            node2->neighbor_count--;
            break;
        }
    }
}

// Function to add a wall by removing edges and marking the grid
void add_wall(Graph *graph, int x1, int y1, int x2, int y2, int GRID_SIZE){
    int passage_x = x1 + rand() % (x2 - x1 + 1);
    int passage_y = y1 + rand() % (y2 - y1 + 1);
    if (x1 == x2){ // Vertical wall
        for (int y = y1; y <= y2; y++){
            if (y != passage_y){ // Leave a passage
                Node *node = graph->nodes[x1 * GRID_SIZE + y];
                if (node->letter == ' '){ // Only mark as a wall if it's empty
                    node->letter = '#';
                    // Remove edges to disconnect from neighbors
                    if (x1 > 0)
                        remove_edge(node, graph->nodes[(x1 - 1) * GRID_SIZE + y]); // Left
                    if (x1 < GRID_SIZE - 1)
                        remove_edge(node, graph->nodes[(x1 + 1) * GRID_SIZE + y]); // Right
                    if (y > 0)
                        remove_edge(node, graph->nodes[x1 * GRID_SIZE + (y - 1)]); // Up
                    if (y < GRID_SIZE - 1)
                        remove_edge(node, graph->nodes[x1 * GRID_SIZE + (y + 1)]); // Down
                    if (x1 > 0 && y > 0)
                        remove_edge(node, graph->nodes[(x1 - 1) * GRID_SIZE + (y - 1)]); // Top-left diagonal
                    if (x1 > 0 && y < GRID_SIZE - 1)
                        remove_edge(node, graph->nodes[(x1 - 1) * GRID_SIZE + (y + 1)]); // Top-right diagonal
                    if (x1 < GRID_SIZE - 1 && y > 0)
                        remove_edge(node, graph->nodes[(x1 + 1) * GRID_SIZE + (y - 1)]); // Bottom-left diagonal
                    if (x1 < GRID_SIZE - 1 && y < GRID_SIZE - 1)
                        remove_edge(node, graph->nodes[(x1 + 1) * GRID_SIZE + (y + 1)]); // Bottom-right diagonal
                }
            }
        }
    }
    else if (y1 == y2)
    { // Horizontal wall
        for (int x = x1; x <= x2; x++)
        {
            if (x != passage_x)
            { // Leave a passage
                Node *node = graph->nodes[x * GRID_SIZE + y1];
                if (node->letter == ' ')
                { // Only mark as a wall if it's empty
                    node->letter = '#';

                    // Remove edges to disconnect from neighbors
                    if (x > 0)
                        remove_edge(node, graph->nodes[(x - 1) * GRID_SIZE + y1]); // Left
                    if (x < GRID_SIZE - 1)
                        remove_edge(node, graph->nodes[(x + 1) * GRID_SIZE + y1]); // Right
                    if (y1 > 0)
                        remove_edge(node, graph->nodes[x * GRID_SIZE + (y1 - 1)]); // Up
                    if (y1 < GRID_SIZE - 1)
                        remove_edge(node, graph->nodes[x * GRID_SIZE + (y1 + 1)]); // Down
                    if (x > 0 && y1 > 0)
                        remove_edge(node, graph->nodes[(x - 1) * GRID_SIZE + (y1 - 1)]); // Top-left diagonal
                    if (x < GRID_SIZE - 1 && y1 > 0)
                        remove_edge(node, graph->nodes[(x + 1) * GRID_SIZE + (y1 - 1)]); // Top-right diagonal
                    if (x > 0 && y1 < GRID_SIZE - 1)
                        remove_edge(node, graph->nodes[(x - 1) * GRID_SIZE + (y1 + 1)]); // Bottom-left diagonal
                    if (x < GRID_SIZE - 1 && y1 < GRID_SIZE - 1)
                        remove_edge(node, graph->nodes[(x + 1) * GRID_SIZE + (y1 + 1)]); // Bottom-right diagonal
                }
            }
        }
    }
}
// add random LETTERS to the graph
void add_random_letters(Graph *graph, int GRID_SIZE)
{
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            Node *node = graph->nodes[i * GRID_SIZE + j];
            if (node->letter == ' ')
            {
                node->letter = 'A' + rand() % 26;
            }
        }
    }
}

// Recursive function to divide the graph into sections using walls
void divide_graph(Graph *graph, int startX, int startY, int endX, int endY, int GRID_SIZE)
{
    if (endX - startX < 2 || endY - startY < 2)
    {
        return; // Stop when sections are too small
    }

    if (rand() % 2 == 0)
    { // Vertical division
        int divideX = startX + rand() % (endX - startX - 1) + 1;
        add_wall(graph, divideX, startY, divideX, endY, GRID_SIZE);        // Add vertical wall
        divide_graph(graph, startX, startY, divideX - 1, endY, GRID_SIZE); // Left section
        divide_graph(graph, divideX + 1, startY, endX, endY, GRID_SIZE);   // Right section
    }
    else
    { // Horizontal division
        int divideY = startY + rand() % (endY - startY - 1) + 1;
        add_wall(graph, startX, divideY, endX, divideY, GRID_SIZE);        // Add horizontal wall
        divide_graph(graph, startX, startY, endX, divideY - 1, GRID_SIZE); // Top section
        divide_graph(graph, startX, divideY + 1, endX, endY, GRID_SIZE);
        // Bottom section
    }
}

// Function to push a node into the priority queue
void push(PriorityQueue *pq, Node *node, int distance)
{
    pq->nodes[pq->size] = node;
    pq->distances[pq->size] = distance;
    pq->size++;
}

// Function to pop the node with the shortest distance
Node *pop(PriorityQueue *pq)
{
    if (pq->size == 0)
        return NULL;

    int minIndex = 0;
    for (int i = 1; i < pq->size; i++)
    {
        if (pq->distances[i] < pq->distances[minIndex])
        {
            minIndex = i;
        }
    }

    Node *minNode = pq->nodes[minIndex];

    // Remove the node from the queue
    for (int i = minIndex; i < pq->size - 1; i++)
    {
        pq->nodes[i] = pq->nodes[i + 1];
        pq->distances[i] = pq->distances[i + 1];
    }
    pq->size--;

    return minNode;
}
char *enlever_premier_dernier(const char *source)
{
    int longueur = strlen(source);

    // Vérifier si la chaîne est trop courte pour être traitée
    if (longueur <= 2)
    {
        return strdup(""); // Retourner une chaîne vide
    }

    // Allouer une nouvelle chaîne de longueur -2 (+1 pour '\0')
    char *nouvelle_chaine = (char *)malloc((longueur - 1) * sizeof(char));
    if (!nouvelle_chaine)
    {
        return NULL; // Retourner NULL en cas d'échec d'allocation
    }

    strncpy(nouvelle_chaine, source + 1, longueur - 2);
    nouvelle_chaine[longueur - 2] = '\0'; // Assurer la terminaison

    return nouvelle_chaine;
}

// Shortest path function that returns the path as a string
char *find_shortest_path(Graph *graph, Node *start, Node *end, int GRID_SIZE)
{
    int distances[MAX_POINTS];
    Node *previous[MAX_POINTS];

    // printf("Graph node count: %d\n", graph->node_count);

    // Initialize distances and previous nodes
    for (int i = 0; i < graph->node_count; i++)
    {
        distances[i] = INF;
        previous[i] = NULL;
    }

    // printf("Start Node: (%d, %d) with letter '%c'\n", start->x, start->y, start->letter);
    // printf("End Node: (%d, %d) with letter '%c'\n", end->x, end->y, end->letter);

    distances[start->x * GRID_SIZE + start->y] = 0;

    PriorityQueue pq = {.size = 0};
    push(&pq, start, 0);

    while (pq.size > 0)
    {
        Node *current = pop(&pq);
        if (current == end)
            break;

        int currentIndex = current->x * GRID_SIZE + current->y;
        // printf("Processing Node: (%d, %d) with letter '%c'\n", current->x, current->y, current->letter);
        // printf("Neighbors Count: %d\n", current->neighbor_count);

        for (int i = 0; i < current->neighbor_count; i++)
        {
            Node *neighbor = current->neighbors[i];

            // Skip walls
            if (neighbor->letter == '#')
            {
                // printf("Skipping wall at (%d, %d)\n", neighbor->x, neighbor->y);
                continue;
            }

            int neighborIndex = neighbor->x * GRID_SIZE + neighbor->y;
            int alt = distances[currentIndex] + 1;

            // printf("Evaluating Neighbor: (%d, %d) with letter '%c'\n", neighbor->x, neighbor->y, neighbor->letter);
            // printf("Current Distance: %d, New Distance: %d\n", distances[neighborIndex], alt);

            if (alt < distances[neighborIndex])
            {
                distances[neighborIndex] = alt;
                previous[neighborIndex] = current;
                push(&pq, neighbor, alt);
                // printf("Updating path: Previous[%d] = (%d, %d)\n", neighborIndex, current->x, current->y);
            }
        }
    }

    // If no path was found
    if (previous[end->x * GRID_SIZE + end->y] == NULL)
    {
        // printf("No path found between (%d, %d) and (%d, %d).\n", start->x, start->y, end->x, end->y);
        return NULL;
    }

    // Reconstruct the path and collect letters
    Node *path[MAX_POINTS];
    int path_length = 0;

    for (Node *at = end; at != NULL; at = previous[at->x * GRID_SIZE + at->y])
    {
        path[path_length++] = at;
        // printf("Adding to path: (%d, %d) with letter '%c'\n", at->x, at->y, at->letter);
    }

    // Create a string from collected letters
    char *word = malloc(path_length + 1);
    if (!word)
    {
        printf("Memory allocation failed.\n");
        return NULL;
    }

    printf("Final Path Collected Letters: ");
    for (int i = 0; i < path_length; i++)
    {
        word[i] = path[path_length - 1 - i]->letter; // Reverse order
        printf("%c ", word[i]);
    }
    word[path_length] = '\0';
    printf("\n");
    printf("Final Path: %s\n", word);
    return (word);
}

int get_distance(Node *a, Node *b)
{
    return abs(a->x - b->x) + abs(a->y - b->y);
}
// Find the optimal order to visit words (Greedy nearest neighbor)
// Find the optimal order to visit words (Greedy nearest neighbor)
void find_best_word_order(Graph *graph, WordPosition *word_positions, int word_count, Node **visit_order, int GRID_SIZE)
{
    int visited[word_count];
    memset(visited, 0, sizeof(visited));

    Node *current = graph->start;
    int order_index = 0;

    for (int i = 0; i < word_count; i++)
    {
        int best_index = -1;
        int min_distance = INT_MAX;

        for (int j = 0; j < word_count; j++)
        {
            if (!visited[j])
            {
                Node *word_start = graph->nodes[word_positions[j].startX * GRID_SIZE + word_positions[j].startY];
                int distance = get_distance(current, word_start);

                if (distance < min_distance)
                {
                    min_distance = distance;
                    best_index = j;
                }
            }
        }

        if (best_index != -1)
        {
            visited[best_index] = 1;

            // Visit word start position
            visit_order[order_index++] = graph->nodes[word_positions[best_index].startX * GRID_SIZE + word_positions[best_index].startY];

            // Visit word end position
            visit_order[order_index++] = graph->nodes[word_positions[best_index].endX * GRID_SIZE + word_positions[best_index].endY];

            // Update current position
            current = visit_order[order_index - 1];
        }
    }

    // Add the end node at the end of the visit order
    visit_order[order_index] = graph->end;
}

// Function to compute and print the full path (start → word start → word end → next word → end)
char *concatenate_paths(char **segments, int count)
{
    int total_length = 0;

    // Calculate total length needed
    for (int i = 0; i < count; i++)
    {
        if (segments[i])
        {
            total_length += strlen(segments[i]);
        }
    }

    // Allocate memory for final path
    char *final_path = malloc(total_length + 1);
    if (!final_path)
        return NULL;

    final_path[0] = '\0'; // Initialize as empty string

    // Concatenate all segments
    for (int i = 0; i < count; i++)
    {
        if (segments[i])
        {
            strcat(final_path, segments[i]);
        }
    }

    return final_path;
}

char *find_best_path(Graph *graph, WordPosition *word_positions, int word_count, int GRID_SIZE)
{
    Node **visit_order = malloc((word_count * 2 + 2) * sizeof(Node *));
    if (!visit_order)
    {
        printf("Memory allocation failed!\n");
        return NULL;
    }

    visit_order[0] = graph->start;

    // Compute the best order to visit words
    find_best_word_order(graph, word_positions, word_count, &visit_order[1], GRID_SIZE);

    // Array to store path segments
    char **path_segments = malloc((word_count * 2 + 1) * sizeof(char *));
    if (!path_segments)
    {
        printf("Memory allocation failed!\n");
        free(visit_order);
        return NULL;
    }

    printf("\nOptimal Path:\n");
    for (int i = 0; i < word_count * 2 + 1; i++)
    {
        path_segments[i] = find_shortest_path(graph, visit_order[i], visit_order[i + 1], GRID_SIZE);

        // Remove redundant start & end nodes from paths
        if (i > 0)
        {
            char *prev_path = path_segments[i - 1];
            char *curr_path = path_segments[i];

            // Ensure we don’t repeat the last character of prev_path and first of curr_path
            if (prev_path && curr_path)
            {
                int prev_len = strlen(prev_path);
                int curr_len = strlen(curr_path);

                // If last char of prev_path matches first char of curr_path, remove duplicate
                if (prev_len > 0 && curr_len > 0 && prev_path[prev_len - 1] == curr_path[0])
                {
                    memmove(curr_path, curr_path + 1, curr_len); // Shift left to remove duplicate
                }
            }
        }
    }

    // Concatenate all path segments into a single path
    char *final_path = concatenate_paths(path_segments, word_count * 2 + 1);
    if (final_path)
    {
        printf("%s\n", final_path);
    }

    // Free allocated memory
    for (int i = 0; i < word_count * 2 + 1; i++)
    {
        free(path_segments[i]);
    }
    free(path_segments);
    free(visit_order);
    return final_path;
}

void draw_graph(SDL_Renderer *renderer, Graph *graph, Player *player, TTF_Font *font, int GRID_SIZE)
{
    // Draw all the cells in the grid
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            Node *node = graph->nodes[i * GRID_SIZE + j];
            SDL_Rect cell = {j * CELL_SIZE, i * CELL_SIZE, CELL_SIZE, CELL_SIZE};

            // Draw walls (dark gray)
            if (node->letter == '#')
            {
                SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                SDL_RenderFillRect(renderer, &cell);
            }
            else
            {
                // Draw empty cells (light gray)
                SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
                SDL_RenderFillRect(renderer, &cell);

                // Draw the letters in the cells (if any)
                if (node->letter != ' ')
                {
                    char letter[2] = {node->letter, '\0'};
                    SDL_Color textColor = {0, 0, 0, 255}; // Black for letters
                    SDL_Surface *textSurface = TTF_RenderText_Blended(font, letter, textColor);
                    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                    int textW, textH;
                    SDL_QueryTexture(textTexture, NULL, NULL, &textW, &textH);
                    SDL_Rect textRect = {cell.x + (CELL_SIZE - textW) / 2, cell.y + (CELL_SIZE - textH) / 2, textW, textH};
                    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                    SDL_DestroyTexture(textTexture);
                    SDL_FreeSurface(textSurface);
                }
                // Highlight visited cells with orange transparency
                if (node->visited)
                {
                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 100); // Orange with transparency
                    SDL_RenderFillRect(renderer, &cell);
                }
            }

            // Draw grid lines (light gray)
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderDrawRect(renderer, &cell);
        }
    }

    // Set transparency for the player
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Draw the player's character inside the active cell last
    SDL_Rect playerRect = {player->y * CELL_SIZE, player->x * CELL_SIZE, CELL_SIZE, CELL_SIZE};

    // Draw the player's active cell border (blue)
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue color for the player's cell border
    SDL_RenderDrawRect(renderer, &playerRect);        // Draw the border

    // Draw the player's character (blue rectangle) with transparency
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 100); // Semi-transparent blue
    SDL_RenderFillRect(renderer, &playerRect);        // Fill the cell where the player is

    // Draw the start point (green with border)
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green color
    SDL_Rect startRect = {graph->start->y * CELL_SIZE, graph->start->x * CELL_SIZE, CELL_SIZE, CELL_SIZE};
    SDL_RenderFillRect(renderer, &startRect);         // Fill start point cell
    SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255); // Darker green for border
    SDL_RenderDrawRect(renderer, &startRect);

    // Draw the end point (red with border)
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red color
    SDL_Rect endRect = {graph->end->y * CELL_SIZE, graph->end->x * CELL_SIZE, CELL_SIZE, CELL_SIZE};
    SDL_RenderFillRect(renderer, &endRect);           // Fill end point cell
    SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255); // Darker red for border
    SDL_RenderDrawRect(renderer, &endRect);
}

int show_difficulty_selection(SDL_Renderer *renderer, TTF_Font *font, int WINDOW_SIZE)
{
    SDL_Event event;
    int running = 1;
    int selected = 0; // 0: Easy, 1: Medium, 2: Hard
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color blue = {0, 0, 255, 255}; // Blue for selected text

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                return 1;
            }
            else if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_UP)
                {
                    selected = (selected - 1 + 3) % 3; // Loop back to Hard if on Easy
                }
                else if (event.key.keysym.sym == SDLK_DOWN)
                {
                    selected = (selected + 1) % 3; // Loop back to Easy if on Hard
                }
                else if (event.key.keysym.sym == SDLK_RETURN)
                {
                    return selected; // Return the selected difficulty
                }
            }
        }

        // Set background to white
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
        SDL_RenderClear(renderer);

        // Render difficulty options
        SDL_Surface *easySurface = TTF_RenderText_Solid(font, "Easy", selected == 0 ? blue : white);
        SDL_Surface *mediumSurface = TTF_RenderText_Solid(font, "Medium", selected == 1 ? blue : white);
        SDL_Surface *hardSurface = TTF_RenderText_Solid(font, "Hard", selected == 2 ? blue : white);

        SDL_Texture *easyTexture = SDL_CreateTextureFromSurface(renderer, easySurface);
        SDL_Texture *mediumTexture = SDL_CreateTextureFromSurface(renderer, mediumSurface);
        SDL_Texture *hardTexture = SDL_CreateTextureFromSurface(renderer, hardSurface);

        // Render difficulty options, centered horizontally
        SDL_Rect easyRect = {50, WINDOW_SIZE / 2 - 30, 60, 40};   // Easy option
        SDL_Rect mediumRect = {50, WINDOW_SIZE / 2 + 20, 80, 40}; // Medium option
        SDL_Rect hardRect = {50, WINDOW_SIZE / 2 + 70, 60, 40};   // Hard option

        SDL_RenderCopy(renderer, easyTexture, NULL, &easyRect);
        SDL_RenderCopy(renderer, mediumTexture, NULL, &mediumRect);
        SDL_RenderCopy(renderer, hardTexture, NULL, &hardRect);

        SDL_FreeSurface(easySurface);
        SDL_FreeSurface(mediumSurface);
        SDL_FreeSurface(hardSurface);
        SDL_DestroyTexture(easyTexture);
        SDL_DestroyTexture(mediumTexture);
        SDL_DestroyTexture(hardTexture);

        SDL_RenderPresent(renderer);
    }
    return 1;
}

// calucl score
int calculate_score(char *path, WordPosition *word_positions, int word_count, int best_path_length)
{
    int score = 0;
    int all_words_found = 1; // Assume all words are found
    for (int i = 0; i < word_count; i++)
    {
        const char *word = word_positions[i].word;
        char *found = strstr(path, word);
        if (found)
        {
            score += strlen(word) * 3;
        }
        else
        {
            all_words_found = 0; // If any word is not found, set this to 0
        }
    }

    // If all words are found and the path length is the best path length, add 50 bonus points
    if (all_words_found && strlen(path) <= best_path_length)
    {
        score += 50;
    }

    return score;
}

int show_menu(SDL_Renderer *renderer, TTF_Font *font, int WINDOW_SIZE)
{
    SDL_Event event;
    int running = 1;
    int selected = 0; // 0: New Game, 1: Quit

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                return 1;
            }
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_DOWN)
                {
                    selected = !selected;
                }
                else if (event.key.keysym.sym == SDLK_RETURN)
                {
                    if (selected == 0)
                    {
                        // New game selected, show difficulty selection
                        int difficulty = show_difficulty_selection(renderer, font, WINDOW_SIZE);
                        return difficulty;
                    }
                    else
                    {
                        return -1; // Quit game
                    }
                }
            }
        }

        // Set background to white
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // White background
        SDL_RenderClear(renderer);

        SDL_Color white = {255, 255, 255, 255}; // Black text color for the menu
        SDL_Color blue = {0, 0, 255, 255};      // Blue for selected text

        // Render header
        SDL_Surface *headerSurface = TTF_RenderText_Solid(font, "Welcome to The Maze Game", white);
        SDL_Texture *headerTexture = SDL_CreateTextureFromSurface(renderer, headerSurface);
        SDL_Rect headerRect = {50, 50, 300, 40};
        SDL_RenderCopy(renderer, headerTexture, NULL, &headerRect);
        SDL_FreeSurface(headerSurface);
        SDL_DestroyTexture(headerTexture);

        TTF_SetFontStyle(font, TTF_STYLE_BOLD);
        // Render "New Game" and "Quit Game" with different colors based on selection
        SDL_Surface *newGameSurface = TTF_RenderText_Solid(font, "New Game", selected == 0 ? blue : white);
        SDL_Surface *quitSurface = TTF_RenderText_Solid(font, "Quit Game", selected == 1 ? blue : white);

        SDL_Texture *newGameTexture = SDL_CreateTextureFromSurface(renderer, newGameSurface);
        SDL_Texture *quitTexture = SDL_CreateTextureFromSurface(renderer, quitSurface);

        SDL_Rect newGameRect = {50, WINDOW_SIZE / 2 - 20, 100, 40};
        SDL_Rect quitRect = {50, WINDOW_SIZE / 2 + 20, 100, 40};

        SDL_RenderCopy(renderer, newGameTexture, NULL, &newGameRect);
        SDL_RenderCopy(renderer, quitTexture, NULL, &quitRect);

        SDL_FreeSurface(newGameSurface);
        SDL_FreeSurface(quitSurface);
        SDL_DestroyTexture(newGameTexture);
        SDL_DestroyTexture(quitTexture);

        // Render footer
        SDL_Surface *footerSurface = TTF_RenderText_Solid(font, "Made by Caption", white);
        SDL_Texture *footerTexture = SDL_CreateTextureFromSurface(renderer, footerSurface);
        SDL_Rect footerRect = {300, WINDOW_SIZE - 50, 200, 30};
        SDL_RenderCopy(renderer, footerTexture, NULL, &footerRect);
        SDL_FreeSurface(footerSurface);
        SDL_DestroyTexture(footerTexture);

        SDL_RenderPresent(renderer);
    }
    return 1;
}



int main(int argc, char *args[])
{
    srand(time(NULL));
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window *window = SDL_CreateWindow("Maze", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 800, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("Arial.ttf", 24);

    if (!font)
    {
        printf("Erreur : impossible de charger la police\n");
        return 1;
    }

    int difficulty = show_menu(renderer, font, 800);
    printf("Selected difficulty: %d\n", difficulty);

    if (difficulty == -1)
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

    // Set GRID_SIZE based on difficulty
    int GRID_SIZE;
    switch (difficulty)
    {
    case 0: // Easy
        GRID_SIZE = 10;
        break;
    case 1: // Medium
        GRID_SIZE = 15;
        break;
    case 2: // Hard
        GRID_SIZE = 18;
        break;
    }

    int WINDOW_SIZE = GRID_SIZE * CELL_SIZE;
    SDL_SetWindowSize(window, WINDOW_SIZE, WINDOW_SIZE);

    Graph *graph = create_graph(GRID_SIZE);
    initialize_graph(graph, GRID_SIZE);

    // Load words from file
    char words[1000][20];
    int word_count = load_words("dictionnaire.txt", words, 5);
    const char *word_ptrs[5];
    for (int i = 0; i < word_count; i++)
    {
        word_ptrs[i] = words[i];
    }
    WordPosition word_positions[5];
    int actual_word_count = 0;

    place_words(graph, word_ptrs, word_positions, &actual_word_count, word_count, GRID_SIZE);

    divide_graph(graph, 0, 0, GRID_SIZE - 1, GRID_SIZE - 1, GRID_SIZE);
    add_random_letters(graph, GRID_SIZE);
    set_start_end(graph);

    char *path = find_shortest_path(graph, graph->start, graph->end, GRID_SIZE);
    printf("Shortest MINIMAL path: %s\n", enlever_premier_dernier(path));

    char *final_best_path = find_best_path(graph, word_positions, 5, GRID_SIZE);
    printf("Final best path: %s\n", enlever_premier_dernier(final_best_path));

    Player player;
    initialize_player(&player, graph);

    Uint32 lastMoveTime = 0;
    Uint32 moveDelay = 150;
    int running = 1;
    SDL_Event event;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = 0;
            }
            else if (event.type == SDL_WINDOWEVENT)
            {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    int new_width = event.window.data1;
                    int new_height = event.window.data2;
                    SDL_SetWindowSize(window, new_width, new_height);
                }
            }
        }

        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastMoveTime > moveDelay)
        {
            const Uint8 *keystate = SDL_GetKeyboardState(NULL);

            if (keystate[SDL_SCANCODE_KP_8])
                move_player(&player, graph, -1, 0, GRID_SIZE);
            if (keystate[SDL_SCANCODE_KP_2])
                move_player(&player, graph, 1, 0, GRID_SIZE);
            if (keystate[SDL_SCANCODE_KP_4])
                move_player(&player, graph, 0, -1, GRID_SIZE);
            if (keystate[SDL_SCANCODE_KP_6])
                move_player(&player, graph, 0, 1, GRID_SIZE);
            if (keystate[SDL_SCANCODE_KP_7])
                move_player(&player, graph, -1, -1, GRID_SIZE);
            if (keystate[SDL_SCANCODE_KP_9])
                move_player(&player, graph, -1, 1, GRID_SIZE);
            if (keystate[SDL_SCANCODE_KP_1])
                move_player(&player, graph, 1, -1, GRID_SIZE);
            if (keystate[SDL_SCANCODE_KP_3])
                move_player(&player, graph, 1, 1, GRID_SIZE);

            lastMoveTime = currentTime;
        }

        // Check if player reached the end point
        if (player.x == graph->end->x && player.y == graph->end->y)
        {
            printf("Congratulations! You've reached the end point.\n");
            int score = calculate_score(player.path, word_positions, 5, strlen(final_best_path) - 2);
            printf("Score: %d\n", score);
            running = 0;
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        draw_graph(renderer, graph, &player, font, GRID_SIZE);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
