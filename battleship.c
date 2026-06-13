#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// Constants for board dimensions to avoid hardcoding sizes
#define BOARD_SIZE 10
#define MAX_INPUT_LENGTH 10
#define BUFFER_SIZE 256

// Enum for different ship types including empty space
// This provides type safety and clear representation of ship data
typedef enum 
{
    NO_SHIP,     // Empty water space
    CARRIER,     // 5 squares
    BATTLESHIP,  // 4 squares
    CRUISER,     // 3 squares
    SUBMARINE,   // 2 squares
    DESTROYER,   // 1 square
    DESTROYED    // Ship square that has been hit
} ShipType;

// Enum for tracking shot status on our tracking grid
// Used to track where we have fired shots
typedef enum 
{
    UNTRIED,     // Haven't shot at this location yet
    MISS,        // Shot and missed
    HIT          // Shot and hit a ship
} ShotStatus;

// Structure to hold game state - avoids global variables
// This encapsulates all game data in one place for better organization
typedef struct 
{
    ShipType **shipGrid;      // Grid showing where our ships are placed
    ShotStatus **shotGrid;    // Grid tracking where we have shot
    int shipsPlaced;          // Counter for ships successfully placed
} GameState;

// Structure to pass information between UpdateState and DisplayWorld
typedef struct
{
    int playerShotRow;
    int playerShotCol;
    int playerHit;
    int computerShotRow;
    int computerShotCol;
    int computerHit;
    int playerWon;
    int computerWon;
} TurnResult;

// Function prototypes - organized by purpose
GameState* initializeGame(void);
void cleanupGame(GameState *game);
int placeShips(GameState *game);
void displayBoard(ShipType **grid);
void displayShotGrid(ShotStatus **grid);
char* getShipSymbol(ShipType ship);
char* getShotSymbol(ShotStatus shot);
int parseInput(const char *input, int *startRow, int *startCol, int *endRow, int *endCol);
int validatePlacement(GameState *game, int startRow, int startCol, int endRow, int endCol, int expectedLength);
void placeShipOnGrid(GameState *game, int startRow, int startCol, int endRow, int endCol, ShipType shipType);
int calculateDistance(int startRow, int startCol, int endRow, int endCol);

// Single player functions
GameState* SetupSinglePlayer(void);
void TeardownSinglePlayer(GameState *computer);
int MakeSinglePlayerShot(GameState *computer, int row, int col);
void GetSinglePlayerShot(GameState *player, int *row, int *col);
void SinglePlayerResponse(GameState *computer, int row, int col, int hit);
int SinglePlayerDidWin(GameState *player);
TurnResult UpdateState(GameState *player, GameState *computer, int shotRow, int shotCol);
void DisplayWorld(GameState *player, TurnResult *result);
int placeShipRandomly(GameState *game, int length, ShipType shipType);

// Two player network functions
int SetupTwoPlayer(char *ipAddress, char *port, int isServer);
void TeardownTwoPlayer(int sockfd);
int MakeTwoPlayerShot(int sockfd, int row, int col);
void GetTwoPlayerShot(int sockfd, int *row, int *col);
void TwoPlayerResponse(int sockfd, int row, int col, int hit);
int TwoPlayerDidWin(int sockfd);
TurnResult UpdateStateTwoPlayer(GameState *player, int sockfd, int shotRow, int shotCol, int isServer);
int sendMessage(int sockfd, const char *message);
int receiveMessage(int sockfd, char *buffer, int bufferSize);

/**
 * Main function - controls overall game flow without using global variables
 * Supports three modes: single player, server, and client
 * Returns 0 on successful completion, 1 on error
 */
