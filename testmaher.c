#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <limits.h>

#define INF INT_MAX

#define GRID_SIZE 15
#define CELL_SIZE 40
#define WINDOW_SIZE (GRID_SIZE * CELL_SIZE)
#define MAX_POINTS (GRID_SIZE * GRID_SIZE) // 1 pour début, 1 pour fin, 100 pour les mots (par exemple) exactement dans le calcul de la distance !!!!

typedef struct Node
{
    int x, y;
    struct Node **neighbors;
    int neighbor_count;
    char letter;
    bool visited;
} Node;

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

    if (!node->neighbors)
    {
        printf("Memory allocation error for neighbors.\n");
        free(node);
        exit(1);
    }
    return node;
}

// Create graph
Graph *create_graph()
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
        printf("Skipping self-loop at (%d, %d)\n", node1->x, node1->y);
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
        printf("Edge between (%d, %d) and (%d, %d) already exists. Skipping.\n",
               node1->x, node1->y, node2->x, node2->y);
        return;
    }

    // Add node2 to node1's neighbors
    node1->neighbors = (Node **)realloc(node1->neighbors, (node1->neighbor_count + 1) * sizeof(Node *));
    node1->neighbors[node1->neighbor_count++] = node2;

    // Add node1 to node2's neighbors
    node2->neighbors = (Node **)realloc(node2->neighbors, (node2->neighbor_count + 1) * sizeof(Node *));
    node2->neighbors[node2->neighbor_count++] = node1;

    printf("Added edge between (%d, %d) and (%d, %d)\n", node1->x, node1->y, node2->x, node2->y);
}

void print_neighbors(Graph *graph)
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
void initialize_graph(Graph *graph)
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
    do
    {
        graph->start = graph->nodes[rand() % graph->node_count];
    } while (graph->start->letter == '#' || graph->start->letter == ' '); // Ensure it's not a wall or empty space

    do
    {
        graph->end = graph->nodes[rand() % graph->node_count];
    } while (graph->end->letter == '#' || graph->end->letter == ' ' || graph->end == graph->start || abs(graph->end->x - graph->start->x) + abs(graph->end->y - graph->start->y) < 5); // Ensure it's not a wall, not an empty space, and different from start
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

int can_place_word(Graph *graph, const char *word, int x, int y, int horizontal)
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

int try_place_word(Graph *graph, const char *word, int *word_count)
{
    int len = strlen(word);
    int attempts = 100;

    while (attempts-- > 0)
    {
        int horizontal = rand() % 2;
        int x = rand() % GRID_SIZE;
        int y = rand() % GRID_SIZE;

        if (can_place_word(graph, word, x, y, horizontal))
        {
            // Place the word on the grid
            for (int i = 0; i < len; i++)
            {
                // Node *node = graph->nodes[(horizontal ? x : x + i) * GRID_SIZE + (horizontal ? y + i : y)];
                Node *node = graph->nodes[(x + (horizontal ? 0 : i)) * GRID_SIZE + (y + (horizontal ? i : 0))];

                node->letter = word[i];
            }

            return 1;
        }
    }
    return 0;
}

