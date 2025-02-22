#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define EMPTY ' '
#define GRID_SIZE 15      // Taille de la grille
#define CELL_SIZE 20      // Taille d'une cellule dans l'interface graphique
#define WINDOW_SIZE (GRID_SIZE * CELL_SIZE) // Taille de la fenêtre

typedef struct {
    char grid[GRID_SIZE][GRID_SIZE]; // Grille contenant les murs et lettres
    int startX, startY;              // Position de départ
    int endX, endY;                  // Position d'arrivée
} Maze;

typedef struct {
    int x, y;         // Player's position
    int score;        // Player's score
} Player;


// Initialise le labyrinthe avec des espaces vides
void initialize_maze(Maze *maze) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            maze->grid[i][j] = ' '; // Espaces vides
        }
    }

    // Placer les points de départ et d'arrivée
    do {
        maze->startX = rand() % GRID_SIZE;
        maze->startY = rand() % GRID_SIZE;
    } while (maze->grid[maze->startX][maze->startY] == '#'); // Vérifier qu'il n'y a pas de mur

    do {
        maze->endX = rand() % GRID_SIZE;
        maze->endY = rand() % GRID_SIZE;
    } while (maze->grid[maze->endX][maze->endY] == '#' || (maze->endX == maze->startX && maze->endY == maze->startY)); // Vérifier qu'il n'y a pas de mur et qu'il ne se trouve pas sur le point de départ
}


// // Place une lettre aléatoire dans les cellules vides
void add_random_letters(Maze *maze) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (maze->grid[i][j] == ' ') {
                maze->grid[i][j] = 'a' + rand() % 26; // Lettres aléatoires
            }
        }
    }
}




// Charge un dictionnaire de mots depuis un fichier
int load_words(const char *filename, char words[][20], int max_words) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Erreur : impossible d'ouvrir le fichier %s\n", filename);
        return 0;
    }

    int count = 0;
    while (fscanf(file, "%19s", words[count]) == 1 && count < max_words) {
        count++;
    }

    fclose(file);
    return count;
}





// Vérifie si on peut placer un mot dans la position donnée
int can_place_word(Maze *maze, const char *word, int x, int y, int horizontal) {
    int len = strlen(word);

    if (horizontal) {
        if (y + len > GRID_SIZE) return 0; // Dépasse la bordure
        for (int i = 0; i < len; i++) {
            if (maze->grid[x][y + i] != EMPTY) return 0; // Collision
        }
    } else {
        if (x + len > GRID_SIZE) return 0; // Dépasse la bordure
        for (int i = 0; i < len; i++) {
            if (maze->grid[x + i][y] != EMPTY) return 0; // Collision
        }
    }

    return 1; // Placement possible
}

// Place un mot en s'assurant qu'il est entier et bien aligné
int try_place_word(Maze *maze, const char *word) {
    int len = strlen(word);
    int attempts = 100;

    while (attempts-- > 0) {
        int horizontal = rand() % 2;
        int x = rand() % GRID_SIZE;
        int y = rand() % GRID_SIZE;

        if (can_place_word(maze, word, x, y, horizontal)) {
            for (int i = 0; i < len; i++) {
                if (horizontal) {
                    maze->grid[x][y + i] = word[i];
                } else {
                    maze->grid[x + i][y] = word[i];
                }
            }
            return 1;
        }
    }
    return 0;
}

// Place plusieurs mots dans le labyrinthe
void place_words(Maze *maze, const char *words[], int word_count) {
    for (int i = 0; i < word_count; i++) {
        if (!try_place_word(maze, words[i])) {
            printf("⚠️ Impossible de placer le mot: %s\n", words[i]);
        }
    }
}

void add_wall(Maze *maze, int x1, int y1, int x2, int y2, int passage_x, int passage_y) {
    if (x1 == x2) {
        for (int y = y1; y <= y2; y++) {
            if (y != passage_y && maze->grid[x1][y] == EMPTY) {
                maze->grid[x1][y] = '#';
            }
        }
    } else if (y1 == y2) {
        for (int x = x1; x <= x2; x++) {
            if (x != passage_x && maze->grid[x][y1] == EMPTY) {
                maze->grid[x][y1] = '#';
            }
        }
    }
}

void divide(Maze *maze, int x1, int y1, int x2, int y2) {
    int width = x2 - x1;
    int height = y2 - y1;

    if (width < 2 || height < 2) return;

    int horizontal = rand() % 2;
    if (width > height) horizontal = 0;
    if (height > width) horizontal = 1;

    if (horizontal) {
        int y_cut = y1 + 1 + rand() % (height - 1);
        int passage_x = x1 + rand() % width;
        add_wall(maze, x1, y_cut, x2, y_cut, passage_x, y_cut);

        divide(maze, x1, y1, x2, y_cut - 1);
        divide(maze, x1, y_cut + 1, x2, y2);
    } else {
        int x_cut = x1 + 1 + rand() % (width - 1);
        int passage_y = y1 + rand() % height;
        add_wall(maze, x_cut, y1, x_cut, y2, x_cut, passage_y);

        divide(maze, x1, y1, x_cut - 1, y2);
        divide(maze, x_cut + 1, y1, x2, y2);
    }
}







