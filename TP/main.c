// ejemplo.c
#include <stdio.h>
#include <pthread.h>

#define VUELTAS 100000

static long contador = 0;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

// Incremento protegido
void* worker_con_mutex(void* arg) {
    for (int i = 0; i < VUELTAS; ++i) {
        pthread_mutex_lock(&mtx);
        contador++;              // sección crítica
        pthread_mutex_unlock(&mtx);
    }
    return NULL;
}

// Incremento sin protección (para comparar)
void* worker_sin_mutex(void* arg) {
    for (int i = 0; i < VUELTAS; ++i) {
        contador++;              // condición de carrera
    }
    return NULL;
}

int main(void) {
    pthread_t a, b;

    // 1) Con mutex
    contador = 0;
    pthread_create(&a, NULL, worker_con_mutex, NULL);
    pthread_create(&b, NULL, worker_con_mutex, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("Con mutex: contador=%ld (esperado=%d)\n", contador, 2 * VUELTAS);

    // 2) Sin mutex
    contador = 0;
    pthread_create(&a, NULL, worker_sin_mutex, NULL);
    pthread_create(&b, NULL, worker_sin_mutex, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("Sin mutex: contador=%ld (normalmente < %d)\n", contador, 2 * VUELTAS);

    pthread_mutex_destroy(&mtx);
    return 0;
}