int main(int argc, char *argv[])
{
    // Seed random number generator
    srand(time(NULL));
    
    // Determine game mode based on command line arguments
    int networkMode = 0; // 0 = single player, 1 = network
    int isServer = 0;
    int sockfd = -1;
    
    if (argc == 1)
    {
        // Single player mode
        printf("Starting single player mode...\n");
        networkMode = 0;
    }
    else if (argc == 2)
    {
        // Server mode - one parameter is the port
        printf("Starting server mode on port %s...\n", argv[1]);
        networkMode = 1;
        isServer = 1;
        sockfd = SetupTwoPlayer(NULL, argv[1], isServer);
        if (sockfd < 0)
        {
            printf("Failed to setup server.\n");
            return 1;
        }
    }
    else if (argc == 3)
    {
        // Client mode - first parameter is IP, second is port
        printf("Starting client mode, connecting to %s:%s...\n", argv[1], argv[2]);
        networkMode = 1;
        isServer = 0;
        sockfd = SetupTwoPlayer(argv[1], argv[2], isServer);
        if (sockfd < 0)
        {
            printf("Failed to connect to server.\n");
            return 1;
        }
    }
    else
    {
        printf("Usage:\n");
        printf("  Single player: %s\n", argv[0]);
        printf("  Server mode:   %s <port>\n", argv[0]);
        printf("  Client mode:   %s <ip_address> <port>\n", argv[0]);
        return 1;
    }
    
    // Initialize game state - allocates memory and sets up grids
    GameState *player = initializeGame();
    if (player == NULL) 
	{
        printf("Failed to initialize game. Exiting.\n");
        if (networkMode) TeardownTwoPlayer(sockfd);
        return 1;
    }
    
    // Run ship placement phase for player
    int placementResult = placeShips(player);
    if (placementResult == 0) 
	{
        printf("Ship placement complete.\n\n");
    } 
	else 
	{
        printf("Ship placement failed.\n");
        cleanupGame(player);
        if (networkMode) TeardownTwoPlayer(sockfd);
        return 1;
    }
    
    GameState *computer = NULL;
    
    // Setup opponent based on mode
    if (networkMode)
    {
        printf("Waiting for opponent to finish ship placement...\n");
        // Signal that we're ready
        sendMessage(sockfd, "READY");
        
        // Wait for opponent to be ready
        char buffer[BUFFER_SIZE];
        if (receiveMessage(sockfd, buffer, BUFFER_SIZE) <= 0)
        {
            printf("Connection lost.\n");
            cleanupGame(player);
            TeardownTwoPlayer(sockfd);
            return 1;
        }
        printf("Opponent ready. Game starting!\n\n");
    }
    else
    {
        // Setup computer player for single player
        computer = SetupSinglePlayer();
        if (computer == NULL)
        {
            printf("Failed to setup computer player.\n");
            cleanupGame(player);
            return 1;
        }
        printf("Computer has placed its ships. Game starting!\n\n");
    }
    
    // Main game loop
    int gameOver = 0;
    while (!gameOver)
    {
        // Display current state
        DisplayWorld(player, NULL);
        
        // AcceptInput - get player's shot
        printf("Enter coordinates to fire at (e.g., A5): ");
        char input[MAX_INPUT_LENGTH];
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            printf("Error reading input.\n");
            break;
        }
        
        // Remove newline
        size_t len = strlen(input);
        if (len > 0 && input[len-1] == '\n')
        {
            input[len-1] = '\0';
        }
        
        // Parse coordinates
        int shotRow, shotCol, dummy1, dummy2;
        if (parseInput(input, &shotRow, &shotCol, &dummy1, &dummy2) != 0 || shotRow != dummy1 || shotCol != dummy2)
        {
            printf("Invalid input format. Use format like A5\n");
            continue;
        }
        
        // Check if already shot at this location
        if (player->shotGrid[shotRow][shotCol] != UNTRIED)
        {
            printf("You already shot at this location!\n");
            continue;
        }
        
        // UpdateState - process the turn
        TurnResult result;
        if (networkMode)
        {
            result = UpdateStateTwoPlayer(player, sockfd, shotRow, shotCol, isServer);
            if (result.playerWon == -1 && result.computerWon == -1)
            {
                printf("Network error. Game ending.\n");
                break;
            }
        }
        else
        {
            result = UpdateState(player, computer, shotRow, shotCol);
        }
        
        // DisplayWorld - show results
        DisplayWorld(player, &result);
        
        // Check for game over
        if (result.playerWon)
        {
            printf("\n*** Congratulations! You sank all enemy ships! ***\n");
            gameOver = 1;
        }
        else if (result.computerWon)
        {
            if (networkMode)
            {
                printf("\n*** Game Over! Your opponent sank all your ships! ***\n");
            }
            else
            {
                printf("\n*** Game Over! The computer sank all your ships! ***\n");
            }
            gameOver = 1;
        }
    }
    
    // Clean up allocated memory - prevents memory leaks
    if (networkMode)
    {
        TeardownTwoPlayer(sockfd);
    }
    else
    {
        TeardownSinglePlayer(computer);
    }
    cleanupGame(player);
    
    return 0;
}

/**
 * Initialize the game by allocating memory for both grids
 * This function handles all dynamic memory allocation with proper error checking
 * Returns pointer to GameState on success, NULL on failure
 */