// Dessine le labyrinthe avec lettres et murs
// Dessiner le labyrinthe avec lettres, murs, point de départ et point d'arrivée
void draw_maze(SDL_Renderer *renderer, TTF_Font *font, Maze *maze) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            SDL_Rect cell = {j * CELL_SIZE, i * CELL_SIZE, CELL_SIZE, CELL_SIZE};

            if (maze->grid[i][j] == '#') {
                // Dessiner un mur taille de la cellule de mur 
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Mur noir
                SDL_RenderFillRect(renderer, &cell);
            } else {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Fond blanc
                SDL_RenderFillRect(renderer, &cell);

                // Dessiner la lettre
                if (maze->grid[i][j] != ' ') {
                    char letter[2] = {maze->grid[i][j], '\0'};
                    SDL_Color textColor = {0, 0, 0}; // Noir
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
        }
    }

    // Dessiner le point de départ
    SDL_Rect startRect = {maze->startY * CELL_SIZE, maze->startX * CELL_SIZE, CELL_SIZE, CELL_SIZE};
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Vert pour le point de départ
    SDL_RenderFillRect(renderer, &startRect);

    // Dessiner le point d'arrivée
    SDL_Rect endRect = {maze->endY * CELL_SIZE, maze->endX * CELL_SIZE, CELL_SIZE, CELL_SIZE};
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Rouge pour le point d'arrivée
    SDL_RenderFillRect(renderer, &endRect);
}

void initialize_player(Player *player, Maze *maze) {
    // Placer le joueur au point de départ
    player->x = maze->startX;
    player->y = maze->startY;
    player->score = 0;
}


void move_player(Player *player, Maze *maze, int dx, int dy) {
    int new_x = player->x + dx;
    int new_y = player->y + dy;

    // Check if the new position is within bounds and not a wall
    if (new_x >= 0 && new_x < GRID_SIZE && new_y >= 0 && new_y < GRID_SIZE && maze->grid[new_x][new_y] != '#') {
        player->x = new_x;
        player->y = new_y;
    }
}


int main(int argc, char *argv[]) {
    srand(time(NULL));

    Maze maze;
    initialize_maze(&maze);
    // divide(&maze, 0, 0, GRID_SIZE - 1, GRID_SIZE - 1);

    // Charger les mots depuis le fichier dictionnaire.txt
    char words[100][20];  // Tableau pour stocker jusqu'à 100 mots
    int word_count = load_words("dictionnaire.txt", words, 5);

    const char *word_ptrs[5];
    for (int i = 0; i < word_count; i++) {
        word_ptrs[i] = words[i];
    }

    place_words(&maze, word_ptrs, word_count);
     //afficher la maze avnat divide   sous forme de matrice
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            //ajouter un espace entre les lettres pour une meilleure lisibilité
            printf("%c |", maze.grid[i][j]);

        }
        printf("\n");
    }

    divide(&maze, 0, 0, GRID_SIZE - 1, GRID_SIZE - 1);
    printf("\n");


    //afficher la maze  aprés divide  sous forme de matrice
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            //ajouter un espace entre les lettres pour une meilleure lisibilité
            printf("%c |", maze.grid[i][j]);

        }
        printf("\n");
    }
    printf("\n");

     //afficher la maze  sous forme de matrice
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            //ajouter un espace entre les lettres pour une meilleure lisibilité
            printf("%c |", maze.grid[i][j]);

        }
        printf("\n");
    }
    
   

    // Initialiser SDL et SDL_ttf
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window *window = SDL_CreateWindow("Labyrinthe avec lettres", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_SIZE, WINDOW_SIZE, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("arial.ttf", 20);

    if (!font) {
        printf("Erreur : Impossible de charger la police.\n");
        return -1;
    }

    // Initialiser le joueur
    Player player;
    initialize_player(&player, &maze);

    // Variables de mouvement
    Uint32 lastMoveTime = 0;
    Uint32 moveDelay = 150; // 150 ms de délai entre les déplacements

    int running = 1;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }

        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastMoveTime > moveDelay) {
            const Uint8 *keystate = SDL_GetKeyboardState(NULL);

            if (keystate[SDL_SCANCODE_KP_8]) { // Déplacer vers le haut
                move_player(&player, &maze, -1, 0);
            }
            if (keystate[SDL_SCANCODE_KP_2]) { // Déplacer vers le bas
                move_player(&player, &maze, 1, 0);
            }
            if (keystate[SDL_SCANCODE_KP_4]) { // Déplacer vers la gauche
                move_player(&player, &maze, 0, -1);
            }
            if (keystate[SDL_SCANCODE_KP_6]) { // Déplacer vers la droite
                move_player(&player, &maze, 0, 1);
            }
            if (keystate[SDL_SCANCODE_KP_7]) { // Déplacer en diagonale haut-gauche
                move_player(&player, &maze, -1, -1);
            }
            if (keystate[SDL_SCANCODE_KP_9]) { // Déplacer en diagonale haut-droite
                move_player(&player, &maze, -1, 1);
            }
            if (keystate[SDL_SCANCODE_KP_1]) { // Déplacer en diagonale bas-gauche
                move_player(&player, &maze, 1, -1);
            }
            if (keystate[SDL_SCANCODE_KP_3]) { // Déplacer en diagonale bas-droite
                move_player(&player, &maze, 1, 1);
            }

            // Mettre à jour l'heure du dernier mouvement
            lastMoveTime = currentTime;
        }

        // Effacer l'écran
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Dessiner le labyrinthe et le joueur
        draw_maze(renderer, font, &maze);

        // Dessiner le joueur
        SDL_Rect playerRect = {player.y * CELL_SIZE, player.x * CELL_SIZE, CELL_SIZE, CELL_SIZE};
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Vert pour le joueur
        SDL_RenderFillRect(renderer, &playerRect);

        

        // Afficher l'écran mis à jour
        SDL_RenderPresent(renderer);
    }

    // Nettoyer
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}