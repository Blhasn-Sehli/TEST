#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define GRID_SIZE 15       // Size of the grid (x by y dimensions)
#define CELL_SIZE 20       // Size of each cell in the GUI
#define WINDOW_SIZE (GRID_SIZE * CELL_SIZE) // Window size

typedef struct Node {
    int x, y;                 // Position of the node
    struct Node **neighbors;  // Array of adjacent nodes
    int neighbor_count;       // Number of neighbors (valid connections)
    char letter;              // Letter to place in the cell
} Node;

typedef struct {
    Node **nodes;     // Array of all nodes (vertices)
    int node_count;   // Number of nodes in the graph
    Node *start;      // Start node
    Node *end;        // End node
} Graph;

typedef struct {
    int x, y;         // Player's position
    int score;        // Player's score
} Player;

// Function to create a new node (graph vertex)
Node *create_node(int x, int y) {
    Node *node = (Node *)malloc(sizeof(Node));
    node->x = x;
    node->y = y;
    node->letter = ' ';   // Empty letter
    node->neighbors = NULL;
    node->neighbor_count = 0;
    return node;
}

// Function to create a graph
Graph *create_graph() {
    Graph *graph = (Graph *)malloc(sizeof(Graph));
    graph->nodes = (Node **)malloc(GRID_SIZE * GRID_SIZE * sizeof(Node *));
    graph->node_count = 0;
    graph->start = NULL;
    graph->end = NULL;
    return graph;
}

// Function to add a node to the graph
void add_node(Graph *graph, Node *node) {
    graph->nodes[graph->node_count++] = node;
}

// Function to add an edge between two nodes
void add_edge(Node *node1, Node *node2) {
    node1->neighbors = (Node **)realloc(node1->neighbors, (node1->neighbor_count + 1) * sizeof(Node *));
    node1->neighbors[node1->neighbor_count++] = node2;
    node2->neighbors = (Node **)realloc(node2->neighbors, (node2->neighbor_count + 1) * sizeof(Node *));
    node2->neighbors[node2->neighbor_count++] = node1;
}

// Function to initialize the graph with nodes
void initialize_graph(Graph *graph) {
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            Node *node = create_node(x, y);
            add_node(graph, node);

            // Connect the node to its adjacent neighbors (up, down, left, right)
            if (x > 0) add_edge(node, graph->nodes[(x - 1) * GRID_SIZE + y]);  // Up
            if (x < GRID_SIZE - 1) add_edge(node, graph->nodes[(x + 1) * GRID_SIZE + y]);  // Down
            if (y > 0) add_edge(node, graph->nodes[x * GRID_SIZE + (y - 1)]);  // Left
            if (y < GRID_SIZE - 1) add_edge(node, graph->nodes[x * GRID_SIZE + (y + 1)]);  // Right
        }
    }
}

// Function to randomly place a start and end point in the graph
void set_start_end(Graph *graph) {
    graph->start = graph->nodes[rand() % graph->node_count];
    graph->end = graph->nodes[rand() % graph->node_count];
}

// Function to place random letters in the graph
void place_random_letters(Graph *graph) {
    for (int i = 0; i < graph->node_count; i++) {
        if (graph->nodes[i] != graph->start && graph->nodes[i] != graph->end) {
            graph->nodes[i]->letter = 'a' + rand() % 26;  // Random letter
        }
    }
}

// Function to divide the graph into sections, adding walls
// Function to divide the graph by removing edges between sections
void divide_graph(Graph *graph, int startX, int startY, int endX, int endY) {
    // Base case: if the section is too small to divide, return
    if (endX - startX <= 2 || endY - startY <= 2) {
        return;
    }

    // Randomly decide whether to divide vertically or horizontally
    if (rand() % 2 == 0) {
        // Vertical division
        int divideX = startX + rand() % (endX - startX - 1) + 1;

        // Remove edges to divide the maze
        for (int y = startY; y < endY; y++) {
            Node *node1 = graph->nodes[divideX * GRID_SIZE + y];
            Node *node2 = graph->nodes[(divideX + 1) * GRID_SIZE + y];

            // Remove the edge between the two adjacent nodes
            for (int i = 0; i < node1->neighbor_count; i++) {
                if (node1->neighbors[i] == node2) {
                    node1->neighbors[i] = node1->neighbors[node1->neighbor_count - 1];
                    node1->neighbor_count--;
                    break;
                }
            }

            for (int i = 0; i < node2->neighbor_count; i++) {
                if (node2->neighbors[i] == node1) {
                    node2->neighbors[i] = node2->neighbors[node2->neighbor_count - 1];
                    node2->neighbor_count--;
                    break;
                }
            }
        }

        // Recursively divide the sections on either side of the vertical division
        divide_graph(graph, startX, startY, divideX, endY);
        divide_graph(graph, divideX, startY, endX, endY);

    } else {
        // Horizontal division
        int divideY = startY + rand() % (endY - startY - 1) + 1;

        // Remove edges to divide the maze
        for (int x = startX; x < endX; x++) {
            Node *node1 = graph->nodes[x * GRID_SIZE + divideY];
            Node *node2 = graph->nodes[x * GRID_SIZE + divideY + 1];

            // Remove the edge between the two adjacent nodes
            for (int i = 0; i < node1->neighbor_count; i++) {
                if (node1->neighbors[i] == node2) {
                    node1->neighbors[i] = node1->neighbors[node1->neighbor_count - 1];
                    node1->neighbor_count--;
                    break;
                }
            }

            for (int i = 0; i < node2->neighbor_count; i++) {
                if (node2->neighbors[i] == node1) {
                    node2->neighbors[i] = node2->neighbors[node2->neighbor_count - 1];
                    node2->neighbor_count--;
                    break;
                }
            }
        }

        // Recursively divide the sections above and below the horizontal division
        divide_graph(graph, startX, startY, endX, divideY);
        divide_graph(graph, startX, divideY, endX, endY);
    }
}


