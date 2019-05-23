#include <stdio.h>
#include <time.h>
#include "mpi.h"

#define TAREFAS 32 // Numero de tarefas no saco de trabalho, processo 0 é o mestre

// Mensagens de comunicacao
int KILL = -1;
int REQUEST = -2;
int RESULT = -3;

// Variavel auxiliar. Nao se torna critica por causa do paralelismo multicomputador
double result_Weight = 0;

// Funcao que faz as operacoes combinatoriais
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


// Funcao principal, completamente remodelada para funcionar com
// MPI com iniciativa dos escravos.
int main(int argc, char** argv) {
    
    int my_rank; // Identificador deste processo
    int proc_n; // Numero de processos disparados pelo usuário na linha de comando (np)
    int message; // Buffer para as mensagens
    int saco[TAREFAS]; // saco de trabalho
    MPI_Status status; // Estrutura que guarda o estado de retorno de mensagens MPI

    MPI_Init(&argc , &argv); // funcao que inicializa o MPI, todo o código paralelo esta abaixo

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank); // pega pega o numero do processo atual (rank)
    MPI_Comm_size(MPI_COMM_WORLD, &proc_n); // pega informação do numero de processos (quantidade total)


    /*Inicio da secao das variaveis a serem testadas no algoritmo*/
    int slaves = proc_n - 1;
    int work = TAREFAS;
    int seed;
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
    
    double best_v[TAREFAS];
    double best_w[TAREFAS];
    double best_seed[TAREFAS];
    int best_index = 0;
    
    int mpi_seed;
    double mpi_value;
    double mpi_weight;
    /*Fim da secao*/
    
    
    
    
    // LOGICA DO MESTRE
    if(my_rank == 0){
        
        // Inicia contagem de tempo. Nota-se que o proprio mestre
        // finaliza a contagem quando todos os escravos estiverem mortos
        clock_t t;
        t = clock();
        
        while(slaves > 0){
            // Mestre inicialmente recebe uma mensagem dos escravos
            // Esta mensagem precisa ser algo do escravo que represente um REQUEST ou RESULT,
            // caso contrario o mestre VAI ignorar
            MPI_Recv(&message, 1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
            
            // Se for REQUEST, ele tem que mandar um seed para o escravo
            // trabalhar e gerar as combinacoes
            if(message == REQUEST){
                // CASO nao tenha mais trabalho, ele pede para o escravo
                // se matar
                if(work == 0){
                    MPI_Send(&KILL, 1, MPI_INT, status.MPI_SOURCE, 1, MPI_COMM_WORLD);
                    slaves--;
                    continue;
                }
                
                // Agora ele tem que mandar para o escravo o seed para geracao das permutacoes
                seed = work;
                MPI_Send(&seed, 1, MPI_INT, status.MPI_SOURCE, 1,
                MPI_COMM_WORLD);
                work--;
                
            // Se for RESULT, ele tem que pegar os dados do escravo,
            // que neste caso sao o melhor valor encontrado com as permutacoes,
            // o seed usado e o peso que ficou a mochila
            } else if(message == RESULT) {
                
                MPI_Recv(&mpi_seed, 1, MPI_INT, status.MPI_SOURCE, 1, MPI_COMM_WORLD, &status);
                MPI_Recv(&mpi_value, 1, MPI_DOUBLE, status.MPI_SOURCE, 1, MPI_COMM_WORLD, &status);
                MPI_Recv(&mpi_weight, 1, MPI_DOUBLE, status.MPI_SOURCE, 1, MPI_COMM_WORLD, &status);
                
                // Ele insere estes valores nestes vetores auxiliares
                // para escolher o melhor no final depois que o trabalho
                // paralelo ja ter sido realizado.
                // A escolha nao foi paralelizada por ser apenas um laco
                // em cima de um so vetor.
                best_v[best_index] = mpi_value;
                best_w[best_index] = mpi_weight;
                best_seed[best_index] = mpi_seed;
                best_index++;
                
                //printf("Resultado para r = %d foi best_value = %.2f\n",mpi_seed,mpi_value);
                //printf("Peso da mochila ficou = %.2f \n", mpi_weight);
            }
        }
        
        
        // DEPOIS de todos os escravos estarem mortos,
        // o mestre escolhe o melhor valor e imprime os resultados
        // Depois, ele para o tempo e imprime na tela. O seu processo
        // de selecao do melhor valor e contabilizado para o tempo.
        // Finalmente, ele se mata no final do codigo
        int i;
        int final_seed = -1;
        double final_value = -1;
        double final_weight = -1;
        
        for(i = 0; i < TAREFAS; i++){
            if(best_v[i] > final_value){
                final_seed = best_seed[i];
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
        
        
        
        
        
    // LOGICA DO ESCRAVO
    } else {
        while(1){
            
            // Peco trabalho para o mestre
            MPI_Send(&REQUEST, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
            
            //Recebe o seed para trabalhar
            MPI_Recv(&message, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
            
            //Se mata se a mensagem recebida for KILL
            if(message == -1){
                MPI_Finalize();
                return 0;
            }
            
            // Declara o seed para realizar as operacoes combinatoriais
            int r = message;
            float data[r];
            float weight[r];
            double result;
            
            // Realiza as operacoes no algoritmo escolhido.
            result = generateCombinations(v,w,cap,data,weight,0,n-1,0,message,0);
            
            if(result == 0) result_Weight = 0;
            best_v[r-1] = result;
            best_w[r-1] = result_Weight;
            
            // Avisa para o mestre que vai enviar os resultados
            MPI_Send(&RESULT, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
            
            // Manda o seed, o valor final e o peso para o mestre
            MPI_Send(&r, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
            MPI_Send(&result, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
            MPI_Send(&result_Weight, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
        }
    }
    // Mestre se mata e o codigo se encerra
    MPI_Finalize();
    return 0;
}
