# Trabalho_SO

**Compilação e Execução**

Antes de compilar e executar o código, é necessário alterar os caminhos de entrada e saída do arquivo devices.csv no código-fonte:

Entrada:
```
FILE* arquivo = fopen("_caminho_do_csv_", "r");
if (!arquivo) {
    perror("Erro ao abrir o arquivo");
    return 1;
}
```
Saída:
```
FILE* output = fopen("_saida_do_csv_", "w");
fprintf(output, "device;ano-mes;sensor;valor_maximo;valor_medio;valor_minimo\n");
for (int i = 0; i < stats_index; i++) {
    fprintf(output, "%s;%s;%s;%.2f;%.2f;%.2f\n",
        stats[i].device, stats[i].ano_mes, stats[i].sensor,
        stats[i].valor_maximo, stats[i].valor_medio, stats[i].valor_minimo);
}
fclose(output);
```
Comando para Compilação:
```
gcc -pthread main.c -o main
```
Execução:
```
./main
```
**Leitura e Carregamento do CSV**

O programa abre o arquivo CSV e ignora a primeira linha (cabeçalho). Em seguida, ele lê cada linha subsequente utilizando o delimitador |, armazenando os campos relevantes na estrutura DadosSensor. São filtradas apenas as linhas com datas a partir de março de 2024 ("2024-03"). O vetor comporta até MAX_STATUS registros, que foi definido como 7.000.000.

**Distribuição de Carga entre Threads**

A carga de trabalho é dividida uniformemente entre as threads com base no número de núcleos disponíveis na CPU, obtido via get_nprocs(). Cada thread recebe um intervalo distinto de registros para processar, sem considerar critérios como período ou dispositivo — a divisão é puramente baseada em quantidade.

**Processamento e Análise pelas Threads**

Cada thread processa seu subconjunto de registros e realiza a análise estatística por sensor, dispositivo e mês. A thread extrai o ano e mês da data e calcula os valores mínimo, máximo e média incremental para os sensores: temperatura, umidade, luminosidade, ruído, eCO2 e eTVOC.
Para evitar condições de corrida, o acesso à estrutura StatusMes, onde os resultados são agregados, é protegido com mutexes. As threads não escrevem no arquivo final — apenas atualizam os dados estatísticos de forma segura e paralela.

**Geração do CSV de Saída**

Após todas as threads finalizarem, a thread principal ordena o vetor StatusMes usando qsort, priorizando a ordenação por ano_mes e depois por device. Em seguida, escreve os resultados no arquivo de saída em formato CSV.

**Execução das Threads**

As threads criadas são executadas em modo núcleo, aproveitando o paralelismo proporcionado pelos múltiplos núcleos do processador.

**Possíveis Ineficiências**

Uma possível ineficiência do programa é o uso intensivo de mutexes na atualização da estrutura compartilhada, o que pode causar contenção entre threads e reduzir os ganhos do paralelismo. Além disso, a busca linear por combinações de device, ano_mes e sensor dentro de StatusMes pode tornar-se lenta à medida que o número de registros aumenta, impactando o desempenho geral do processamento.


