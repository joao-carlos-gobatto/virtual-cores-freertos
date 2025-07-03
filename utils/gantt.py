import re
import sys
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from collections import defaultdict

def extrair_ticks_print(filepath):
    ticks = []
    with open(filepath, 'r') as f:
        for linha in f:
            if linha.startswith('###'):
                texto = linha[3:].lstrip()
                match = re.match(r'(\d+)', texto)
                if match:
                    ticks.append(int(match.group(1)))
    return ticks

def ler_arquivo(filepath):
    dados = []
    with open(filepath, 'r') as file:
        for linha in file:
            if linha.startswith('$'):
                partes = linha[1:].split(',')
                if len(partes) == 8:
                    t0, task0, r0, v0 = int(partes[0]), partes[1], int(partes[2]), int(partes[3])
                    t1, task1, r1, v1 = int(partes[4]), partes[5], int(partes[6]), int(partes[7])
                    dados.append([(t0, task0, r0, v0), (t1, task1, r1, v1)])
    return dados

def preparar_dados_logical_cores(dados, limite=70, largura=3):
    tarefas = {}
    for entrada in dados[:limite]:
        for tick, task, real, virt in entrada:
            if task != 'idle':
                nome_tarefa = 'print' if task == '-1' else task
                chave = (real, virt)
                tarefas.setdefault(chave, []).append((nome_tarefa, tick, largura))
    return tarefas

def plotar_gantt_logical_cores(tarefas, ticks_de_print):
    fig, ax = plt.subplots(figsize=(14, 1.2 * len(tarefas)))

    paleta = ["#f94144","#f3722c","#f8961e","#f9844a","#f9c74f","#90be6d","#43aa8b","#4d908e","#577590","#277da1"]
    cor_print = '#629351'
    fundo_cores = ['#f8f9fa', '#adb5bd']  # cores alternadas para plano de fundo

    # Ordenar cores (núcleos lógicos) e preparar posição y
    cores = sorted(tarefas.keys())
    y_ticks = [i * 2.0 for i in range(len(cores))]
    y_labels = [f'Lógico {r}.{v}' for r, v in cores]
    altura = 1.0

    # Mapear núcleos físicos para os índices de seus núcleos lógicos
    fisicos = defaultdict(list)
    for idx, (r, v) in enumerate(cores):
        fisicos[r].append(idx)

    # Desenhar fundo para cada core físico
    for i, (core_fisico, indices) in enumerate(fisicos.items()):
        y_min = y_ticks[min(indices)] - 0.2
        y_max = y_ticks[max(indices)] + altura + 0.2
        ax.axhspan(y_min, y_max, facecolor=fundo_cores[i % 2], zorder=0)

    # Cores por nome de tarefa
    nomes_unicos = []
    for lista in tarefas.values():
        for nome, _, _ in lista:
            if nome != 'print' and nome not in nomes_unicos:
                nomes_unicos.append(nome)
    mapa_cores = {nome: paleta[i % len(paleta)] for i, nome in enumerate(nomes_unicos)}

    # Desenhar tarefas
    for i, core in enumerate(cores):
        for nome, inicio, duracao in tarefas[core]:
            cor = cor_print if nome == 'print' else mapa_cores.get(nome, 'gray')
            ax.broken_barh([(inicio, duracao)], (y_ticks[i], altura), facecolors=cor, zorder=2)
            ax.text(inicio + duracao / 2, y_ticks[i] + altura / 2, nome,
                    fontsize=16, va='center', ha='center', color='black', zorder=3)

    ax.set_yticks(y_ticks)
    ax.set_yticklabels(y_labels)
    ax.set_xlabel('Tick')
    ax.set_title('Gantt por Núcleo Lógico (com agrupamento por núcleo físico)')

    # Legenda
    handles = [mpatches.Patch(color=mapa_cores[n], label=n) for n in nomes_unicos]
    handles.append(mpatches.Patch(color=cor_print, label='print'))
    ax.legend(handles=handles, loc='lower right', bbox_to_anchor=(1, -0.15),
              ncol=4, frameon=False)

    plt.grid(True, axis='x', linestyle='--', alpha=0.3)
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Uso: python nome_do_script.py arquivo.txt")
        sys.exit(1)

    arquivo = sys.argv[1]
    ticks_de_print = extrair_ticks_print(arquivo)
    dados = ler_arquivo(arquivo)
    tarefas = preparar_dados_logical_cores(dados, limite=100, largura=3)
    plotar_gantt_logical_cores(tarefas, ticks_de_print)