GameState* initializeGame(void) 
{
    // Allocate memory for the game state structure
    GameState *game = malloc(sizeof(GameState));
    if (game == NULL) 
	{
        printf("Memory allocation failed for game state.\n");
        return NULL;
    }
    
    // Initialize ship counter
    game->shipsPlaced = 0;
    
    // Allocate memory for ship grid (rows)
    game->shipGrid = malloc(BOARD_SIZE * sizeof(ShipType*));
    if (game->shipGrid == NULL) 
	{
        printf("Memory allocation failed for ship grid.\n");
        free(game);
        return NULL;
    }
    
    // Allocate memory for shot grid (rows)
    game->shotGrid = malloc(BOARD_SIZE * sizeof(ShotStatus*));
    if (game->shotGrid == NULL) 
	{
        printf("Memory allocation failed for shot grid.\n");
        free(game->shipGrid);
        free(game);
        return NULL;
    }
    
    // Allocate memory for each row and initialize values
    for (int i = 0; i < BOARD_SIZE; i++) 
	{
        // Allocate columns for ship grid
        game->shipGrid[i] = malloc(BOARD_SIZE * sizeof(ShipType));
        if (game->shipGrid[i] == NULL) 
		{
            printf("Memory allocation failed for ship grid row %d.\n", i);
            // Clean up previously allocated rows before failing
            for (int j = 0; j < i; j++) 
			{
                free(game->shipGrid[j]);
            }
            free(game->shipGrid);
            free(game->shotGrid);
            free(game);
            return NULL;
        }
        
        // Allocate columns for shot grid
        game->shotGrid[i] = malloc(BOARD_SIZE * sizeof(ShotStatus));
        if (game->shotGrid[i] == NULL) 
		{
            printf("Memory allocation failed for shot grid row %d.\n", i);
            // Clean up previously allocated rows before failing
            for (int j = 0; j <= i; j++) 
			{
                free(game->shipGrid[j]);
            }
            for (int j = 0; j < i; j++) 
			{
                free(game->shotGrid[j]);
            }
            free(game->shipGrid);
            free(game->shotGrid);
            free(game);
            return NULL;
        }
        
        // Initialize grid values to empty/untried
        for (int j = 0; j < BOARD_SIZE; j++) 
		{
            game->shipGrid[i][j] = NO_SHIP;
            game->shotGrid[i][j] = UNTRIED;
        }
    }
    
    return game;
}

/**
 * Clean up dynamically allocated memory for the game
 * This prevents memory leaks by properly freeing all allocated memory
 */
void cleanupGame(GameState *game) 
{
    if (game == NULL) 
	{
        return;
    }
    
    // Free each row of both grids
    for (int i = 0; i < BOARD_SIZE; i++) 
	{
        if (game->shipGrid && game->shipGrid[i]) 
		{
            free(game->shipGrid[i]);
        }
        if (game->shotGrid && game->shotGrid[i]) 
		{
            free(game->shotGrid[i]);
        }
    }
    
    // Free the row pointer arrays
    if (game->shipGrid) 
	{
        free(game->shipGrid);
    }
    if (game->shotGrid) 
	{
        free(game->shotGrid);
    }
    
    // Free the game state structure
    free(game);
}

/**
 * Handle the ship placement mini-game loop
 * This implements the game-loop structure within ship placement:
 * Initialize -> Loop(AcceptInput -> UpdateState -> DisplayWorld) -> Teardown
 * Returns 0 on success, 1 on failure
 */
int placeShips(GameState *game) 
{
    if (game == NULL) 
	{
        return 1;
    }
    
    // Array of ship types and their lengths for placement order
    ShipType ships[] = {CARRIER, BATTLESHIP, CRUISER, SUBMARINE, DESTROYER};
    int shipLengths[] = {5, 4, 3, 2, 1};
    int numShips = sizeof(ships) / sizeof(ships[0]);
    
    // Initialize: Print instructions and initial board
    printf("Place your ships. Format examples:\n");
    printf("  AE4 = Vertical ship from A4 to E4 (5 squares)\n");
    printf("  J37 = Horizontal ship from J3 to J7 (5 squares)\n");
    printf("  I2J2 = Vertical ship from I2 to J2 (2 squares)\n");
    printf("  A3 = Single square at A3 (1 square)\n");
    displayBoard(game->shipGrid);
    
    // Ship placement loop - one iteration per ship type
    for (int shipIndex = 0; shipIndex < numShips; shipIndex++) 
	{
        int shipPlaced = 0; // Flag to track if current ship has been successfully placed
        
        // Keep trying until this ship is placed successfully
        while (!shipPlaced) 
		{
            // AcceptInput: Get ship placement from user
            printf("Please enter a location for a ship of %d squares: ", shipLengths[shipIndex]);
            
            char input[MAX_INPUT_LENGTH];
            if (fgets(input, sizeof(input), stdin) == NULL) 
			{
                printf("Error reading input.\n");
                return 1;
            }
            
            // Remove newline character if present for cleaner processing
            size_t len = strlen(input);
            if (len > 0 && input[len-1] == '\n') 
			{
                input[len-1] = '\0';
            }
            
            // Parse input to get coordinates
            int startRow, startCol, endRow, endCol;
            if (parseInput(input, &startRow, &startCol, &endRow, &endCol) != 0) 
			{
                printf("Invalid input format.\n");
                displayBoard(game->shipGrid);
                continue; // Ask for input again
            }
            
            // UpdateState: Validate placement and update grid if valid
            if (validatePlacement(game, startRow, startCol, endRow, endCol, shipLengths[shipIndex]) == 0) 
			{
                placeShipOnGrid(game, startRow, startCol, endRow, endCol, ships[shipIndex]);
                shipPlaced = 1; // Mark this ship as successfully placed
                game->shipsPlaced++;
            }
            
            // DisplayWorld: Show the updated board
            displayBoard(game->shipGrid);
        }
    }
    
    // Teardown: All ships have been placed successfully
    return 0;
}

