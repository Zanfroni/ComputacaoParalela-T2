# Para executar o gerador para gerar numeros aleatorios, basta digitar python3 gerador.py

import random

for i in range(32):
    print('{0:.2f},'.format(random.uniform(1,50)), end = '')
