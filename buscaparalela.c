/*
   Trabalho: Programa Multithread com PThreads
   Parte: BUSCA SERIAL

   Grupo: Eduardo Souza Arrigoni, Felipe Soeiro Accioly, Isabeli Resende

*/
#define _CRT_SECURE_NO_WARNINGS 1
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#pragma comment(lib,"pthreadVC2.lib") // para compatibilidade futura com PThreads
#define HAVE_STRUCT_TIMESPEC

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>

#ifdef _WIN32
#include <windows.h>

/*
   Substitui  o do clock_gettime do Linux no Windows
   usando QueryPerformanceCounter para medir tempo com alta precis o.
*/
int clock_gettime(int dummy, struct timespec* ct) {
    static LARGE_INTEGER frequencia;
    static BOOL freqInicializada = FALSE;
    LARGE_INTEGER count;

    if (!freqInicializada) {
        QueryPerformanceFrequency(&frequencia);
        freqInicializada = TRUE;
    }

    QueryPerformanceCounter(&count);

    ct->tv_sec = (time_t)(count.QuadPart / frequencia.QuadPart);
    ct->tv_nsec = (long)((count.QuadPart % frequencia.QuadPart) * 1e9 / frequencia.QuadPart);
    return 0;
}

#define CLOCK_MONOTONIC 0
#endif

/* ===============================
   DEFINICOES PRINCIPAIS
   =============================== */
#define LINHAS_MATRIZ 10000  // n mero de linhas da matriz
#define COLUNAS_MATRIZ 10000 // n mero de colunas da matriz
#define SEED_ALEATORIA 12345  // semente fixa para gerar sempre a mesma matriz
#define MAX_ALEATORIO 31999  // limite dos n meros aleat rios
#define QUANT_THREADS 500
#define LINHA_MACRO_BLOCO 100
#define COLUNA_MACRO_BLOCO 100


   /* ===============================
         VARIAVEIS GLOBAIS
      =============================== */
static pthread_mutex_t mutexTotalPrimos = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutexMatrizGlobal = PTHREAD_MUTEX_INITIALIZER;
int** matriz_global = NULL;   // ponteiro para a matriz alocada dinamicamente
int linhas = LINHAS_MATRIZ;
int colunas = COLUNAS_MATRIZ;
int quantidadeTotalPrimos;
int posLinha = 0;
int posColuna = 0;
int quantidadeBlocos;
int proximoBloco;
/* ===============================
    PROTOTIPO DAS FUNCOES
   =============================== */
int** alocar_matriz(int l, int c);
void preencher_matriz(int** m, int l, int c);
void liberar_matriz(int** m);
int ehPrimo(int n);
double diferenca_tempo_seg(struct timespec inicio, struct timespec fim);
void buscaSerial(int** m, int l, int c);

/* ===============================
   ALOCACAO DINAMICA DE MATRIZ
   =============================== */

int** alocar_matriz(int l, int c) {
    if (l == 0 || c == 0) return NULL;

    int** m = malloc(l * sizeof(int*));

    for (int i = 0; i < l; i++) {
        m[i] = malloc(c * sizeof(int));
    }
    if (m == NULL) {
        fprintf(stderr, "Erro: falha ao alocar matriz (%zu x %zu)\n", l, c);
        return NULL;
    }
    return m;
}


/* ===============================
   PREENCHIMENTO DA MATRIZ
   =============================== */
void preencher_matriz(int** m, int l, int c) {
    srand(SEED_ALEATORIA); // semente fixa para sempre gerar a mesma matriz

    for (int i = 0; i < l; i++) {
        for (int j = 0; j < c; j++) {
            m[i][j] = rand() % (MAX_ALEATORIO + 1);
        }
    }
}

/* ===============================
   LIBERACAO DA MATRIZ
   =============================== */
void liberar_matriz(int** m) {
    if (m != NULL) free(m);
}

/* ===============================
   VERIFICACAO DE N MERO PRIMO
   =============================== */