/**
 * Display the game board with proper formatting
 * Shows column numbers at top and row letters on left
 * This function is called separately to maintain clean code organization
 */
void displayBoard(ShipType **grid) 
{
    // Print column headers (0-9)
    printf("    ");
    for (int col = 0; col < BOARD_SIZE; col++) 
	{
        printf("%d    ", col);
    }
    printf("\n");
    
    // Print each row with row letter and grid contents
    for (int row = 0; row < BOARD_SIZE; row++) 
	{
        printf("%c  ", 'A' + row); // Row letter (A-J)
        
        for (int col = 0; col < BOARD_SIZE; col++) 
		{
            printf("%s |", getShipSymbol(grid[row][col]));
        }
        printf("\n");
    }
}

/**
 * Display the shot tracking grid
 */
void displayShotGrid(ShotStatus **grid)
{
    // Print column headers (0-9)
    printf("    ");
    for (int col = 0; col < BOARD_SIZE; col++)
    {
        printf("%d    ", col);
    }
    printf("\n");
    
    // Print each row with row letter and grid contents
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        printf("%c  ", 'A' + row);
        
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            printf("%s |", getShotSymbol(grid[row][col]));
        }
        printf("\n");
    }
}

/**
 * Convert ship type enum to display string
 * Returns appropriate symbol for each ship type for board display
 */
char* getShipSymbol(ShipType ship) {
    switch (ship) {
        case CARRIER:    return " CV "; // Carrier
        case BATTLESHIP: return " BB "; // Battleship
        case CRUISER:    return " CA "; // Cruiser (CA for heavy cruiser)
        case SUBMARINE:  return " SS "; // Submarine
        case DESTROYER:  return " DD "; // Destroyer
        case DESTROYED:  return " XX "; // Destroyed ship piece
        case NO_SHIP:    return "    "; // Empty water
        default:         return " ?? "; // Unknown (shouldn't happen)
    }
}

/**
 * Convert shot status enum to display string
 */
char* getShotSymbol(ShotStatus shot) {
    switch (shot) {
        case UNTRIED: return "    "; // Haven't shot here
        case MISS:    return " -- "; // Missed shot
        case HIT:     return " XX "; // Hit a ship
        default:      return " ?? "; // Unknown
    }
}

/**
 * Parse user input string to extract start and end coordinates
 * Handles both formats: AE4 (vertical from A4 to E4) and J37 (horizontal from J3 to J7)
 * Input validation ensures coordinates are within bounds
 * Returns 0 on success, 1 on failure
 */
int parseInput(const char *input, int *startRow, int *startCol, int *endRow, int *endCol) 
{
    if (input == NULL || strlen(input) < 2) 
	{
        return 1;
    }
    
    // Convert input to uppercase for consistent processing
    char upperInput[MAX_INPUT_LENGTH];
    strcpy(upperInput, input);
    for (int i = 0; upperInput[i]; i++) 
	{
        upperInput[i] = toupper(upperInput[i]);
    }
    
    // Check if first character is a valid row letter (A-J)
    if (upperInput[0] < 'A' || upperInput[0] > 'J') 
	{
        printf("Location specification is not in bounds\n");
        return 1;
    }
    
    // Handle single coordinate first (for 1-square ships)
    if (strlen(upperInput) == 2 && isdigit(upperInput[1])) 
	{
        // Format: A3 - single square at A3
        *startRow = upperInput[0] - 'A';
        *endRow = upperInput[0] - 'A';
        *startCol = upperInput[1] - '0';
        *endCol = upperInput[1] - '0';
        
        // Check bounds
        if (*startCol < 0 || *startCol >= BOARD_SIZE) {
            printf("Location specification is not in bounds\n");
            return 1;
        }
        
    // Check format: AE4 (vertical) vs J37 (horizontal)
    } 
	else if (strlen(upperInput) == 3 && upperInput[1] >= 'A' && upperInput[1] <= 'J' && isdigit(upperInput[2])) 
	{
        // Format: AE4 - vertical ship from A4 to E4
        *startRow = upperInput[0] - 'A';
        *endRow = upperInput[1] - 'A';
        *startCol = upperInput[2] - '0';
        *endCol = upperInput[2] - '0';
        
        // Check bounds for column number
        if (*startCol < 0 || *startCol >= BOARD_SIZE) 
		{
            printf("Location specification is not in bounds\n");
            return 1;
        }
        
    } 
	else if (strlen(upperInput) == 3 && isdigit(upperInput[1]) && isdigit(upperInput[2])) 
	{
        // Format: J37 - horizontal ship from J3 to J7
        *startRow = upperInput[0] - 'A';
        *endRow = upperInput[0] - 'A';  // Same row for horizontal placement
        *startCol = upperInput[1] - '0';
        *endCol = upperInput[2] - '0';
        
        // Check bounds for column numbers
        if (*startCol < 0 || *startCol >= BOARD_SIZE || *endCol < 0 || *endCol >= BOARD_SIZE) 
		{
            printf("Location specification is not in bounds\n");
            return 1;
        }
        
    } 
	else if (strlen(upperInput) == 4 && isdigit(upperInput[1]) && upperInput[2] >= 'A' && upperInput[2] <= 'J' && isdigit(upperInput[3])) 
	{
        // Format: I2J2 - vertical ship from I2 to J2
        *startRow = upperInput[0] - 'A';
        *endRow = upperInput[2] - 'A';
        *startCol = upperInput[1] - '0';
        *endCol = upperInput[3] - '0';
        
        // Check bounds for column numbers  
        if (*startCol < 0 || *startCol >= BOARD_SIZE || *endCol < 0 || *endCol >= BOARD_SIZE) 
		{
            printf("Location specification is not in bounds\n");
            return 1;
        }
        
        // For this format, columns should be the same
        if (*startCol != *endCol) 
		{
            return 1; // Invalid - should be same column for vertical placement
        }
        
    } 
	else 
	{
        return 1; // Invalid format
    }
    
    return 0;
}

