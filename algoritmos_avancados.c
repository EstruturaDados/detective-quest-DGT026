#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NOME 80
#define MAX_PISTA 120
#define HASH_SIZE 31   // pequeno e suficiente para o exemplo

// =======================
// ESTRUTURAS DE DADOS
// =======================

// Nó da árvore binária que representa uma sala
typedef struct Sala {
    char nome[MAX_NOME];
    char pista[MAX_PISTA]; // string vazia se não houver pista
    struct Sala *esquerda;
    struct Sala *direita;
} Sala;

// Nó da BST que armazena pistas coletadas (sem duplicatas)
typedef struct PistaNode {
    char pista[MAX_PISTA];
    struct PistaNode *esq;
    struct PistaNode *dir;
} PistaNode;

// Nó da lista encadeada para tabela hash (associação pista -> suspeito)
typedef struct HashNode {
    char pista[MAX_PISTA];
    char suspeito[MAX_NOME];
    struct HashNode *proximo;
} HashNode;

// =======================
// FUNÇÕES: SALAS (ÁRVORE)
// =======================

/*
 * criarSala() – cria dinamicamente um cômodo com nome e pista (pista pode ser "")
 * Retorna ponteiro para a sala criada.
 */
Sala* criarSala(const char *nome, const char *pista) {
    Sala *nova = (Sala*) malloc(sizeof(Sala));
    if (!nova) {
        fprintf(stderr, "Erro de alocação de memória para sala\n");
        exit(EXIT_FAILURE);
    }
    strncpy(nova->nome, nome, MAX_NOME-1); nova->nome[MAX_NOME-1] = '\0';
    if (pista) {
        strncpy(nova->pista, pista, MAX_PISTA-1); nova->pista[MAX_PISTA-1] = '\0';
    } else {
        nova->pista[0] = '\0';
    }
    nova->esquerda = nova->direita = NULL;
    return nova;
}

// libera recursivamente árvore de salas
void liberarSalas(Sala *raiz) {
    if (!raiz) return;
    liberarSalas(raiz->esquerda);
    liberarSalas(raiz->direita);
    free(raiz);
}

// =======================
// FUNÇÕES: BST DE PISTAS
// =======================

/*
 * inserirPista() – insere a pista coletada na BST (sem duplicatas)
 * Retorna raiz (pode ser a mesma passada).
 */
PistaNode* inserirPista(PistaNode *raiz, const char *pista) {
    if (pista == NULL || strlen(pista) == 0) return raiz; // nada a inserir

    if (raiz == NULL) {
        PistaNode *novo = (PistaNode*) malloc(sizeof(PistaNode));
        if (!novo) {
            fprintf(stderr, "Erro de alocação de memória para PistaNode\n");
            exit(EXIT_FAILURE);
        }
        strncpy(novo->pista, pista, MAX_PISTA-1); novo->pista[MAX_PISTA-1] = '\0';
        novo->esq = novo->dir = NULL;
        return novo;
    }

    int cmp = strcmp(pista, raiz->pista);
    if (cmp < 0)
        raiz->esq = inserirPista(raiz->esq, pista);
    else if (cmp > 0)
        raiz->dir = inserirPista(raiz->dir, pista);
    // se cmp == 0, já existe — não insere duplicata
    return raiz;
}

// Em-ordem: imprime pistas em ordem alfabética
void exibirPistasEmOrdem(PistaNode *raiz) {
    if (!raiz) return;
    exibirPistasEmOrdem(raiz->esq);
    printf("- %s\n", raiz->pista);
    exibirPistasEmOrdem(raiz->dir);
}

// percorre a BST e conta quantas pistas apontam para 'suspeito' conforme a hash
int contarPistasPorSuspeito(PistaNode *raiz, const char *suspeito, HashNode **tabelaHash);

// libera BST de pistas
void liberarPistas(PistaNode *raiz) {
    if (!raiz) return;
    liberarPistas(raiz->esq);
    liberarPistas(raiz->dir);
    free(raiz);
}

// =======================
// FUNÇÕES: TABELA HASH
// =======================

// função de hash simples para strings
unsigned int hashString(const char *s) {
    unsigned int h = 0;
    while (*s) {
        h = (h * 31u) + (unsigned char)(*s);
        s++;
    }
    return h % HASH_SIZE;
}

/*
 * inserirNaHash() – insere associação pista -> suspeito na tabela hash.
 * Se a pista já existir, sobrescreve o suspeito (não é esperado no cenário fixo).
 */
