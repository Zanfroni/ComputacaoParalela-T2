#include <stdio.h>
#include <time.h>

// Variavel auxiliar (se tonara critica mo M/C e nao podera ser mais usada desta forma)
// Serve apenas para registrar o peso da mochila da permutacao com melhor valor
double result_Weight = 0;

// Funcao que realiza o algoritmo da mochila em questao
// A mesma retorna para o usuario apenas o melhor valor. O peso esta na variavel global,
// enquanto que o seed simplesmente da para dizer olhando o tamanho da permutacao.
// Este algoritmo geraabsolutamente todas as combinacoes possiveis
// para o seed r de entrada, validando o melhor valor encontrado ate o momento quando ele
// termina de configurar uma permutacao
double generateCombinations(float v[], float w[], float cap, float data[], float weight[], int start, int end,
                            int index, int r, double best_value) {
    if (index == r){
        double total = 0;
        double totalW = 0;
        for (int j=0; j<r; j++){
            total += data[j];
            totalW += weight[j];
            //printf("%f ", data[j]);
        }
        //printf("\n");
        //printf("Total dessa soma deu = %f \n", total);
        //printf("\n");
        
        if(total > best_value && totalW <= cap){
            best_value = total;
            result_Weight = totalW;
        }
        return best_value;
    }
    
    for (int i=start; i<=end && end-i+1 >= r-index; i++) {
        data[index] = v[i];
        weight[index] = w[i];
        best_value = generateCombinations(v, w, cap, data, weight, i+1, end, index+1,
                                          r, best_value);
    }
    return best_value;
}


// Funcao sequencial
// Manda os vetores de valores (v), pesos (w) e o seed r (que vai de 1 ate s), gerando todas as combincaoes possiveis para
// os intervalos. A cada melhor valor que retorna, ele guarda em um novo vetor chamado best_v, que vai de 1 ate s.
// No final ele escolhe o melhor candidato e imprime o seed usado para encontrar ele, o valor e o peso da mochila.
// A parte que sera paralelizada para os escravos sera o primeiro laco, onde contem o metodo generateCombinations(),
// ou seja, cada escravo ira trabalhar com as combinacoes de seeds diferentes em paralelo, esperando-se acelerar o
// desempenho do algoritmo.
// Utilizou-se valores decimais nos valores e pesos por sugestao do professor para garantir que o algoritmo fique
// mais ocupado com contas mais complicadas.
int main() {
    int s = 32;
    float v[] = {9.76,7.90,25.99,36.95,13.14,37.33,40.12,45.85,9.96,
                 34.22,7.03,38.14,38.31,23.63,10.64,31.12,19.47,18.60,
                 49.39,9.87,10.36,6.95,25.80,30.92,14.64,42.81,12.08,38.04,14.88,9.31,
                 15.33,2.85};
    float w[] = {25.07,25.66,19.64,39.85,1.50,11.05,28.34,31.43,34.69,
                 46.17,16.15,16.77,41.21,48.12,24.53,29.77,7.42,19.16,45.61,
                 9.16,38.96,44.98,22.26,37.93,43.65,38.23,3.76,7.55,22.14,45.02,6.49,36.33};
    //float v[] = {1,2,3,4,500};
    //float w[] = {1,1,1,1,198};
    float cap = 200;
    int n = sizeof(v)/sizeof(1);

    int r;
    double best_v[s];
    double best_w[s];
    
    clock_t t;
    t = clock();
    double result = 0;
    
    for(r = 1; r <= s; r++){
        float data[r];
        float weight[r];
        result = generateCombinations(v, w, cap, data, weight, 0, n-1, 0, r, 0);
        if(result == 0) result_Weight = 0;
        best_v[r-1] = result;
        best_w[r-1] = result_Weight;
        printf("Resultado para r = %d foi best_value = %.2f \n",r,result);
        printf("Peso da mochila ficou = %.2f \n", result_Weight);
    }
    
    int i;
    int final_seed = -1;
    double final_value = -1;
    double final_weight = -1;
    for(i = 0; i < s; i++){
        if(best_v[i] > final_value){
            final_seed = i+1;
            final_value = best_v[i];
            final_weight = best_w[i];
        }
    }
    
    printf("MELHOR SEED ENCONTRADO = %d \n",final_seed);
    printf("MELHOR VALOR FOI = %.2f \n", final_value);
    printf("PESO DA MOCHILA FICOU = %.2f \n", final_weight);
    
    t = clock() - t;
    double time_taken = ((double)t)/CLOCKS_PER_SEC;
    printf("Tempo de execucao: %f segundos\n", time_taken);
}
