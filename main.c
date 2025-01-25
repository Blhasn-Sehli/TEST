#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WIDTH 10
#define HEIGHT 10
#define CELL_SIZE 50
#define MAX_WORD_LENGTH 50
#define MAX_WORDS 5

typedef struct Node
{
    char letter;
    int x, y;    // Position dans la grille
    int visited; // Si le nœud a été visité
} Node;

typedef struct
{
    Node *nodes[HEIGHT][WIDTH];        // Grille représentée comme un graphe
    int playerX, playerY;              // Position du joueur
    int endX, endY;                    // Position de la sortie
    int score;                         // Score du joueur
    char currentWord[MAX_WORD_LENGTH]; // Mot formé par le joueur
    int wordIndex;                     // Index du mot actuel
} Graph;

// Charger le dictionnaire
int loadDictionary(char dictionary[MAX_WORDS][MAX_WORD_LENGTH], const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        printf("Erreur : Impossible de charger le dictionnaire.\n");
        exit(1);
    }

    int count = 0;
    while (fscanf(file, "%s", dictionary[count]) != EOF)
    {
        count++;
        if (count >= MAX_WORDS)
            break;
    }

    fclose(file);
    return count;
}
//afficher le dictionnaire 

// Générer la grille à partir du dictionnaire
void generateGraph(Graph *graph, char dictionary[MAX_WORDS][MAX_WORD_LENGTH], int wordCount)
{
    srand(time(NULL));

    // Initialiser la grille avec des nœuds vides
    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            Node *node = (Node *)malloc(sizeof(Node));
            if (!node)
            {
                printf("Erreur : Impossible d'allouer de la mémoire pour le nœud.\n");
                exit(1);
            }
            node->letter = ' ';
            node->x = i;
            node->y = j;
            node->visited = 0;
            graph->nodes[i][j] = node;
        }
    }

    // Placer des mots aléatoirement dans la grille
    for (int w = 0; w < wordCount && w < HEIGHT; w++)
    {
        int startX = rand() % HEIGHT;
        int startY = rand() % WIDTH;
        int direction = rand() % 3; // 0: horizontal, 1: vertical, 2: diagonal
        char *word = dictionary[w];

        for (int k = 0; k < strlen(word); k++)
        {
            if (startX >= 0 && startX < HEIGHT && startY >= 0 && startY < WIDTH)
            {
                graph->nodes[startX][startY]->letter = word[k];

                // Avancer dans la direction choisie
                if (direction == 0)
                    startY++; // Horizontal
                else if (direction == 1)
                    startX++; // Vertical
                else
                {
                    startX++;
                    startY++;
                } // Diagonal
            }
        }
    }

    // Initialiser la position du joueur et de la sortie
    graph->playerX = 0;
    graph->playerY = 0;
    graph->endX = HEIGHT - 1;
    graph->endY = WIDTH - 1;
    graph->score = 0;
    graph->wordIndex = 0;
    memset(graph->currentWord, 0, sizeof(graph->currentWord));
}

// Charger une texture de lettre
SDL_Texture *loadLetterTexture(SDL_Renderer *renderer, char letter)
{
    char filepath[100];
    sprintf(filepath, "assets/%c.bmp", letter);

    SDL_Surface *surface = SDL_LoadBMP(filepath);
    if (!surface)
    {
        printf("Erreur : Impossible de charger %s\n", filepath);
        return NULL; // Retourner NULL si le fichier est manquant
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

// Dessiner la grille avec SDL2
void renderGraph(SDL_Renderer *renderer, Graph *graph, SDL_Texture *textures[26])
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Fond noir
    SDL_RenderClear(renderer);

    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            SDL_Rect cell = {j * CELL_SIZE, i * CELL_SIZE + 50, CELL_SIZE, CELL_SIZE};
            Node *node = graph->nodes[i][j];

            if (i == graph->playerX && j == graph->playerY)
            {
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Vert pour le joueur
            }
            else if (i == graph->endX && j == graph->endY)
            {
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Rouge pour la sortie
            }
            else
            {
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Gris clair pour les lettres
            }

            SDL_RenderFillRect(renderer, &cell);

            // Afficher la lettre dans la case
            if (node->letter != ' ')
            {
                int index = node->letter - 'a';
                if (index >= 0 && index < 26)
                {
                    SDL_Texture *texture = textures[index];
                    if (texture)
                    { // Vérifier si la texture est valide
                        SDL_Rect dst = {j * CELL_SIZE, i * CELL_SIZE + 50, CELL_SIZE, CELL_SIZE};
                        SDL_RenderCopy(renderer, texture, NULL, &dst);
                    }
                    else
                    {
                        printf("Texture invalide pour la lettre %c\n", node->letter);
                    }
                }
                else
                {
                    // printf("Index de texture invalide pour la lettreeeee %c\n", node->letter);
                }
            }
        }
    }

    // Afficher le mot formé
    // Placeholder : Ajoutez un système pour afficher le mot formé

    SDL_RenderPresent(renderer);
}

// Déplacer le joueur
void movePlayer(Graph *graph, int dx, int dy)
{
    int newX = graph->playerX + dx;
    int newY = graph->playerY + dy;

    if (newX >= 0 && newX < HEIGHT && newY >= 0 && newY < WIDTH)
    {
        graph->playerX = newX;
        graph->playerY = newY;

        // Ajouter la lettre au mot formé
        Node *currentNode = graph->nodes[newX][newY];
        if (currentNode->letter != ' ' && !currentNode->visited)
        {
            graph->currentWord[graph->wordIndex++] = currentNode->letter;
            currentNode->visited = 1;
        }
    }
}

// Vérifier la victoire
int checkWin(Graph *graph)
{
    return graph->playerX == graph->endX && graph->playerY == graph->endY;
}

int main(int argc, char *argv[])
{
    Graph graph;
    char dictionary[MAX_WORDS][MAX_WORD_LENGTH];
    int wordCount = loadDictionary(dictionary, "dictionnaire.txt");

    generateGraph(&graph, dictionary, wordCount);

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("Erreur : Impossible d'initialiser SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Word Maze", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH * CELL_SIZE, HEIGHT * CELL_SIZE + 50, 0);
    if (!window)
    {
        printf("Erreur : Impossible de créer la fenêtre: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        printf("Erreur : Impossible de créer le renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Charger les textures des lettres
    SDL_Texture *textures[26];
    for (char letter = 'A'; letter <= 'Z'; letter++)
    {
        textures[letter - 'A'] = loadLetterTexture(renderer, letter);
        if (!textures[letter - 'A'])
        {
            printf("Erreur : Texture pour la lettre %c non chargée.\n", letter);
        }
    }

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

        const Uint8 *state = SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_UP])
            movePlayer(&graph, -1, 0);
        if (state[SDL_SCANCODE_DOWN])
            movePlayer(&graph, 1, 0);
        if (state[SDL_SCANCODE_LEFT])
            movePlayer(&graph, 0, -1);
        if (state[SDL_SCANCODE_RIGHT])
            movePlayer(&graph, 0, 1);

        if (checkWin(&graph))
        {
            printf("Félicitations, vous avez gagné avec un score de %d!\n", graph.score);
            running = 0;
        }

        renderGraph(renderer, &graph, textures);
        SDL_Delay(100);
    }

    // Libérer les textures
    for (int i = 0; i < 26; i++)
    {
        if (textures[i])
        {
            SDL_DestroyTexture(textures[i]);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}