void inserirNaHash(HashNode **tabela, const char *pista, const char *suspeito) {
    unsigned int idx = hashString(pista);
    // procura se já existe
    HashNode *curr = tabela[idx];
    while (curr) {
        if (strcmp(curr->pista, pista) == 0) {
            // atualiza suspeito
            strncpy(curr->suspeito, suspeito, MAX_NOME-1); curr->suspeito[MAX_NOME-1] = '\0';
            return;
        }
        curr = curr->proximo;
    }
    // não encontrou, cria novo nó no início da lista
    HashNode *novo = (HashNode*) malloc(sizeof(HashNode));
    if (!novo) {
        fprintf(stderr, "Erro de alocação para HashNode\n");
        exit(EXIT_FAILURE);
    }
    strncpy(novo->pista, pista, MAX_PISTA-1); novo->pista[MAX_PISTA-1] = '\0';
    strncpy(novo->suspeito, suspeito, MAX_NOME-1); novo->suspeito[MAX_NOME-1] = '\0';
    novo->proximo = tabela[idx];
    tabela[idx] = novo;
}

/*
 * encontrarSuspeito() – consulta a tabela hash e retorna ponteiro para o nome do suspeito
 * Retorna NULL se não encontrado.
 */
const char* encontrarSuspeito(HashNode **tabela, const char *pista) {
    unsigned int idx = hashString(pista);
    HashNode *curr = tabela[idx];
    while (curr) {
        if (strcmp(curr->pista, pista) == 0) {
            return curr->suspeito;
        }
        curr = curr->proximo;
    }
    return NULL;
}

// libera memória da tabela hash
void liberarHash(HashNode **tabela) {
    for (int i = 0; i < HASH_SIZE; ++i) {
        HashNode *curr = tabela[i];
        while (curr) {
            HashNode *tmp = curr;
            curr = curr->proximo;
            free(tmp);
        }
        tabela[i] = NULL;
    }
}

// =======================
// FUNÇÕES: EXPLORAÇÃO
// =======================

/*
 * explorarSalas() – navega pela árvore e ativa o sistema de pistas.
 * Parâmetros:
 *  - atual: sala atual
 *  - arvorePistas: ponteiro para a raiz da BST que armazena pistas coletadas
 *  - tabelaHash: tabela hash onde estão as associações pista -> suspeito
 *
 * A função coleta automaticamente a pista (se houver) e a insere na BST (sem duplicatas).
 */
void explorarSalas(Sala *atual, PistaNode **arvorePistas, HashNode **tabelaHash) {
    if (!atual) return;

    char opcao;
    while (1) {
        printf("\nVocê está em: %s\n", atual->nome);
        if (strlen(atual->pista) > 0) {
            printf("Pista encontrada aqui: '%s'\n", atual->pista);
            // adiciona à BST (somente se não existir já)
            *arvorePistas = inserirPista(*arvorePistas, atual->pista);
        } else {
            printf("Nenhuma pista nesta sala.\n");
        }

        // mostra opções de navegação (apenas quando há caminho)
        int temEsq = (atual->esquerda != NULL);
        int temDir = (atual->direita != NULL);

        printf("\nOpções:\n");
        if (temEsq) printf("e - Ir para a esquerda (%s)\n", atual->esquerda->nome);
        if (temDir) printf("d - Ir para a direita (%s)\n", atual->direita->nome);
        printf("s - Sair da exploração\n");
        printf("Opção: ");
        scanf(" %c", &opcao);

        if (opcao == 'e' && temEsq) {
            explorarSalas(atual->esquerda, arvorePistas, tabelaHash);
            return; // após voltar da recursão, interrompe ciclo atual
        } else if (opcao == 'd' && temDir) {
            explorarSalas(atual->direita, arvorePistas, tabelaHash);
            return;
        } else if (opcao == 's') {
            printf("Você escolheu sair da exploração.\n");
            return;
        } else {
            printf("Opção inválida ou caminho inexistente. Tente novamente.\n");
        }
    }
}

// varredura recursiva para contar pistas apontando ao suspeito
int contarPistasPorSuspeitoAux(PistaNode *raiz, const char *suspeito, HashNode **tabelaHash) {
    if (!raiz) return 0;
    int soma = 0;
    // esquerda
    soma += contarPistasPorSuspeitoAux(raiz->esq, suspeito, tabelaHash);
    // atual
    const char *s = encontrarSuspeito(tabelaHash, raiz->pista);
    if (s && strcmp(s, suspeito) == 0) soma++;
    // direita
    soma += contarPistasPorSuspeitoAux(raiz->dir, suspeito, tabelaHash);
    return soma;
}

