#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define WHITE 0
#define BLACK 1

typedef struct {
    CRITICAL_SECTION cs;
    CONDITION_VARIABLE cond_white;
    CONDITION_VARIABLE cond_black;

    int active_white;
    int active_black;
    int waiting_white;
    int waiting_black;
    int turn; // WHITE sau BLACK
} ResourceController;

ResourceController ctrl;

void init_controller(ResourceController *c) {
    InitializeCriticalSection(&c->cs);
    InitializeConditionVariable(&c->cond_white);
    InitializeConditionVariable(&c->cond_black);
    c->active_white = 0;
    c->active_black = 0;
    c->waiting_white = 0;
    c->waiting_black = 0;
    c->turn = WHITE;
}

void destroy_controller(ResourceController *c) {
    DeleteCriticalSection(&c->cs);
}

void enter_white(ResourceController *c) {
    EnterCriticalSection(&c->cs);
    c->waiting_white++;
    while (c->active_black > 0 || (c->waiting_black > 0 && c->turn == BLACK)) {
        SleepConditionVariableCS(&c->cond_white, &c->cs, INFINITE);
    }
    c->waiting_white--;
    c->active_white++;
    LeaveCriticalSection(&c->cs);
}

void exit_white(ResourceController *c) {
    EnterCriticalSection(&c->cs);
    c->active_white--;
    if (c->active_white == 0) {
        if (c->waiting_black > 0) {
            c->turn = BLACK;
            WakeAllConditionVariable(&c->cond_black);
        } else if (c->waiting_white > 0) {
            WakeAllConditionVariable(&c->cond_white);
        }
    }
    LeaveCriticalSection(&c->cs);
}

void enter_black(ResourceController *c) {
    EnterCriticalSection(&c->cs);
    c->waiting_black++;
    while (c->active_white > 0 || (c->waiting_white > 0 && c->turn == WHITE)) {
        SleepConditionVariableCS(&c->cond_black, &c->cs, INFINITE);
    }
    c->waiting_black--;
    c->active_black++;
    LeaveCriticalSection(&c->cs);
}

void exit_black(ResourceController *c) {
    EnterCriticalSection(&c->cs);
    c->active_black--;
    if (c->active_black == 0) {
        if (c->waiting_white > 0) {
            c->turn = WHITE;
            WakeAllConditionVariable(&c->cond_white);
        } else if (c->waiting_black > 0) {
            WakeAllConditionVariable(&c->cond_black);
        }
    }
    LeaveCriticalSection(&c->cs);
}

DWORD WINAPI thread_func(LPVOID arg) {
    int id = ((int*)arg)[0];
    int color = ((int*)arg)[1];
    free(arg);

    if (color == WHITE) {
        enter_white(&ctrl);
        printf("[ALB %d] Foloseste resursa (Activi Albi: %d, Negri: %d)\n",
               id, ctrl.active_white, ctrl.active_black);
        Sleep(200); // Simulare lucru finit (200 ms)
        printf("[ALB %d] Elibereaza resursa\n", id);
        exit_white(&ctrl);
    } else {
        enter_black(&ctrl);
        printf("[NEGRU %d] Foloseste resursa (Activi Albi: %d, Negri: %d)\n",
               id, ctrl.active_white, ctrl.active_black);
        Sleep(200); // Simulare lucru finit (200 ms)
        printf("[NEGRU %d] Elibereaza resursa\n", id);
        exit_black(&ctrl);
    }
    return 0;
}

int main() {
    init_controller(&ctrl);
    HANDLE threads[12];
    int pattern[12] = {WHITE, WHITE, BLACK, WHITE, BLACK, BLACK, WHITE, WHITE, BLACK, WHITE, BLACK, WHITE};

    for (int i = 0; i < 12; i++) {
        int* args = (int*)malloc(2 * sizeof(int));
        args[0] = i + 1;
        args[1] = pattern[i];
        threads[i] = CreateThread(NULL, 0, thread_func, args, 0, NULL);
        Sleep(30); // Sosiri intercalate
    }

    WaitForMultipleObjects(12, threads, TRUE, INFINITE);

    for (int i = 0; i < 12; i++) {
        CloseHandle(threads[i]);
    }

    destroy_controller(&ctrl);
    return 0;
}