// Function to place words in the graph
void place_words(Graph *graph, const char *words[], int word_count) {
    for (int i = 0; i < word_count; i++) {
        // Try to place the word, if it fits, you can connect nodes in sequence for the word
        printf("Placing word: %s\n", words[i]);
        // For each word, find a starting node and connect it along the word's path
    }
}

// Initialize the player
void initialize_player(Player *player, Graph *graph) {
    player->x = graph->start->x;
    player->y = graph->start->y;
    player->score = 0;
}

// Function to move the player in the graph
void move_player(Player *player, Graph *graph, int dx, int dy) {
    int new_x = player->x + dx;
    int new_y = player->y + dy;
    
    // Find the corresponding node in the graph for the new position
    for (int i = 0; i < graph->node_count; i++) {
        Node *node = graph->nodes[i];
        if (node->x == new_x && node->y == new_y) {
            player->x = new_x;
            player->y = new_y;
            // If there's a letter, increment score
            if (node->letter != ' ') {
                player->score += 10;
                node->letter = ' ';  // Clear the letter after collecting
            }
            break;
        }
    }
}

void draw_graph(SDL_Renderer *renderer, TTF_Font *font, Graph *graph) {
    for (int i = 0; i < graph->node_count; i++) {
        Node *node = graph->nodes[i];
        SDL_Rect cell = {node->y * CELL_SIZE, node->x * CELL_SIZE, CELL_SIZE, CELL_SIZE};
        
        // Draw background (empty or wall)
        if (node->letter == ' ') {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);  // White for empty cells
            SDL_RenderFillRect(renderer, &cell);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Black for walls
            SDL_RenderFillRect(renderer, &cell);
        }

        // Draw letter
        if (node->letter != ' ') {
            char letter[2] = {node->letter, '\0'};
            SDL_Color textColor = {0, 0, 0};  // Black
            SDL_Surface *textSurface = TTF_RenderText_Solid(font, letter, textColor);
            SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            int textW, textH;
            SDL_QueryTexture(textTexture, NULL, NULL, &textW, &textH);
            SDL_Rect textRect = {cell.x + (CELL_SIZE - textW) / 2, cell.y + (CELL_SIZE - textH) / 2, textW, textH};
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            SDL_DestroyTexture(textTexture);
            SDL_FreeSurface(textSurface);
        }
    }

    // Draw start and end points
    SDL_Rect startRect = {graph->start->y * CELL_SIZE, graph->start->x * CELL_SIZE, CELL_SIZE, CELL_SIZE};
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);  // Green for start
    SDL_RenderFillRect(renderer, &startRect);

    SDL_Rect endRect = {graph->end->y * CELL_SIZE, graph->end->x * CELL_SIZE, CELL_SIZE, CELL_SIZE};
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);  // Red for end
    SDL_RenderFillRect(renderer, &endRect);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

    // Initialize the graph
    Graph *graph = create_graph();
    initialize_graph(graph);
    set_start_end(graph);
    //afficher le graphe pour voir les points de départ et d'arrivée
    for (int i = 0; i < graph->node_count; i++) {
        Node *node = graph->nodes[i];
        printf("Node: (%d, %d)\n", node->x, node->y);
    }
    divide_graph(graph, 0, 0, GRID_SIZE, GRID_SIZE);
    //afficher aprés la division
    for (int i = 0; i < graph->node_count; i++) {
        Node *node = graph->nodes[i];
        printf("Node: (%d, %d)\n", node->x, node->y);
    } 

    place_random_letters(graph);

    // Place words in the graph
    const char *words[] = {"HELLO", "WORLD"};
    place_words(graph, words, 2);

    // Initialize SDL and the player
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_Window *window = SDL_CreateWindow("Graph Maze with Words", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_SIZE, WINDOW_SIZE, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("arial.ttf", 20);

    Player player;
    initialize_player(&player, graph);

    int running = 1;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }

        // Player movement logic (based on keyboard input)
        const Uint8 *keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_UP]) move_player(&player, graph, -1, 0);  // Move up
        if (keystate[SDL_SCANCODE_DOWN]) move_player(&player, graph, 1, 0);  // Move down
        if (keystate[SDL_SCANCODE_LEFT]) move_player(&player, graph, 0, -1); // Move left
        if (keystate[SDL_SCANCODE_RIGHT]) move_player(&player, graph, 0, 1); // Move right

        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Draw the maze (graph) and player
        draw_graph(renderer, font, graph);

        // Display the screen
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
