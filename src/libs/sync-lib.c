#include "sync-lib.h"
#include "structures.h"

void set_init_semaphores(Tgame_sync * game_sync) {
    sem_init(&game_sync->show_needed, 1, 0);
    sem_init(&game_sync->show_done, 1, 0);
    sem_init(&game_sync->master_mutex, 1, 1);
    sem_init(&game_sync->game_state_mutex, 1, 1);
    sem_init(&game_sync->player_read_count_mutex, 1, 1);
    game_sync->player_reading_status = 0;
}

void set_master_writing(Tgame_sync * game_sync) {
    sem_wait(&game_sync->master_mutex);
    sem_wait(&game_sync->game_state_mutex);
    sem_post(&game_sync->master_mutex);
}

void unset_master_writing(Tgame_sync * game_sync) {
    sem_post(&game_sync->game_state_mutex);
}

void set_player_reading(Tgame_sync * game_sync) {
    sem_wait(&game_sync->master_mutex);
    sem_post(&game_sync->master_mutex);
    sem_wait(&game_sync->player_read_count_mutex); // espero a modificar variable
    if (game_sync->player_reading_status++ == 0) 
        sem_wait(&game_sync->game_state_mutex); // espero a que writer libere
    sem_post(&game_sync->player_read_count_mutex); // dejo modificar variable
}

void unset_player_reading(Tgame_sync * game_sync) {
    sem_wait(&game_sync->player_read_count_mutex); // espero a modificar variable
    if (game_sync->player_reading_status-- == 1)
        sem_post(&game_sync->game_state_mutex); // dejo al writer
    sem_post(&game_sync->player_read_count_mutex); // dejo modificar variable
}

void destroy_semaphores(Tgame_sync * game_sync) {
    sem_destroy(&game_sync->show_needed);
    sem_destroy(&game_sync->show_done);
    sem_destroy(&game_sync->master_mutex);
    sem_destroy(&game_sync->game_state_mutex);
    sem_destroy(&game_sync->player_read_count_mutex);
}
