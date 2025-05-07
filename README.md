# Trabalho_SO

Para compilar e executar o código e necessário alterar o caminho do arquivo devices.csv, tanto de entrada como de saída, posterior executar o comando no terminal.

Entrada:

FILE* arquivo = fopen("caminho_do_csv", "r");
    if (!arquivo) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

Saída:

FILE* output = fopen("saida_do_csv", "w");
    fprintf(output, "device;ano-mes;sensor;valor_maximo;valor_medio;valor_minimo\n");
    for (int i = 0; i < stats_index; i++) {
        fprintf(output, "%s;%s;%s;%.2f;%.2f;%.2f\n",
            stats[i].device, stats[i].ano_mes, stats[i].sensor,
            stats[i].valor_maximo, stats[i].valor_medio, stats[i].valor_minimo);
    }
    fclose(output);


Comando para compilar arquivo:

gcc -pthread main.c -o main

Executar o arquivo compilado:

./main

O CSV é carregado para o programa utilizando a função descrita acima na "Entrada", ele vai abrir o arquivo, ignorar a primeira linha, o cabeçalho, posterior ele lê cada linha subsequente, e separa utilizando o delimitador |. Os campos são armazenados no vetor na estrutura DadosSensor,  filtrando somente linhas com data a partir de Março de 2024, armazenando todos os regristros válidos, até o limite de MAX_STATUS, que foi fixado em 7000000, por conta da quantidade de linhas do CSV.

A carga é distribuida entre as threads de forma igualitaria, é verificado a quantidade de núcleos disponíveis na CPU utilizando o get_nprocs (), depois é dividos todos os regristros uniformemente. 

Cada thread é responsável por processar um subconjunto dos dados lidos do arquivo CSV, realizando a análise estatística por sensor, por mês e por dispositivo. Durante esse processamento, a thread extrai o ano e mês da data de cada registro e calcula os valores mínimo, máximo e a média incremental para cada um dos sensores (temperatura, umidade, luminosidade, ruído, eCO2 e eTVOC), agrupando-os por dispositivo e por mês. Para garantir consistência ao acessar e atualizar a estrutura compartilhada de estatísticas (StatusMes), a thread utiliza mutexes, evitando condições de corrida durante a leitura, modificação ou inserção de novos registros estatísticos. Esse processamento paralelo permite que múltiplas threads atualizem os dados agregados de forma segura e eficiente.

As threads não escrevem diretamente no arquivo, após todas as threads terminarem, a thread principal ordena os vetor com os dados usando qsort, por ano_mes e depois device, mantendo uma ordem de leitura, e depois a função de saída escreve os resultados em um novo arquivo. 

As threads criadas são executadas em modo núcleo, aproveitando o paralelismo. 

Um possível problema de ineficiência no programa é o uso intensivo de mutexes para acessar a estrutura compartilhada de estatísticas, o que pode gerar contenção entre threads e limitar o ganho de desempenho do paralelismo. Além disso, a busca linear dentro da estrutura StatusMes para cada sensor pode se tornar lenta à medida que o número de registros cresce, tornando o tempo de processamento menos eficiente.