/**
 * Validate that the ship placement is legal
 * Checks bounds, length, and collision with existing ships
 * Provides specific error messages for different validation failures
 * Returns 0 if valid, 1 if invalid
 */
int validatePlacement(GameState *game, int startRow, int startCol, int endRow, int endCol, int expectedLength) 
{
    // Calculate actual length of placement
    int actualLength = calculateDistance(startRow, startCol, endRow, endCol);
    
    // Check if length matches expected
    if (actualLength != expectedLength) 
	{
        printf("Ship is %d in size, but should be %d.\n", actualLength, expectedLength);
        return 1;
    }
    
    // Determine direction and check for collisions
    int rowStep = (endRow > startRow) ? 1 : (endRow < startRow) ? -1 : 0;
    int colStep = (endCol > startCol) ? 1 : (endCol < startCol) ? -1 : 0;
    
    int currentRow = startRow;
    int currentCol = startCol;
    
    // Check each position along the ship's path
    for (int i = 0; i < actualLength; i++) {
        // Check bounds
        if (currentRow < 0 || currentRow >= BOARD_SIZE || currentCol < 0 || currentCol >= BOARD_SIZE) 
		{
            printf("Location specification is not in bounds\n");
            return 1;
        }
        
        // Check for existing ship collision
        if (game->shipGrid[currentRow][currentCol] != NO_SHIP) 
		{
            printf("Ship found at  %c, %d.\n", 'A' + currentRow, currentCol);
            return 1;
        }
        
        // Move to next position
        if (i < actualLength - 1) 
		{ // Don't move past the end
            currentRow += rowStep;
            currentCol += colStep;
        }
    }
    
    return 0; // Placement is valid
}

/**
 * Place a ship on the grid after validation
 * Updates all positions along the ship's length with the ship type
 */
void placeShipOnGrid(GameState *game, int startRow, int startCol, int endRow, int endCol, ShipType shipType) 
{
    // Determine direction
    int rowStep = (endRow > startRow) ? 1 : (endRow < startRow) ? -1 : 0;
    int colStep = (endCol > startCol) ? 1 : (endCol < startCol) ? -1 : 0;
    
    int currentRow = startRow;
    int currentCol = startCol;
    int length = calculateDistance(startRow, startCol, endRow, endCol);
    
    // Place ship at each position along its path
    for (int i = 0; i < length; i++) 
	{
        game->shipGrid[currentRow][currentCol] = shipType;
        
        // Move to next position (except for last iteration)
        if (i < length - 1) 
		{
            currentRow += rowStep;
            currentCol += colStep;
        }
    }
}

/**
 * Calculate the distance/length of a ship placement
 * Works for both horizontal and vertical placements
 * Returns the number of squares the ship occupies
 */
int calculateDistance(int startRow, int startCol, int endRow, int endCol) 
{
    int rowDiff = abs(endRow - startRow);
    int colDiff = abs(endCol - startCol);
    
    // For horizontal placement, column difference determines length
    // For vertical placement, row difference determines length
    // Add 1 because we count inclusive (A4 to E4 is 5 squares, not 4)
    return (rowDiff > colDiff) ? (rowDiff + 1) : (colDiff + 1);
}

/**
 * Setup computer player - allocate grids and randomly place ships
 */
