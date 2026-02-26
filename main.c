#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>

#define FILOSOFOS_POR_DEFECTO 5
#define DURACION_SEGUNDOS_POR_DEFECTO 15



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

my_mutex* palillos;

// Estados
// 0 = Pensando
// 1 = HAMBRIENTO
// 2 = Comiendo

int* estado;
int num_filosofos = FILOSOFOS_POR_DEFECTO;
int duracion_segundos = DURACION_SEGUNDOS_POR_DEFECTO;
my_mutex print_lock;
volatile int ejecutando = 1;
time_t inicio_programa;

void pensar() {
    for (volatile long i = 0; i < 450000000; i++);
}

const char* estado_a_texto(int e) {
    if (e == 0) return "PENSANDO";
    if (e == 1) return "HAMBRIENTO";
    return "COMIENDO";
}

void imprimir_resumen_estados() {
    printf("[RESUMEN] ");
    for (int i = 0; i < num_filosofos; i++) {
        printf("F%d=%s", i, estado_a_texto(estado[i]));
        if (i < num_filosofos - 1) {
            printf(" | ");
        }
    }
    printf("\n");
}

void log_evento(int id, const char* evento) {

    my_mutex_lock(&print_lock);

    long transcurrido = (long)(time(NULL) - inicio_programa);
    printf("[t=%2lds] Filosofo %d -> %s\n", transcurrido, id, evento);
    imprimir_resumen_estados();
    printf("--------------------------------------------------\n");

    my_mutex_unlock(&print_lock);
}


void* filosofo(void* arg) {

    int id = *(int*)arg;

    int izquierda = id;
    int derecha = (id + 1) % num_filosofos;

    // Orden total: primero el menor índice
    int primero = (izquierda < derecha) ? izquierda : derecha;
    int segundo = (izquierda < derecha) ? derecha : izquierda;

    while (ejecutando) {

        // Pensando
        estado[id] = 0;
        log_evento(id, "PENSANDO");
        pensar();

        if (!ejecutando) {
            break;
        }

        // Hambriento
        estado[id] = 1;
        log_evento(id, "HAMBRIENTO (intenta tomar palillos)");

        // Tomar palillos
        my_mutex_lock(&palillos[primero]);
        log_evento(id, "TOMO primer palillo");
        my_mutex_lock(&palillos[segundo]);
        log_evento(id, "TOMO segundo palillo");

        if (!ejecutando) {
            my_mutex_unlock(&palillos[segundo]);
            my_mutex_unlock(&palillos[primero]);
            break;
        }

        // Comiendo
        estado[id] = 2;
        log_evento(id, "COMIENDO");
        pensar();

        // Soltar palillos
        my_mutex_unlock(&palillos[segundo]);
        my_mutex_unlock(&palillos[primero]);
        log_evento(id, "SUELTA palillos");
    }

    estado[id] = 0;
    log_evento(id, "TERMINA ejecucion del hilo");

    return NULL;
}


int main(int argc, char* argv[]) {

    if (argc >= 2) {
        int valor = atoi(argv[1]);
        if (valor > 0) {
            duracion_segundos = valor;
        }
    }

    if (argc >= 3) {
        int valor = atoi(argv[2]);
        if (valor >= 2) {
            num_filosofos = valor;
        }
    }

    if (argc > 3) {
        printf("Uso: %s [duracion_segundos] [cantidad_filosofos]\n", argv[0]);
        printf("Ejemplo: %s 20 7\n", argv[0]);
        return 1;
    }

    palillos = (my_mutex*)malloc(sizeof(my_mutex) * num_filosofos);
    estado = (int*)malloc(sizeof(int) * num_filosofos);

    if (palillos == NULL || estado == NULL) {
        printf("Error: no se pudo reservar memoria para la simulacion.\n");
        free(palillos);
        free(estado);
        return 1;
    }

    pthread_t* hilos = (pthread_t*)malloc(sizeof(pthread_t) * num_filosofos);
    int* ids = (int*)malloc(sizeof(int) * num_filosofos);

    if (hilos == NULL || ids == NULL) {
        printf("Error: no se pudo reservar memoria para hilos.\n");
        free(palillos);
        free(estado);
        free(hilos);
        free(ids);
        return 1;
    }

    // Inicializar palillos
    for (int i = 0; i < num_filosofos; i++) {
        my_mutex_init(&palillos[i]);
    }

    // Inicializar mutex de impresión
    my_mutex_init(&print_lock);

    // Inicializar estados
    for (int i = 0; i < num_filosofos; i++) {
        estado[i] = 0;
    }

    inicio_programa = time(NULL);

    printf("Iniciando simulacion de filosofos por %d segundos...\n", duracion_segundos);
    printf("Cantidad de filosofos: %d\n", num_filosofos);
    printf("Estados posibles: PENSANDO | HAMBRIENTO | COMIENDO\n");
    printf("Uso: %s [duracion_segundos] [cantidad_filosofos]\n", argv[0]);
    printf("==================================================\n");

    // Crear hilos
    for (int i = 0; i < num_filosofos; i++) {
        ids[i] = i;
        pthread_create(&hilos[i], NULL, filosofo, &ids[i]);
    }

    // Esperar hasta cumplir la duracion configurada
    while ((time(NULL) - inicio_programa) < duracion_segundos) {
        for (volatile long i = 0; i < 15000000; i++);
    }

    ejecutando = 0;

    my_mutex_lock(&print_lock);
    printf("\nTiempo cumplido (%d s). Cerrando simulacion...\n", duracion_segundos);
    printf("==================================================\n");
    my_mutex_unlock(&print_lock);

    // Esperar hilos
    for (int i = 0; i < num_filosofos; i++) {
        pthread_join(hilos[i], NULL);
    }

    printf("Simulacion finalizada correctamente.\n");

    free(hilos);
    free(ids);
    free(palillos);
    free(estado);

    return 0;
}