int ehPrimo(int n) {
    if (n < 2) return 0;       // 0 = N o   primo
    if (n == 2) return 1;     // 1 =   primo
    if (n % 2 == 0) return 0;

    int limite = (int)sqrt(n); // c lculo fora do loop (sqrt calcula raiz quadrada)
    for (int d = 3; d <= limite; d += 2) { // testa apenas divisores  mpares
        if (n % d == 0) return 0;         // encontrou um divisor, n o   primo
    }
    return 1;
}

/* ===============================
   DIFERENCA DE TEMPO EM SEGUNDOS
   =============================== */
double diferenca_tempo_seg(struct timespec inicio, struct timespec fim) {
    double s = (double)(fim.tv_sec - inicio.tv_sec);            //segundos
    double ns = (double)(fim.tv_nsec - inicio.tv_nsec) * 1e-9; //nanosegundos em segundos
    return s + ns;  //tempo total em alta precis o
}

/* ===============================
   BUSCA SERIAL DE NUMEROS PRIMOS
   =============================== */
static void buscaSerial(int** m, size_t l, size_t c) {
    struct timespec t_inicio, t_fim;
    clock_gettime(CLOCK_MONOTONIC, &t_inicio); // marca in cio da contagem

    long long contador = 0;  // contador local

    int total = l * c;
    // Percorre toda a matriz e conta n meros primos
    for (int i = 0; i < l; i++) {
        for (int j = 0; j < c; j++) {
            if (ehPrimo(m[i][j])) contador++;

        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t_fim); // marca fim da contagem

    double tempo = diferenca_tempo_seg(t_inicio, t_fim);

    printf("*** BUSCA SERIAL ***\n");
    printf("Dimensoes: %zu x %zu (total %zu elementos)\n", l, c, total);
    printf("Total de primos encontrados: %lld\n", contador);
    printf("Tempo de execucao: %.6f segundos\n\n", tempo);
}

/* ========================================
   CADA THREAD PROCURA PRIMO EM SEUS BLOCOS
   ======================================== */
static void* procuraPrimoBloco() {

    long long contador = 0;
    int i;
    int j;
    int blocoAtual;
    int blocoLinha, blocoColuna;
    int inicioLinhaBloco;
    int fimLinhaBloco;
    int inicioColunaBloco;
    int fimColunaBloco;
    int blocosLargura = (colunas + COLUNA_MACRO_BLOCO - 1) / COLUNA_MACRO_BLOCO;
    
    while (1) {

        //Regiao critica
        //Designa qual bloco a thread deve pegar
        pthread_mutex_lock(&mutexMatrizGlobal);
        if (proximoBloco >= quantidadeBlocos) {

            pthread_mutex_unlock(&mutexMatrizGlobal);
            break;
        }

        blocoAtual = proximoBloco;
        proximoBloco++;
        pthread_mutex_unlock(&mutexMatrizGlobal);
        //Regiao critica
        
        // Localiza as coordenadas do bloco na matriz principal
        blocoLinha = blocoAtual / blocosLargura;
        blocoColuna = blocoAtual % blocosLargura;

        inicioLinhaBloco = blocoLinha * LINHA_MACRO_BLOCO;
        inicioColunaBloco = blocoColuna * COLUNA_MACRO_BLOCO;

        fimLinhaBloco = inicioLinhaBloco + LINHA_MACRO_BLOCO;
        fimColunaBloco = inicioColunaBloco + COLUNA_MACRO_BLOCO;

        //Corrige caso o bloco ultrapasse o limite da matriz principal
        if (fimLinhaBloco > LINHAS_MATRIZ) {
            fimLinhaBloco = LINHAS_MATRIZ;
        }
        if (fimColunaBloco > COLUNAS_MATRIZ) {
            fimColunaBloco = COLUNAS_MATRIZ;
        }
        
        //Conta os primos localmente
        contador = 0;
        for (i = inicioLinhaBloco; i < fimLinhaBloco; i++) {
            for (j = inicioColunaBloco; j < fimColunaBloco; j++) {
                if (ehPrimo(matriz_global[i][j])) contador++;
            }
        }

        //Regiao critica
        //Soma no contador global
        pthread_mutex_lock(&mutexTotalPrimos);

        quantidadeTotalPrimos += contador;

        pthread_mutex_unlock(&mutexTotalPrimos);
        //Regiao critica
    }
    
    
    
    
}

static void buscaParalela() {
    clock_t inicio, fim;
    inicio = clock();
    //Cria uma lista de threads
    pthread_t threads[QUANT_THREADS];

    //Faz o calculo de quantos bloco cabem em cada linha e coluna
    int blocosAltura = (linhas + LINHA_MACRO_BLOCO - 1) / LINHA_MACRO_BLOCO;
    int blocosLargura = (colunas + COLUNA_MACRO_BLOCO - 1) / COLUNA_MACRO_BLOCO;

   
    quantidadeBlocos = blocosAltura * blocosLargura;
    proximoBloco = 0;
    
    //Inicializa os Mutex
    pthread_mutex_init(&mutexMatrizGlobal, NULL);
    pthread_mutex_init(&mutexTotalPrimos, NULL);

    //Cria cada Thread
    for (int i = 0; i < QUANT_THREADS; i++) {
        
        pthread_create(&threads[i], NULL, procuraPrimoBloco, NULL);
    }
    //Aguarda a finalização de todas as Threads
    for (int i = 0; i < QUANT_THREADS; i++) {

        pthread_join(threads[i], NULL);    
    }
     

    fim = clock();
    double tempo = ((double)(fim - inicio)) / CLOCKS_PER_SEC;

    printf("\n*** BUSCA PARALELA ***\n");
    printf("Total de primos encontrados: %d\n", quantidadeTotalPrimos);
    printf("Tempo de execucao: %.6f segundos\n\n", tempo);
    

}
/* ================
   FUNCAO PRINCIPAL
   ================ */
int main() {
    printf("*** Trabalho: Busca de Primos ***\n");
    printf("Parametros:\n");
    printf("LINHAS_MATRIZ=%zu, COLUNAS_MATRIZ=%zu, SEED_ALEATORIA=%d, MAX_ALEATORIO=%d\n\n",
        linhas, colunas, SEED_ALEATORIA, MAX_ALEATORIO);

    //Aloca dinamicamento a matriz principal
    matriz_global = alocar_matriz(linhas, colunas);
    if (!matriz_global) {
        printf("Erro: falha ao alocar memoria da matriz.\n");
        return EXIT_FAILURE;
    }

    printf("Gerando matriz aleatoria...\n");
    preencher_matriz(matriz_global, linhas, colunas);
    printf("Matriz gerada com sucesso!\n\n");

    int opcao;
    printf("*** Menu inicial ***\n");
    printf("1) Busca serial.\n");
    printf("2) Busca paralela.\n");
    printf("0) Encerrar programa.\n");
    printf("Escolha uma opção acima:\t");
    scanf("%d", &opcao);
    while (opcao != 0) {

        switch (opcao) {
            case 1: 

                buscaSerial(matriz_global, linhas, colunas);
                break;
            case 2:

                buscaParalela();
                break;
            case 0: 

                liberar_matriz(matriz_global);
                matriz_global = NULL;
                return 0;

            default:

                printf("\nValor inválido, escolha novamente...\n");
        }
        printf("*** Menu inicial ***\n");
        printf("1) Busca serial.\n");
        printf("2) Busca paralela.\n");
        printf("0) Encerrar programa.\n");
        printf("Escolha uma opção acima:\t");
        scanf("%d", &opcao);
    }
    //Libera espaco e finaliza o programa
    liberar_matriz(matriz_global);
    matriz_global = NULL;

    printf("\nFim da execucao.\n");
    return EXIT_SUCCESS;
}