GameState* SetupSinglePlayer(void)
{
    GameState *computer = initializeGame();
    if (computer == NULL)
    {
        return NULL;
    }
    
    // Place ships randomly
    ShipType ships[] = {CARRIER, BATTLESHIP, CRUISER, SUBMARINE, DESTROYER};
    int shipLengths[] = {5, 4, 3, 2, 1};
    int numShips = sizeof(ships) / sizeof(ships[0]);
    
    for (int i = 0; i < numShips; i++)
    {
        if (placeShipRandomly(computer, shipLengths[i], ships[i]) != 0)
        {
            cleanupGame(computer);
            return NULL;
        }
    }
    
    return computer;
}

/**
 * Teardown computer player - free allocated memory
 */
void TeardownSinglePlayer(GameState *computer)
{
    cleanupGame(computer);
}

/**
 * Randomly place a ship on the computer's grid
 */
int placeShipRandomly(GameState *game, int length, ShipType shipType)
{
    int maxAttempts = 1000;
    
    for (int attempt = 0; attempt < maxAttempts; attempt++)
    {
        // Random starting position
        int startRow = rand() % BOARD_SIZE;
        int startCol = rand() % BOARD_SIZE;
        
        // Random direction (0 = horizontal, 1 = vertical)
        int vertical = rand() % 2;
        
        int endRow, endCol;
        if (vertical)
        {
            endRow = startRow + length - 1;
            endCol = startCol;
        }
        else
        {
            endRow = startRow;
            endCol = startCol + length - 1;
        }
        
        // Check if placement is valid
        if (validatePlacement(game, startRow, startCol, endRow, endCol, length) == 0)
        {
            placeShipOnGrid(game, startRow, startCol, endRow, endCol, shipType);
            return 0;
        }
    }
    
    return 1; // Failed to place ship
}

/**
 * Make a shot at the computer's grid - returns 1 for hit, 0 for miss
 */
int MakeSinglePlayerShot(GameState *computer, int row, int col)
{
    if (computer->shipGrid[row][col] != NO_SHIP && computer->shipGrid[row][col] != DESTROYED)
    {
        // Hit!
        computer->shipGrid[row][col] = DESTROYED;
        return 1;
    }
    return 0; // Miss
}

/**
 * Get a random shot from the computer that hasn't been tried yet
 */
void GetSinglePlayerShot(GameState *player, int *row, int *col)
{
    int found = 0;
    while (!found)
    {
        *row = rand() % BOARD_SIZE;
        *col = rand() % BOARD_SIZE;
        
        if (player->shotGrid[*row][*col] == UNTRIED)
        {
            found = 1;
        }
    }
}

/**
 * Update computer's shot grid based on hit/miss result
 */
void SinglePlayerResponse(GameState *computer, int row, int col, int hit)
{
    if (hit)
    {
        computer->shotGrid[row][col] = HIT;
    }
    else
    {
        computer->shotGrid[row][col] = MISS;
    }
}

/**
 * Check if a player has won (opponent has no ships left)
 */
int SinglePlayerDidWin(GameState *opponent)
{
    // Check if all non-empty squares are destroyed
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            ShipType cell = opponent->shipGrid[row][col];
            if (cell != NO_SHIP && cell != DESTROYED)
            {
                return 0; // Found a ship piece that's not destroyed
            }
        }
    }
    return 1; // All ships destroyed
}

/**
 * Process a game turn - player shoots, computer shoots, check for winners
 */
TurnResult UpdateState(GameState *player, GameState *computer, int shotRow, int shotCol)
{
    TurnResult result;
    result.playerShotRow = shotRow;
    result.playerShotCol = shotCol;
    
    // Player's shot at computer
    result.playerHit = MakeSinglePlayerShot(computer, shotRow, shotCol);
    
    // Update player's shot tracking grid
    if (result.playerHit)
    {
        player->shotGrid[shotRow][shotCol] = HIT;
    }
    else
    {
        player->shotGrid[shotRow][shotCol] = MISS;
    }
    
    // Computer's shot at player
    GetSinglePlayerShot(player, &result.computerShotRow, &result.computerShotCol);
    
    // Check if computer hit player's ship
    if (player->shipGrid[result.computerShotRow][result.computerShotCol] != NO_SHIP &&
        player->shipGrid[result.computerShotRow][result.computerShotCol] != DESTROYED)
    {
        result.computerHit = 1;
        player->shipGrid[result.computerShotRow][result.computerShotCol] = DESTROYED;
    }
    else
    {
        result.computerHit = 0;
    }
    
    // Update computer's response
    SinglePlayerResponse(computer, result.computerShotRow, result.computerShotCol, result.computerHit);
    
    // Check for winners
    result.playerWon = SinglePlayerDidWin(computer);
    result.computerWon = SinglePlayerDidWin(player);
    
    return result;
}

/**
 * Display the game world - both grids and turn results
 */
