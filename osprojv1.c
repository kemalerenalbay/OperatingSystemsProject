#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define GRID_SIZE 4
#define SHIPS 4

// Function to initialize grid with '.' for water
void initializeGrid(char grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = '.';
        }
    }
}

// Function to randomly place 4 ships ('S') on the grid
void placeShips(char grid[GRID_SIZE][GRID_SIZE]) {
    int placed = 0;
    while (placed < SHIPS) {
        int row = rand() % GRID_SIZE;
        int col = rand() % GRID_SIZE;
        if (grid[row][col] == '.') {  // If the spot is empty, place a ship
            grid[row][col] = 'S';
            placed++;
        }
    }
}

// Function to print the grid (for debugging)
void printGrid(char grid[GRID_SIZE][GRID_SIZE], const char *owner) {
    printf("%s's Grid:\n", owner);
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            printf("%c ", grid[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

// Function to check if the guess hits a ship
int checkHit(char grid[GRID_SIZE][GRID_SIZE], int row, int col) {
    return (grid[row][col] == 'S');
}

// Function to randomly guess a point on the opponent's grid
void makeGuess(int *row, int *col) {
    *row = rand() % GRID_SIZE;
    *col = rand() % GRID_SIZE;
}

// Function to check if all ships have been hit
int allShipsSunk(char grid[GRID_SIZE][GRID_SIZE]) {
    int sunk = 1;
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (grid[i][j] == 'S') {
                sunk = 0;
            }
        }
    }
    return sunk;
}

int main() {
    

    int pipefd1[2], pipefd2[2];  // Two pipes for two-way communication
    pipe(pipefd1);  // Parent writes to pipefd1[1], child reads from pipefd1[0]
    pipe(pipefd2);  // Child writes to pipefd2[1], parent reads from pipefd2[0]

    pid_t pid = fork();
    srand(time(NULL) + getpid());
    char parentGrid[GRID_SIZE][GRID_SIZE];
    char childGrid[GRID_SIZE][GRID_SIZE];

    if (pid == 0) {
        // Child process
        initializeGrid(childGrid);
        placeShips(childGrid);
        printGrid(childGrid, "Child");

        int parentRow, parentCol;
        while (1) {
            // Receive parent's guess
            read(pipefd1[0], &parentRow, sizeof(int));
            read(pipefd1[0], &parentCol, sizeof(int));

            // Check if parent hit a ship
            if (checkHit(childGrid, parentRow, parentCol)) {
                printf("Child: Parent hit my ship at (%d, %d)!\n", parentRow, parentCol);
                childGrid[parentRow][parentCol] = 'X';  // Mark hit
            } else {
                printf("Child: Parent missed at (%d, %d).\n", parentRow, parentCol);
            }

            // Check if all ships are sunk
            if (allShipsSunk(childGrid)) {
                printf("Parent wins! Child's ships are all sunk.\n");
                exit(0);
            }

            // Child makes a guess
            int childRow, childCol;
            makeGuess(&childRow, &childCol);

            printf("Child guesses (%d, %d)\n", childRow, childCol);
            write(pipefd2[1], &childRow, sizeof(int));
            write(pipefd2[1], &childCol, sizeof(int));
        }

    } else {
        // Parent process
        initializeGrid(parentGrid);
        placeShips(parentGrid);
        printGrid(parentGrid, "Parent");

        int childRow, childCol;
        while (1) {
            // Parent makes a guess
            int parentRow, parentCol;
            makeGuess(&parentRow, &parentCol);

            printf("Parent guesses (%d, %d)\n", parentRow, parentCol);
            write(pipefd1[1], &parentRow, sizeof(int));
            write(pipefd1[1], &parentCol, sizeof(int));

            // Receive child's guess
            read(pipefd2[0], &childRow, sizeof(int));
            read(pipefd2[0], &childCol, sizeof(int));

            // Check if child hit a ship
            if (checkHit(parentGrid, childRow, childCol)) {
                printf("Parent: Child hit my ship at (%d, %d)!\n", childRow, childCol);
                parentGrid[childRow][childCol] = 'X';  // Mark hit
            } else {
                printf("Parent: Child missed at (%d, %d).\n", childRow, childCol);
            }

            // Check if all ships are sunk
            if (allShipsSunk(parentGrid)) {
                printf("Child wins! Parent's ships are all sunk.\n");
                exit(0);
            }
        }
    }

    return 0;
}
