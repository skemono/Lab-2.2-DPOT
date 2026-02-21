#include <stdio.h>
#include <pthread.h>

#define N 5

// Implementación manual de mutex (spinlock)

typedef struct {
    volatile int locked;
} my_mutex;

void my_mutex_init(my_mutex *m) {
    m->locked = 0;
}

void my_mutex_lock(my_mutex *m) {
    while (__sync_lock_test_and_set(&m->locked, 1)) {
        while (m->locked);
    }
}

void my_mutex_unlock(my_mutex *m) {
    __sync_lock_release(&m->locked);
}

my_mutex palillos[N];

// Estados
// 0 = Pensando
// 1 = Tiene Uno
// 2 = Comiendo

int estado[N];
my_mutex print_lock;

void pensar() {
    for (volatile long i = 0; i < 2100000000; i++);
}


void mostrar_estado() {

    my_mutex_lock(&print_lock);

    printf("\033[H\033[J");  // Limpiar pantalla

    printf("=====================================\n");
    printf("        MESA DE FILOSOFOS\n");
    printf("=====================================\n\n");
    printf("Estados:\n");
    printf("0 = PENSANDO\n");
    printf("1 = TIENE UNO\n");
    printf("2 = COMIENDO\n");
    printf("=====================================\n\n");

    printf("              (%d)\n", estado[0]);
    printf("        (%d)           (%d)\n",
           estado[4], estado[1]);
    printf("              MESA\n");
    printf("        (%d)           (%d)\n",
           estado[3], estado[2]);

    printf("\n-------------------------------------\n");

    for (int i = 0; i < N; i++) {
        printf("Filosofo %d: ", i);

        if (estado[i] == 0)
            printf("PENSANDO");
        else if (estado[i] == 1)
            printf("TIENE UNO");
        else if (estado[i] == 2)
            printf("COMIENDO");

        printf("\n");
    }

    printf("=====================================\n");

    my_mutex_unlock(&print_lock);
}


void* filosofo(void* arg) {

    int id = *(int*)arg;

    int izquierda = id;
    int derecha = (id + 1) % N;

    // Orden total: primero el menor índice
    int primero = (izquierda < derecha) ? izquierda : derecha;
    int segundo = (izquierda < derecha) ? derecha : izquierda;

    while (1) {

        // Pensando
        estado[id] = 0;
        mostrar_estado();
        pensar();

        // Hambriento
        estado[id] = 1;
        mostrar_estado();

        // Tomar palillos
        my_mutex_lock(&palillos[primero]);
        my_mutex_lock(&palillos[segundo]);

        // Comiendo
        estado[id] = 2;
        mostrar_estado();
        pensar();

        // Soltar palillos
        my_mutex_unlock(&palillos[segundo]);
        my_mutex_unlock(&palillos[primero]);
    }

    return NULL;
}


int main() {

    pthread_t hilos[N];
    int ids[N];

    // Inicializar palillos
    for (int i = 0; i < N; i++) {
        my_mutex_init(&palillos[i]);
    }

    // Inicializar mutex de impresión
    my_mutex_init(&print_lock);

    // Inicializar estados
    for (int i = 0; i < N; i++) {
        estado[i] = 0;
    }

    // Crear hilos
    for (int i = 0; i < N; i++) {
        ids[i] = i;
        pthread_create(&hilos[i], NULL, filosofo, &ids[i]);
    }

    // Esperar hilos (infinito)
    for (int i = 0; i < N; i++) {
        pthread_join(hilos[i], NULL);
    }

    return 0;
}