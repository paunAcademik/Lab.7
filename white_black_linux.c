#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define WHITE 0
#define BLACK 1

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond_white;
    pthread_cond_t cond_black;

    int active_white;
    int active_black;
    int waiting_white;
    int waiting_black;
    int turn; // WHITE sau BLACK
} ResourceController;

ResourceController ctrl;

void init_controller(ResourceController *c) {
    pthread_mutex_init(&c->mutex, NULL);
    pthread_cond_init(&c->cond_white, NULL);
    pthread_cond_init(&c->cond_black, NULL);
    c->active_white = 0;
    c->active_black = 0;
    c->waiting_white = 0;
    c->waiting_black = 0;
    c->turn = WHITE;
}

void destroy_controller(ResourceController *c) {
    pthread_mutex_destroy(&c->mutex);
    pthread_cond_destroy(&c->cond_white);
    pthread_cond_destroy(&c->cond_black);
}

void enter_white(ResourceController *c) {
    pthread_mutex_lock(&c->mutex);
    c->waiting_white++;
    while (c->active_black > 0 || (c->waiting_black > 0 && c->turn == BLACK)) {
        pthread_cond_wait(&c->cond_white, &c->mutex);
    }
    c->waiting_white--;
    c->active_white++;
    pthread_mutex_unlock(&c->mutex);
}

void exit_white(ResourceController *c) {
    pthread_mutex_lock(&c->mutex);
    c->active_white--;
    if (c->active_white == 0) {
        if (c->waiting_black > 0) {
            c->turn = BLACK;
            pthread_cond_broadcast(&c->cond_black);
        } else if (c->waiting_white > 0) {
            pthread_cond_broadcast(&c->cond_white);
        }
    }
    pthread_mutex_unlock(&c->mutex);
}

void enter_black(ResourceController *c) {
    pthread_mutex_lock(&c->mutex);
    c->waiting_black++;
    while (c->active_white > 0 || (c->waiting_white > 0 && c->turn == WHITE)) {
        pthread_cond_wait(&c->cond_black, &c->mutex);
    }
    c->waiting_black--;
    c->active_black++;
    pthread_mutex_unlock(&c->mutex);
}

void exit_black(ResourceController *c) {
    pthread_mutex_lock(&c->mutex);
    c->active_black--;
    if (c->active_black == 0) {
        if (c->waiting_white > 0) {
            c->turn = WHITE;
            pthread_cond_broadcast(&c->cond_white);
        } else if (c->waiting_black > 0) {
            pthread_cond_broadcast(&c->cond_black);
        }
    }
    pthread_mutex_unlock(&c->mutex);
}

void* thread_func(void* arg) {
    int id = ((int*)arg)[0];
    int color = ((int*)arg)[1];
    free(arg);

    if (color == WHITE) {
        enter_white(&ctrl);
        printf("[ALB %d] Foloseste resursa (Activi Albi: %d, Negri: %d)\n",
               id, ctrl.active_white, ctrl.active_black);
        usleep(200000); // Simulare lucru finit (200 ms)
        printf("[ALB %d] Elibereaza resursa\n", id);
        exit_white(&ctrl);
    } else {
        enter_black(&ctrl);
        printf("[NEGRU %d] Foloseste resursa (Activi Albi: %d, Negri: %d)\n",
               id, ctrl.active_white, ctrl.active_black);
        usleep(200000); // Simulare lucru finit (200 ms)
        printf("[NEGRU %d] Elibereaza resursa\n", id);
        exit_black(&ctrl);
    }
    return NULL;
}

int main() {
    init_controller(&ctrl);
    pthread_t threads[12];
    int pattern[12] = {WHITE, WHITE, BLACK, WHITE, BLACK, BLACK, WHITE, WHITE, BLACK, WHITE, BLACK, WHITE};

    for (int i = 0; i < 12; i++) {
        int* args = malloc(2 * sizeof(int));
        args[0] = i + 1;
        args[1] = pattern[i];
        pthread_create(&threads[i], NULL, thread_func, args);
        usleep(30000); // Sosiri intercalate
    }

    for (int i = 0; i < 12; i++) {
        pthread_join(threads[i], NULL);
    }

    destroy_controller(&ctrl);
    return 0;
}