void place_words(Graph *graph, const char *words[], int *word_count, int word_count_total)
{
    for (int i = 0; i < word_count_total; i++)
    {
        if (!try_place_word(graph, words[i], word_count))
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
void move_player(Player *player, Graph *graph, int dx, int dy)
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
                // node->letter = ' ';   // Remove the letter after collection
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
void add_wall(Graph *graph, int x1, int y1, int x2, int y2)
{
    int passage_x = x1 + rand() % (x2 - x1 + 1);
    int passage_y = y1 + rand() % (y2 - y1 + 1);

    if (x1 == x2)
    { // Vertical wall
        for (int y = y1; y <= y2; y++)
        {
            if (y != passage_y)
            { // Leave a passage
                Node *node = graph->nodes[x1 * GRID_SIZE + y];
                if (node->letter == ' ')
                { // Only mark as a wall if it's empty
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
void add_random_letters(Graph *graph)
{
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            Node *node = graph->nodes[i * GRID_SIZE + j];
            if (node->letter == ' ')
            {
                node->letter = 'a' + rand() % 26;
            }
        }
    }
}

// Recursive function to divide the graph into sections using walls
void divide_graph(Graph *graph, int startX, int startY, int endX, int endY)
{
    if (endX - startX < 2 || endY - startY < 2)
    {
        return; // Stop when sections are too small
    }

    if (rand() % 2 == 0)
    { // Vertical division
        int divideX = startX + rand() % (endX - startX - 1) + 1;
        add_wall(graph, divideX, startY, divideX, endY);        // Add vertical wall
        divide_graph(graph, startX, startY, divideX - 1, endY); // Left section
        divide_graph(graph, divideX + 1, startY, endX, endY);   // Right section
    }
    else
    { // Horizontal division
        int divideY = startY + rand() % (endY - startY - 1) + 1;
        add_wall(graph, startX, divideY, endX, divideY);        // Add horizontal wall
        divide_graph(graph, startX, startY, endX, divideY - 1); // Top section
        divide_graph(graph, startX, divideY + 1, endX, endY);   // Bottom section
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
char *find_shortest_path(Graph *graph, Node *start, Node *end)
{
    int distances[MAX_POINTS];
    Node *previous[MAX_POINTS];

    printf("Graph node count: %d\n", graph->node_count);

    // Initialize distances and previous nodes
    for (int i = 0; i < graph->node_count; i++)
    {
        distances[i] = INF;
        previous[i] = NULL;
    }

    printf("Start Node: (%d, %d) with letter '%c'\n", start->x, start->y, start->letter);
    printf("End Node: (%d, %d) with letter '%c'\n", end->x, end->y, end->letter);

    distances[start->x * GRID_SIZE + start->y] = 0;

    PriorityQueue pq = {.size = 0};
    push(&pq, start, 0);

    while (pq.size > 0)
    {
        Node *current = pop(&pq);
        if (current == end)
            break;

        int currentIndex = current->x * GRID_SIZE + current->y;
        printf("Processing Node: (%d, %d) with letter '%c'\n", current->x, current->y, current->letter);
        printf("Neighbors Count: %d\n", current->neighbor_count);

        for (int i = 0; i < current->neighbor_count; i++)
        {
            Node *neighbor = current->neighbors[i];

            // Skip walls
            if (neighbor->letter == '#')
            {
                printf("Skipping wall at (%d, %d)\n", neighbor->x, neighbor->y);
                continue;
            }

            int neighborIndex = neighbor->x * GRID_SIZE + neighbor->y;
            int alt = distances[currentIndex] + 1;

            printf("Evaluating Neighbor: (%d, %d) with letter '%c'\n", neighbor->x, neighbor->y, neighbor->letter);
            printf("Current Distance: %d, New Distance: %d\n", distances[neighborIndex], alt);

            if (alt < distances[neighborIndex])
            {
                distances[neighborIndex] = alt;
                previous[neighborIndex] = current;
                push(&pq, neighbor, alt);
                printf("Updating path: Previous[%d] = (%d, %d)\n", neighborIndex, current->x, current->y);
            }
        }
    }

    // If no path was found
    if (previous[end->x * GRID_SIZE + end->y] == NULL)
    {
        printf("No path found between (%d, %d) and (%d, %d).\n", start->x, start->y, end->x, end->y);
        return NULL;
    }

    // Reconstruct the path and collect letters
    Node *path[MAX_POINTS];
    int path_length = 0;

    for (Node *at = end; at != NULL; at = previous[at->x * GRID_SIZE + at->y])
    {
        path[path_length++] = at;
        printf("Adding to path: (%d, %d) with letter '%c'\n", at->x, at->y, at->letter);
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
    return enlever_premier_dernier(word);
}

// Draw the graph
void draw_graph(SDL_Renderer *renderer, Graph *graph, Player *player, TTF_Font *font)
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

int main(int argc, char *args[])
{
    srand(time(NULL));
    Graph *graph = create_graph();
    initialize_graph(graph);
    printf("JUST AFTER INITIALIZING GRAPH\n");
    print_neighbors(graph);

    // Load words from file
    char words[1000][20];
    int word_count = load_words("dictionnaire.txt", words, 5);
    const char *word_ptrs[5];
    for (int i = 0; i < word_count; i++)
    {
        word_ptrs[i] = words[i];
    }

    place_words(graph, word_ptrs, &word_count, 5);

    set_start_end(graph);
    printf("JUST AFTER SETTING START AND END\n");
    print_neighbors(graph);

    divide_graph(graph, 0, 0, GRID_SIZE - 1, GRID_SIZE - 1);

    add_random_letters(graph);
    printf("JUST AFTER ADDING RANDOM LETTERS\n");
    print_neighbors(graph);

    // shortest path
    // afficher start et end neighbors
    // printf("Start neighbors\n");
    // for (int i = 0; i < graph->start->neighbor_count; i++)
    // {
    //     printf("Start neighbors %d %d\n", graph->start->neighbors[i]->x, graph->start->neighbors[i]->y);
    // }
    // printf("End neighbors\n");
    // for (int i = 0; i < graph->end->neighbor_count; i++)
    // {
    //     printf("End neighbors %d %d\n", graph->end->neighbors[i]->x, graph->end->neighbors[i]->y);
    // }
    char *path = find_shortest_path(graph, graph->start, graph->end);
    printf("Shortest path: %s\n", path);

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_Window *window = SDL_CreateWindow("Maze", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_SIZE, WINDOW_SIZE, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("arial.ttf", 24);

    if (!font)
    {
        printf("Erreur : impossible de charger la police\n");
        return 1;
    }

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
        }

        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastMoveTime > moveDelay)
        {
            const Uint8 *keystate = SDL_GetKeyboardState(NULL);

            if (keystate[SDL_SCANCODE_KP_8])
            { // Déplacer vers le haut
                move_player(&player, graph, -1, 0);
            }
            if (keystate[SDL_SCANCODE_KP_2])
            { // Déplacer vers le bas
                move_player(&player, graph, 1, 0);
            }
            if (keystate[SDL_SCANCODE_KP_4])
            { // Déplacer vers la gauche
                move_player(&player, graph, 0, -1);
            }
            if (keystate[SDL_SCANCODE_KP_6])
            { // Déplacer vers la droite
                move_player(&player, graph, 0, 1);
            }
            if (keystate[SDL_SCANCODE_KP_7])
            { // Déplacer en diagonale haut-gauche
                move_player(&player, graph, -1, -1);
            }
            if (keystate[SDL_SCANCODE_KP_9])
            { // Déplacer en diagonale haut-droite
                move_player(&player, graph, -1, 1);
            }
            if (keystate[SDL_SCANCODE_KP_1])
            { // Déplacer en diagonale bas-gauche
                move_player(&player, graph, 1, -1);
            }
            if (keystate[SDL_SCANCODE_KP_3])
            { // Déplacer en diagonale bas-droite
                move_player(&player, graph, 1, 1);
            }

            lastMoveTime = currentTime;
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        draw_graph(renderer, graph, &player, font);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}