void DisplayWorld(GameState *player, TurnResult *result)
{
    printf("\n============================================\n");
    printf("Your Shots (tracking enemy ships):\n");
    displayShotGrid(player->shotGrid);
    
    printf("\nYour Ships:\n");
    displayBoard(player->shipGrid);
    printf("============================================\n");
    
    if (result != NULL)
    {
        printf("\nYour shot at %c%d: %s\n", 
               'A' + result->playerShotRow, result->playerShotCol,
               result->playerHit ? "HIT!" : "Miss.");
        printf("Opponent shot at %c%d: %s\n",
               'A' + result->computerShotRow, result->computerShotCol,
               result->computerHit ? "HIT!" : "Miss.");
        printf("\n");
    }
}

/**
 * Setup network connection - server or client
 * Returns socket file descriptor on success, -1 on failure
 */
int SetupTwoPlayer(char *ipAddress, char *port, int isServer)
{
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;
    int portNum = atoi(port);
    
    if (isServer)
    {
        // Create socket for server
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            perror("Error opening socket");
            return -1;
        }
        
        // Allow socket reuse to avoid "Address already in use" errors
        int opt = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            perror("setsockopt failed");
            close(sockfd);
            return -1;
        }
        
        // Setup server address structure
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(portNum);
        
        // Bind socket to port
        if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            perror("Error on binding");
            close(sockfd);
            return -1;
        }
        
        // Listen for incoming connections
        if (listen(sockfd, 1) < 0)
        {
            perror("Error on listen");
            close(sockfd);
            return -1;
        }
        
        printf("Waiting for client connection...\n");
        
        // Accept client connection
        socklen_t clientLen = sizeof(clientAddr);
        int newsockfd = accept(sockfd, (struct sockaddr *)&clientAddr, &clientLen);
        if (newsockfd < 0)
        {
            perror("Error on accept");
            close(sockfd);
            return -1;
        }
        
        printf("Client connected!\n");
        close(sockfd); // Close listening socket, keep connection socket
        return newsockfd;
    }
    else
    {
        // Client mode - connect to server
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            perror("Error opening socket");
            return -1;
        }
        
        // Setup server address structure
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(portNum);
        
        // Convert IP address from text to binary
        if (inet_pton(AF_INET, ipAddress, &serverAddr.sin_addr) <= 0)
        {
            perror("Invalid address");
            close(sockfd);
            return -1;
        }
        
        // Connect to server
        if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            perror("Connection failed");
            close(sockfd);
            return -1;
        }
        
        printf("Connected to server!\n");
        return sockfd;
    }
}

/**
 * Close network connection and cleanup
 */
void TeardownTwoPlayer(int sockfd)
{
    if (sockfd >= 0)
    {
        close(sockfd);
    }
}

/**
 * Send a message over the network socket
 * Returns number of bytes sent, or -1 on error
 */
int sendMessage(int sockfd, const char *message)
{
    int len = strlen(message);
    int n = send(sockfd, message, len, 0);
    if (n < 0)
    {
        perror("Error sending message");
    }
    return n;
}

/**
 * Receive a message from the network socket
 * Returns number of bytes received, or -1 on error
 */
int receiveMessage(int sockfd, char *buffer, int bufferSize)
{
    memset(buffer, 0, bufferSize);
    int n = recv(sockfd, buffer, bufferSize - 1, 0);
    if (n < 0)
    {
        perror("Error receiving message");
    }
    return n;
}

/**
 * Make a shot over the network - sends coordinates and receives hit/miss result
 * Returns 1 for hit, 0 for miss, -1 for network error
 */
int MakeTwoPlayerShot(int sockfd, int row, int col)
{
    char message[BUFFER_SIZE];
    snprintf(message, BUFFER_SIZE, "SHOT %d %d", row, col);
    
    if (sendMessage(sockfd, message) < 0)
    {
        return -1;
    }
    
    // Receive response (HIT or MISS)
    char buffer[BUFFER_SIZE];
    if (receiveMessage(sockfd, buffer, BUFFER_SIZE) <= 0)
    {
        return -1;
    }
    
    if (strcmp(buffer, "HIT") == 0)
    {
        return 1;
    }
    else if (strcmp(buffer, "MISS") == 0)
    {
        return 0;
    }
    
    return -1; // Invalid response
}

/**
 * Get opponent's shot from network - receives coordinates and sends back hit/miss
 */
void GetTwoPlayerShot(int sockfd, int *row, int *col)
{
    char buffer[BUFFER_SIZE];
    
    if (receiveMessage(sockfd, buffer, BUFFER_SIZE) <= 0)
    {
        *row = -1;
        *col = -1;
        return;
    }
    
    // Parse "SHOT row col" message
    char command[BUFFER_SIZE];
    sscanf(buffer, "%s %d %d", command, row, col);
}

/**
 * Send response to opponent's shot over network
 */
