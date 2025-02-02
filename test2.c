#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define GRID_SIZE 15
#define CELL_SIZE 20
#define WINDOW_SIZE (GRID_SIZE * CELL_SIZE)

typedef struct Node {
    int x, y;
    struct Node **neighbors;
    int neighbor_count;
    char letter;
} Node;

typedef struct {
    Node **nodes;
    int node_count;
    Node *start;
    Node *end;
} Graph;

typedef struct {
    int x, y;
    int score;
} Player;

// Create a node
Node *create_node(int x, int y) {
    Node *node = (Node *)malloc(sizeof(Node));
    if (!node) {
        printf("Memory allocation error for node.\n");
        exit(1);
    }
    node->x = x;
    node->y = y;
    node->neighbor_count = 0;
    node->neighbors = (Node **)malloc(4 * sizeof(Node *));
    node->letter = ' ';  // Initialize as empty space

    if (!node->neighbors) {
        printf("Memory allocation error for neighbors.\n");
        free(node);
        exit(1);
    }
    return node;
}

// Create graph
Graph *create_graph() {
    Graph *graph = (Graph *)malloc(sizeof(Graph));
    graph->nodes = (Node **)malloc(GRID_SIZE * GRID_SIZE * sizeof(Node *));
    graph->node_count = 0;
    graph->start = NULL;
    graph->end = NULL;
    return graph;
}

// Add node to graph
void add_node(Graph *graph, Node *node) {
    graph->nodes[graph->node_count++] = node;
}

// Add an edge
void add_edge(Node *node1, Node *node2) {
    node1->neighbors = (Node **)realloc(node1->neighbors, (node1->neighbor_count + 1) * sizeof(Node *));
    node1->neighbors[node1->neighbor_count++] = node2;
    node2->neighbors = (Node **)realloc(node2->neighbors, (node2->neighbor_count + 1) * sizeof(Node *));
    node2->neighbors[node2->neighbor_count++] = node1;
}




// Initialize the graph
void initialize_graph(Graph *graph) {
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            Node *node = create_node(x, y);
            add_node(graph, node);
        }
    }

    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            Node *node = graph->nodes[x * GRID_SIZE + y];

            if (x > 0)
                add_edge(node, graph->nodes[(x - 1) * GRID_SIZE + y]);
            if (x < GRID_SIZE - 1)
                add_edge(node, graph->nodes[(x + 1) * GRID_SIZE + y]);
            if (y > 0)
                add_edge(node, graph->nodes[x * GRID_SIZE + (y - 1)]);
            if (y < GRID_SIZE - 1)
                add_edge(node, graph->nodes[x * GRID_SIZE + (y + 1)]);
        }
    }
}

// Set start and end points
void set_start_end(Graph *graph) {
    graph->start = graph->nodes[rand() % graph->node_count];
    graph->end = graph->nodes[rand() % graph->node_count];
}



// Initialize the player
void initialize_player(Player *player, Graph *graph) {
    player->x = graph->start->x;
    player->y = graph->start->y;
    player->score = 0;
}

// Move the player
void move_player(Player *player, Graph *graph, int dx, int dy) {
    int new_x = player->x + dx;
    int new_y = player->y + dy;

    for (int i = 0; i < graph->node_count; i++) {
        Node *node = graph->nodes[i];
        if (node->x == new_x && node->y == new_y && node->letter != '#') {
            player->x = new_x;
            player->y = new_y;
            if (node->letter != ' ') {
                player->score += 10;
                node->letter = ' ';
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

    if (x1 == x2) // Vertical wall
    {
        for (int y = y1; y <= y2; y++)
        {
            if (y != passage_y) // Leave a passage
            {
                Node *node = graph->nodes[x1 * GRID_SIZE + y];
                node->letter = '#'; // Mark as wall

                // Remove edges to disconnect from neighbors
                if (x1 > 0)
                    remove_edge(node, graph->nodes[(x1 - 1) * GRID_SIZE + y]); // Remove left connection
                if (x1 < GRID_SIZE - 1)
                    remove_edge(node, graph->nodes[(x1 + 1) * GRID_SIZE + y]); // Remove right connection
            }
        }
    }
    else if (y1 == y2) // Horizontal wall
    {
        for (int x = x1; x <= x2; x++)
        {
            if (x != passage_x) // Leave a passage
            {
                Node *node = graph->nodes[x * GRID_SIZE + y1];
                node->letter = '#'; // Mark as wall

                // Remove edges to disconnect from neighbors
                if (y1 > 0)
                    remove_edge(node, graph->nodes[x * GRID_SIZE + (y1 - 1)]); // Remove top connection
                if (y1 < GRID_SIZE - 1)
                    remove_edge(node, graph->nodes[x * GRID_SIZE + (y1 + 1)]); // Remove bottom connection
            }
        }
    }
}


// Recursive function to divide the graph into sections using walls
void divide_graph(Graph *graph, int startX, int startY, int endX, int endY)
{
    if (endX - startX < 3 || endY - startY < 3)
    {
        return; // Stop when sections are too small
    }

    if (rand() % 2 == 0)
    { // Vertical division
        int divideX = startX + rand() % (endX - startX - 1) + 1;
        add_wall(graph, divideX, startY, divideX, endY); // Add vertical wall
        divide_graph(graph, startX, startY, divideX - 1, endY); // Left section
        divide_graph(graph, divideX + 1, startY, endX, endY); // Right section
    }
    else
    { // Horizontal division
        int divideY = startY + rand() % (endY - startY - 1) + 1;
        add_wall(graph, startX, divideY, endX, divideY); // Add horizontal wall
        divide_graph(graph, startX, startY, endX, divideY - 1); // Top section
        divide_graph(graph, startX, divideY + 1, endX, endY); // Bottom section
    }
}


// Render the maze
void draw_graph(SDL_Renderer *renderer, Graph *graph, Player *player) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            Node *node = graph->nodes[i * GRID_SIZE + j];
            SDL_Rect cell = {j * CELL_SIZE, i * CELL_SIZE, CELL_SIZE, CELL_SIZE};

            if (node->letter == '#')
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            else
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

            SDL_RenderFillRect(renderer, &cell);
        }
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_Rect playerRect = {player->y * CELL_SIZE, player->x * CELL_SIZE, CELL_SIZE, CELL_SIZE};
    SDL_RenderFillRect(renderer, &playerRect);
}

int main( int argc, char* args[] ) {
    srand(time(NULL));
    Graph *graph = create_graph();
    initialize_graph(graph);
    set_start_end(graph);
    divide_graph(graph, 0, 0, GRID_SIZE - 1, GRID_SIZE - 1);


    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Maze", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_SIZE, WINDOW_SIZE, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    Player player;
    initialize_player(&player, graph);

    int running = 1;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event))
            if (event.type == SDL_QUIT)
                running = 0;

        SDL_RenderClear(renderer);
        draw_graph(renderer, graph, &player);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
