# Edge Engine: Motor Orquestrador de Processos Concorrentes

**Projeto Final da disciplina de Sistemas Operacionais (DCC403) - UFRR** **Tema 2:** Desenvolvimento de um Motor Orquestrador Nativo em C para Processamento Concorrente e Pipelines de Dados em Ambientes de Borda (Edge Computing).

---

## Sobre o Projeto

Este projeto consiste na implementação de um interpretador de linha de comando (*shell* / orquestrador) nativo para sistemas Unix/POSIX. Ele foi construído estritamente em linguagem C (padrão POSIX.1-2008), focado em eficiência de memória e processamento local para dispositivos de borda (*Edge Computing*), evitando o uso de abstrações pesadas.

O orquestrador gerencia ativamente o ciclo de vida de processos, redirecionamento em memória compartilhada e o tratamento assíncrono de sinais para garantir que os recursos do sistema operacional sejam preservados.

## Principais Funcionalidades

* **Isolamento de Memória:** Delegação de tarefas utilizando as chamadas de sistema `fork()` e `execvp()`.
* **Comunicação Interprocessos (IPC):** Encadeamento de fluxos de dados em memória via *pipes* unidirecionais (`pipe()`) e duplicação de descritores (`dup2()`), evitando I/O excessivo em disco.
* **Processamento Assíncrono (Anti-Zombie):** Suporte a tarefas em *background* (operador `&`) com captura ativa do sinal `SIGCHLD` e uso de `waitpid(WNOHANG)` para prevenção rigorosa de processos fantasmas.
* **Expansão Dinâmica (Wildcards):** Tratamento e expansão de metacaracteres (ex: `*.txt`, `*.c`) utilizando a API POSIX `glob()`.
* **Saneamento de Memória:** Código totalmente auditado, sem vazamentos de memória estática ou dinâmica (verificado via *Valgrind*).

## Pré-requisitos e Compilação

Para executar o projeto, é necessário um ambiente Linux nativo (ou WSL no Windows) com as ferramentas de compilação básicas instaladas.

1. Clone este repositório:
   ```bash
   git clone [https://github.com/seu-usuario/FP_DCC403_Tema_2_UFRR.git](https://github.com/seu-usuario/FP_DCC403_Tema_2_UFRR.git)
   cd FP_DCC403_Tema_2_UFRR

   Compile o código utilizando o utilitário Make:

Bash
make
Este comando gerará automaticamente o executável edge-engine.

Para auditar a integridade de memória do código (opcional):

Bash
make valgrind

Como Usar
Inicie o interpretador executando o binário gerado:

Bash
./edge-engine
O prompt customizado edge-engine> surgirá na tela. A partir daí, o orquestrador suporta o roteamento padrão de comandos Unix.

Exemplos de Uso
1. Execução Simples e Expansão de Arquivos:

Bash
edge-engine> ls *.c
2. Encadeamento de Dados via Pipe em Memória:

Bash
edge-engine> cat Makefile | grep gcc
3. Execução Assíncrona (Background):
O terminal será liberado instantaneamente enquanto a tarefa roda nos bastidores.

Bash
edge-engine> sleep 10 &
4. Saída Segura (Comando Interno):

Bash
edge-engine> exit

Estrutura do Repositório
main.c — Código-fonte principal contendo as lógicas de parsing, IPC, tratamento de sinais e chamadas POSIX.

Makefile — Arquivo de automação e orquestração de compilação.

Relatorio_Final.pdf — Documentação técnica no formato IEEE descrevendo a arquitetura e testes do motor.
