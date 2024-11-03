#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define GRID_SIZE 8
#define SHIP_TYPES 3

typedef struct
{
    int size;
    int count;
    char symbol;
} Ship;

Ship ships[SHIP_TYPES] = {
    {4, 1, 'B'}, // Battleship
    {3, 2, 'C'}, // Cruiser
    {2, 2, 'D'}  // Destroyer
};

// Function to initialize grid with '.' for water
void initializeGrid(char grid[GRID_SIZE][GRID_SIZE])
{
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            grid[i][j] = '.';
        }
    }
}

// Function to check if a ship can be placed at a specific location with a given orientation
int canPlaceShip(char grid[GRID_SIZE][GRID_SIZE], int row, int col, int size, int vertical)
{
    for (int i = 0; i < size; i++)
    {
        int r = row + (vertical ? i : 0);
        int c = col + (vertical ? 0 : i);

        if (r >= GRID_SIZE || c >= GRID_SIZE || grid[r][c] != '.')
        {
            return 0;
        }

        // Check surrounding cells for a 1-cell gap requirement
        for (int dr = -1; dr <= 1; dr++)
        {
            for (int dc = -1; dc <= 1; dc++)
            {
                int nr = r + dr, nc = c + dc;
                if (nr >= 0 && nr < GRID_SIZE && nc >= 0 && nc < GRID_SIZE)
                {
                    if (grid[nr][nc] != '.')
                        return 0;
                }
            }
        }
    }
    return 1;
}

// Function to place a ship on the grid
void placeShip(char grid[GRID_SIZE][GRID_SIZE], int row, int col, int size, int vertical, char symbol)
{
    for (int i = 0; i < size; i++)
    {
        int r = row + (vertical ? i : 0);
        int c = col + (vertical ? 0 : i);
        grid[r][c] = symbol;
    }
}

// Function to automatically place ships randomly
void placeShips(char grid[GRID_SIZE][GRID_SIZE])
{
    for (int i = 0; i < SHIP_TYPES; i++)
    {
        for (int j = 0; j < ships[i].count; j++)
        {
            int placed = 0;
            while (!placed)
            {
                int row = rand() % GRID_SIZE;
                int col = rand() % GRID_SIZE;
                int vertical = rand() % 2; // 0 for horizontal, 1 for vertical
                if (canPlaceShip(grid, row, col, ships[i].size, vertical))
                {
                    placeShip(grid, row, col, ships[i].size, vertical, ships[i].symbol);
                    placed = 1;
                }
            }
        }
    }
}

// Function to print the grid
void printGrid(char grid[GRID_SIZE][GRID_SIZE], const char *owner)
{
    printf("%s's Grid:\n", owner);
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            printf("%c ", grid[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

// Function to check if all ships have been hit
int allShipsSunk(char grid[GRID_SIZE][GRID_SIZE])
{
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            if (grid[i][j] == 'B' || grid[i][j] == 'C' || grid[i][j] == 'D')
            {
                return 0;
            }
        }
    }
    return 1;
}

// Function to make a random guess for a shot
void makeGuess(int *row, int *col)
{
    *row = rand() % GRID_SIZE;
    *col = rand() % GRID_SIZE;
}

// Function to check if a guess is a hit
int checkHit(char grid[GRID_SIZE][GRID_SIZE], int row, int col)
{
    if (grid[row][col] == 'B' || grid[row][col] == 'C' || grid[row][col] == 'D')
    {
        grid[row][col] = 'X'; // Mark as hit
        return 1;             // Hit
    }
    return 0; // Miss
}

int main()
{
    int pipefd1[2], pipefd2[2];
    pipe(pipefd1); // Parent writes to pipefd1[1], child reads from pipefd1[0]
    pipe(pipefd2); // Child writes to pipefd2[1], parent reads from pipefd2[0]

    pid_t pid = fork();
    srand(time(NULL) + getpid());

    char parentGrid[GRID_SIZE][GRID_SIZE];
    char childGrid[GRID_SIZE][GRID_SIZE];

    if (pid == 0)
    {
        // Child process
        initializeGrid(childGrid);
        placeShips(childGrid);

        printf("Initial Grids:\n");
        printGrid(childGrid, "Child");

        int parentRow, parentCol;
        while (1)
        {
            // Receive parent's guess
            read(pipefd1[0], &parentRow, sizeof(int));
            read(pipefd1[0], &parentCol, sizeof(int));

            // Check if parent's guess is a hit
            if (checkHit(childGrid, parentRow, parentCol))
            {
                printf("Child: Parent hit my ship at (%d, %d)!\n", parentRow, parentCol);
            }
            else
            {
                printf("Child: Parent missed at (%d, %d).\n", parentRow, parentCol);
            }

            // Display grid after each turn
            printf("Child's Grid After Parent's Turn:\n");
            printGrid(childGrid, "Child");

            // Check if all ships are sunk
            if (allShipsSunk(childGrid))
            {
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
    }
    else
    {
        // Parent process
        initializeGrid(parentGrid);
        placeShips(parentGrid);

        printf("Initial Grids:\n");
        printGrid(parentGrid, "Parent");

        int childRow, childCol;
        while (1)
        {
            // Parent makes a guess
            int parentRow, parentCol;
            makeGuess(&parentRow, &parentCol);

            printf("Parent guesses (%d, %d)\n", parentRow, parentCol);
            write(pipefd1[1], &parentRow, sizeof(int));
            write(pipefd1[1], &parentCol, sizeof(int));

            // Receive child's guess
            read(pipefd2[0], &childRow, sizeof(int));
            read(pipefd2[0], &childCol, sizeof(int));

            // Check if child's guess is a hit
            if (checkHit(parentGrid, childRow, childCol))
            {
                printf("Parent: Child hit my ship at (%d, %d)!\n", childRow, childCol);
            }
            else
            {
                printf("Parent: Child missed at (%d, %d).\n", childRow, childCol);
            }

            // Display grid after each turn
            printf("Parent's Grid After Child's Turn:\n");
            printGrid(parentGrid, "Parent");

            // Check if all ships are sunk
            if (allShipsSunk(parentGrid))
            {
                printf("Child wins! Parent's ships are all sunk.\n");
                exit(0);
            }
        }
    }

    return 0;
}