void TwoPlayerResponse(int sockfd, int row, int col, int hit)
{
    if (hit)
    {
        sendMessage(sockfd, "HIT");
    }
    else
    {
        sendMessage(sockfd, "MISS");
    }
}

/**
 * Check if player won by querying opponent over network
 * Returns 1 if won, 0 if not, -1 on network error
 */
int TwoPlayerDidWin(int sockfd)
{
    sendMessage(sockfd, "CHECK_WIN");
    
    char buffer[BUFFER_SIZE];
    if (receiveMessage(sockfd, buffer, BUFFER_SIZE) <= 0)
    {
        return -1;
    }
    
    if (strcmp(buffer, "WIN") == 0)
    {
        return 1;
    }
    else if (strcmp(buffer, "CONTINUE") == 0)
    {
        return 0;
    }
    
    return -1;
}

/**
 * Process a game turn over network
 * Handles turn order based on server/client role
 */
TurnResult UpdateStateTwoPlayer(GameState *player, int sockfd, int shotRow, int shotCol, int isServer)
{
    TurnResult result;
    result.playerShotRow = shotRow;
    result.playerShotCol = shotCol;
    
    // Server goes first, client goes second
    if (isServer)
    {
        // Server: make shot first
        result.playerHit = MakeTwoPlayerShot(sockfd, shotRow, shotCol);
        if (result.playerHit < 0)
        {
            result.playerWon = -1;
            result.computerWon = -1;
            return result;
        }
        
        // Update shot grid
        player->shotGrid[shotRow][shotCol] = result.playerHit ? HIT : MISS;
        
        // Check if we won
        result.playerWon = TwoPlayerDidWin(sockfd);
        if (result.playerWon < 0)
        {
            result.computerWon = -1;
            return result;
        }
        
        // If game continues, receive opponent's shot
        if (!result.playerWon)
        {
            GetTwoPlayerShot(sockfd, &result.computerShotRow, &result.computerShotCol);
            if (result.computerShotRow < 0)
            {
                result.playerWon = -1;
                result.computerWon = -1;
                return result;
            }
            
            // Check if opponent hit our ship
            if (player->shipGrid[result.computerShotRow][result.computerShotCol] != NO_SHIP &&
                player->shipGrid[result.computerShotRow][result.computerShotCol] != DESTROYED)
            {
                result.computerHit = 1;
                player->shipGrid[result.computerShotRow][result.computerShotCol] = DESTROYED;
            }
            else
            {
                result.computerHit = 0;
            }
            
            // Send response
            TwoPlayerResponse(sockfd, result.computerShotRow, result.computerShotCol, result.computerHit);
            
            // Check if opponent won
            result.computerWon = SinglePlayerDidWin(player);
            
            // Send win status
            char buffer[BUFFER_SIZE];
            receiveMessage(sockfd, buffer, BUFFER_SIZE); // Receive CHECK_WIN
            sendMessage(sockfd, result.computerWon ? "WIN" : "CONTINUE");
        }
        else
        {
            result.computerWon = 0;
        }
    }
    else
    {
        // Client: receive opponent's shot first
        GetTwoPlayerShot(sockfd, &result.computerShotRow, &result.computerShotCol);
        if (result.computerShotRow < 0)
        {
            result.playerWon = -1;
            result.computerWon = -1;
            return result;
        }
        
        // Check if opponent hit our ship
        if (player->shipGrid[result.computerShotRow][result.computerShotCol] != NO_SHIP &&
            player->shipGrid[result.computerShotRow][result.computerShotCol] != DESTROYED)
        {
            result.computerHit = 1;
            player->shipGrid[result.computerShotRow][result.computerShotCol] = DESTROYED;
        }
        else
        {
            result.computerHit = 0;
        }
        
        // Send response
        TwoPlayerResponse(sockfd, result.computerShotRow, result.computerShotCol, result.computerHit);
        
        // Check if opponent won
        result.computerWon = SinglePlayerDidWin(player);
        
        // Receive and respond to win check
        char buffer[BUFFER_SIZE];
        receiveMessage(sockfd, buffer, BUFFER_SIZE); // Receive CHECK_WIN
        sendMessage(sockfd, result.computerWon ? "WIN" : "CONTINUE");
        
        // If opponent didn't win, make our shot
        if (!result.computerWon)
        {
            result.playerHit = MakeTwoPlayerShot(sockfd, shotRow, shotCol);
            if (result.playerHit < 0)
            {
                result.playerWon = -1;
                result.computerWon = -1;
                return result;
            }
            
            // Update shot grid
            player->shotGrid[shotRow][shotCol] = result.playerHit ? HIT : MISS;
            
            // Check if we won
            result.playerWon = TwoPlayerDidWin(sockfd);
            if (result.playerWon < 0)
            {
                result.computerWon = -1;
                return result;
            }
        }
        else
        {
            result.playerWon = 0;
            result.playerHit = 0;
        }
    }
    
    return result;
}
