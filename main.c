#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/sysinfo.h>

#define MAX_STATUS 7000000

typedef struct {
    char device[50];
    char data[20];
    float temperatura;
    float umidade;
    float luminosidade;
    float ruido;
    float eco2;
    float etvoc;
} DadosSensor;

typedef struct {
    char device[50];
    char ano_mes[8];
    char sensor[20];
    float valor_maximo;
    float valor_medio;
    float valor_minimo;
    int count;
} StatusMes;

typedef struct {
    DadosSensor* data;
    int start;
    int end;
    StatusMes* stats;
    int* stats_index;
    pthread_mutex_t* stats_mutex;
    pthread_mutex_t* index_mutex;
} ThreadArgs;

void get_ano_mes(const char* data, char* ano_mes) {
    strncpy(ano_mes, data, 7);
    ano_mes[7] = '\0';
}

void* process_data(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;

    for (int i = args->start; i < args->end; i++) {
        char ano_mes[8];
        get_ano_mes(args->data[i].data, ano_mes);

        float valores[] = {
            args->data[i].temperatura,
            args->data[i].umidade,
            args->data[i].luminosidade,
            args->data[i].ruido,
            args->data[i].eco2,
            args->data[i].etvoc
        };

        const char* sensor_names[] = {
            "temperatura", "umidade", "luminosidade",
            "ruido", "eco2", "etvoc"
        };

        for (int j = 0; j < 6; j++) {
            int found = 0;

            pthread_mutex_lock(args->stats_mutex);
            for (int k = 0; k < *(args->stats_index); k++) {
                if (strcmp(args->stats[k].device, args->data[i].device) == 0 &&
                    strcmp(args->stats[k].ano_mes, ano_mes) == 0 &&
                    strcmp(args->stats[k].sensor, sensor_names[j]) == 0) {

                    if (valores[j] > args->stats[k].valor_maximo)
                        args->stats[k].valor_maximo = valores[j];
                    if (valores[j] < args->stats[k].valor_minimo)
                        args->stats[k].valor_minimo = valores[j];

                    args->stats[k].valor_medio =
                        (args->stats[k].valor_medio * args->stats[k].count + valores[j]) /
                        (args->stats[k].count + 1);

                    args->stats[k].count++;
                    found = 1;
                    break;
                }
            }
            pthread_mutex_unlock(args->stats_mutex);

            if (!found) {
                pthread_mutex_lock(args->index_mutex);
                int idx = *(args->stats_index);
                if (idx < MAX_STATUS) {
                    strncpy(args->stats[idx].device, args->data[i].device, 49);
                    strncpy(args->stats[idx].ano_mes, ano_mes, 7);
                    strncpy(args->stats[idx].sensor, sensor_names[j], 19);

                    args->stats[idx].valor_maximo = valores[j];
                    args->stats[idx].valor_minimo = valores[j];
                    args->stats[idx].valor_medio = valores[j];
                    args->stats[idx].count = 1;

                    (*(args->stats_index))++;
                }
                pthread_mutex_unlock(args->index_mutex);
            }
        }
    }
    return NULL;
}

int comparar_statusmes(const void* a, const void* b) {
    const StatusMes* s1 = (const StatusMes*)a;
    const StatusMes* s2 = (const StatusMes*)b;

    int cmp = strcmp(s1->ano_mes, s2->ano_mes);
    if (cmp != 0) return cmp;

    // Se ano_mes for igual, ordena por device
    return strcmp(s1->device, s2->device);
}

int main() {
    int num_threads = get_nprocs();
    printf("Utilizando %d threads\n", num_threads);

    FILE* arquivo = fopen("caminho_do_csv", "r");
    if (!arquivo) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    DadosSensor* data = malloc(sizeof(DadosSensor) * MAX_STATUS);
    char line[256];
    int count = 0;

    fgets(line, sizeof(line), arquivo); // cabeçalho

    while (fgets(line, sizeof(line), arquivo)) {
        if (count >= MAX_STATUS) {
            fprintf(stderr, "Número máximo de registros (%d) atingido.\n", MAX_STATUS);
            break;
        }
    
        char* token = strtok(line, "|"); // id (ignorado)
        token = strtok(NULL, "|"); // device
        if (!token || strlen(token) == 0) continue;
        strncpy(data[count].device, token, sizeof(data[count].device) - 1);
    
        token = strtok(NULL, "|"); // contagem (ignorado)
        token = strtok(NULL, "|"); // data
        if (!token || strlen(token) == 0) continue;

        strncpy(data[count].data, token, sizeof(data[count].data));
        data[count].data[sizeof(data[count].data) - 1] = '\0'; // Garante fim da string

        // Verificar se a data é >= "2024-03"
        if (strncmp(data[count].data, "2024-03", 7) < 0) continue;

    
        token = strtok(NULL, "|"); data[count].temperatura = (token) ? atof(token) : 0.0;
        token = strtok(NULL, "|"); data[count].umidade = (token) ? atof(token) : 0.0;
        token = strtok(NULL, "|"); data[count].luminosidade = (token) ? atof(token) : 0.0;
        token = strtok(NULL, "|"); data[count].ruido = (token) ? atof(token) : 0.0;
        token = strtok(NULL, "|"); data[count].eco2 = (token) ? atof(token) : 0.0;
        token = strtok(NULL, "|"); data[count].etvoc = (token) ? atof(token) : 0.0;
    
        count++;
    }
    
    printf("Total de registros lidos: %d\n", count);


    fclose(arquivo);


    StatusMes* stats = calloc(MAX_STATUS, sizeof(StatusMes));
    int stats_index = 0;
    pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t index_mutex = PTHREAD_MUTEX_INITIALIZER;

    pthread_t threads[num_threads];
    ThreadArgs args[num_threads];
    int chunk_size = count / num_threads;

    for (int i = 0; i < num_threads; i++) {
        args[i].data = data;
        args[i].start = i * chunk_size;
        args[i].end = (i == num_threads - 1) ? count : (i + 1) * chunk_size;
        args[i].stats = stats;
        args[i].stats_index = &stats_index;
        args[i].stats_mutex = &stats_mutex;
        args[i].index_mutex = &index_mutex;

        pthread_create(&threads[i], NULL, process_data, &args[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Ordenar resultados
    qsort(stats, stats_index, sizeof(StatusMes), comparar_statusmes);


    FILE* output = fopen("saida_do_csv", "w");
    fprintf(output, "device;ano-mes;sensor;valor_maximo;valor_medio;valor_minimo\n");
    for (int i = 0; i < stats_index; i++) {
        fprintf(output, "%s;%s;%s;%.2f;%.2f;%.2f\n",
            stats[i].device, stats[i].ano_mes, stats[i].sensor,
            stats[i].valor_maximo, stats[i].valor_medio, stats[i].valor_minimo);
    }
    fclose(output);

    free(data);
    free(stats);
    pthread_mutex_destroy(&stats_mutex);
    pthread_mutex_destroy(&index_mutex);

    return 0;
}
