#include <sys/wait.h>
typedef struct { 
    char namePlayer[16]; // Nombre del jugador 
    unsigned int points; // Puntaje 
    unsigned int cantInvalidMovements; // Cantidad de solicitudes de movimientos inválidas realizadas 
    unsigned int cantValidMovements; // Cantidad de solicitudes de movimientos válidas realizadas 
    unsigned short x, y; // Coordenadas x e y en el tablero 
    pid_t pid; // Identificador de proceso 
    bool canMove; // Indica si el jugador tiene movimientos válidos disponibles 
  } playerStatus;

typedef struct { 
  unsigned short width; // Ancho del tablero 
  unsigned short heigth; // Alto del tablero 
  unsigned int cantPlayers; // Cantidad de jugadores 
  playerStatus players[9]; // Lista de jugadores  
  bool canEnd; // Indica si el juego se ha terminado 
  int board[]; // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1  
} gameStatus;