int contarPistasPorSuspeito(PistaNode *raiz, const char *suspeito, HashNode **tabelaHash) {
    return contarPistasPorSuspeitoAux(raiz, suspeito, tabelaHash);
}

/*
 * verificarSuspeitoFinal() – conduz à fase de julgamento final.
 * Exibe as pistas coletadas, pede ao jogador para acusar um suspeito e
 * verifica se pelo menos 2 pistas apontam para esse suspeito.
 */
void verificarSuspeitoFinal(PistaNode *arvorePistas, HashNode **tabelaHash) {
    if (!arvorePistas) {
        printf("\nVocê não coletou pistas suficientes para acusar alguém.\n");
        return;
    }

    printf("\n--- Pistas coletadas (ordem alfabética) ---\n");
    exibirPistasEmOrdem(arvorePistas);

    char acusado[MAX_NOME];
    printf("\nIndique o nome do suspeito a ser acusado: ");
    // lê nome completo com espaços
    scanf(" %[^\n]s", acusado);

    int contador = contarPistasPorSuspeito(arvorePistas, acusado, tabelaHash);
    printf("\nTotal de pistas que apontam para '%s': %d\n", acusado, contador);

    if (contador >= 2) {
        printf("Decisão: Acusação SUSTENTADA! Há evidências suficientes para responsabilizar %s.\n", acusado);
    } else {
        printf("Decisão: Acusação FRACA. Não há pistas suficientes para sustentar a acusação contra %s.\n", acusado);
    }
}

// =======================
// MAIN: montagem do mapa
// =======================

int main() {
    // Inicializa tabela hash vazia
    HashNode *tabelaHash[HASH_SIZE];
    for (int i = 0; i < HASH_SIZE; ++i) tabelaHash[i] = NULL;

    // Montagem fixa da mansão (árvore de salas)
    // Para cada sala, definimos uma pista (pode ser string vazia)
    Sala *hall = criarSala("Hall de Entrada", "Pegadas molhadas no tapete");
    Sala *salaEstar = criarSala("Sala de Estar", "Copo quebrado");
    Sala *cozinha = criarSala("Cozinha", "Faca com resquícios de tinta");
    Sala *biblioteca = criarSala("Biblioteca", "Livro rasgado com anotações");
    Sala *jardim = criarSala("Jardim", "Botas com lama");
    Sala *escritorio = criarSala("Escritório", "Carta com nome do suspeito A");
    Sala *sotao = criarSala("Sotão", "");
    Sala *quarto = criarSala("Quarto Principal", "fio de tecido vermelho");

    // Conexões (árvore)
    hall->esquerda = salaEstar;
    hall->direita = cozinha;

    salaEstar->esquerda = biblioteca;
    salaEstar->direita = jardim;

    cozinha->esquerda = escritorio;
    cozinha->direita = sotao;

    biblioteca->esquerda = quarto; // adiciona profundidade para exemplo

    // Montagem da tabela hash: associa pistas a suspeitos (exemplo)
    // Aqui definimos as associações estáticas do enunciado/experiência.
    inserirNaHash(tabelaHash, "Pegadas molhadas no tapete", "Suspeito B");
    inserirNaHash(tabelaHash, "Copo quebrado", "Suspeito A");
    inserirNaHash(tabelaHash, "Faca com resquícios de tinta", "Suspeito A");
    inserirNaHash(tabelaHash, "Livro rasgado com anotações", "Suspeito C");
    inserirNaHash(tabelaHash, "Botas com lama", "Suspeito B");
    inserirNaHash(tabelaHash, "Carta com nome do suspeito A", "Suspeito A");
    inserirNaHash(tabelaHash, "fio de tecido vermelho", "Suspeito C");
    // pistas sem correspondência simplesmente não retornam suspeito (NULL)

    // BST de pistas coletadas (inicialmente vazia)
    PistaNode *arvorePistas = NULL;

    // Introdução e exploração
    printf("=== INVESTIGAÇÃO: EXPLORAÇÃO DA MANSÃO ===\n");
    printf("Iniciando no Hall de Entrada. Explore e colete pistas.\n");
    explorarSalas(hall, &arvorePistas, tabelaHash);

    // Fase final: exibir pistas e acusar
    verificarSuspeitoFinal(arvorePistas, tabelaHash);

    // Liberar memória
    liberarPistas(arvorePistas);
    liberarHash(tabelaHash);
    liberarSalas(hall);

    printf("\nFim do programa. Obrigado por investigar!\n");
    return 0;
}
