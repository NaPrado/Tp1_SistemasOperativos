#ifndef STRUCTURES_H
#define STRUCTURES_H
#include <stdbool.h>
#include <sys/wait.h>
#include <semaphore.h>
typedef struct { 
    char name_player[16]; // Nombre del jugador 
    unsigned int points; // Puntaje 
    unsigned int cant_invalid_movements; // Cantidad de solicitudes de movimientos inválidas realizadas 
    unsigned int cant_valid_movements; // Cantidad de solicitudes de movimientos válidas realizadas 
    unsigned short x, y; // Coordenadas x e y en el tablero 
    pid_t pid; // Identificador de proceso 
    bool can_move; // Indica si el jugador tiene movimientos válidos disponibles 
  } playerStatus;

typedef struct { 
  unsigned short width; // Ancho del tablero 
  unsigned short heigth; // Alto del tablero 
  unsigned int cant_players; // Cantidad de jugadores 
  playerStatus players[9]; // Lista de jugadores  
  bool can_end; // Indica si el juego se ha terminado 
  int board[]; // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1  
} gameStatus;

//asignar nombres
typedef struct { 
    sem_t show_needed; // Se usa para indicarle a la vista que hay cambios por imprimir 
    sem_t show_done; // Se usa para indicarle al master que la vista terminó de imprimir 
    sem_t C; // Mutex para evitar inanición del master al acceder al estado 
    sem_t D; // Mutex para el estado del juego 
    sem_t E; // Mutex para la siguiente variable 
    unsigned int player_reading_status; // Cantidad de jugadores leyendo el estado 
} semaphoresStatus;
#endif