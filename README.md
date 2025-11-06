# MultiThread — Paralelismo de Matrizes (pthreads)

Este repositório contém um programa em C que demonstra leitura e processamento paralelo de uma matriz utilizando a biblioteca pthreads. O objetivo é mostrar como dividir o trabalho entre threads para percorrer/ler uma matriz em paralelo aproveitando múltiplos núcleos.

## Conceito
- O programa gera uma matriz MxN de números aleatórios.
- Cria T threads para buscar números primos presentes na matriz.
- Cada thread é responsável por blocos da matriz(submatrizes), realiza a busca sobre esses elementos e soma na variável global o número total de primos.
- O código usa mutexes para controlar a alocação de blocos e controle da quantidade de primos encontrados.

**Exemplo 1 de região crítica:**
<pre><code class="language-c">
pthread_mutex_lock(&mutexMatrizGlobal);
  if (proximoBloco >= quantidadeBlocos) {
      pthread_mutex_unlock(&mutexMatrizGlobal);
      break;
  }
  blocoAtual = proximoBloco;
  proximoBloco++;
  pthread_mutex_unlock(&mutexMatrizGlobal);
</code></pre>
  
**Exemplo 2 de região crítica:**
<pre><code class="language-c">
pthread_mutex_lock(&mutexTotalPrimos);
  
  quantidadeTotalPrimos += contador;
  
  pthread_mutex_unlock(&mutexTotalPrimos);
</code></pre>
  
## Testes interessantes
- Teste com matrizes pequenas e poucas Threads.
- Compare resultados com a busca sequencial.
- Teste escalabilidade aumentando número de threads e observando tempo de execução (speedup).
- Teste para a quantidade núcleos físicos e lógicos de seu computador.
- Aumente também o tamanho da matriz pincipal e de seus blocos.
    Dica: Uma matriz de tamanho razoável começaria a partir de 10000x10000.
- Tente aumentar para algumas centenas o número de Threads, fixando os tamanhos.

**Divirta-se e seja criativo**
