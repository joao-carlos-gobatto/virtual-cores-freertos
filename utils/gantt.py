import re
import sys
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

#Exemplo de uso do script: python gantt.py nome_do_arquivo_de_entrada.txt

def extrair_ticks_print(filepath):
    ticks = []
    with open(filepath, 'r') as f:
        for linha in f:
            if linha.startswith('###'):
                texto = linha[3:].lstrip()
                m = re.match(r'(\d+)', texto)
                if m:
                    ticks.append(int(m.group(1)))
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

def preparar_dados_logical_cores(dados, limite=70):
    tarefas = {}
    for entrada in dados[:limite]:
        for tick, task, real, virt in entrada:
            if task != 'idle':
                chave = (real, virt)
                tarefas.setdefault(chave, []).append((task, tick, 1))
    return tarefas

def plotar_gantt_logical_cores(tarefas, ticks_de_print):
    fig, ax = plt.subplots(figsize=(12, 0.8 * len(tarefas)))
    paleta = ['#4fbeda','#3bacc8','#289ab6','#1488a3','#e63b16','#ec552d','#f37044','#f98a5a']
    cor_print = '#629351'

    cores = sorted(tarefas.keys())
    y_ticks = [i*1.5 for i in range(len(cores))]
    y_labels = [f'Lógico {r}.{v}' for r,v in cores]
    altura = 0.8

    # mapeia cores por nome de task
    unicas = []
    for lst in tarefas.values():
        for lbl, _, _ in lst:
            if lbl!='print' and lbl not in unicas:
                unicas.append(lbl)
    mapa = {nome: paleta[i%len(paleta)] for i,nome in enumerate(unicas)}

    for i, core in enumerate(cores):
        for lbl, start, dur in tarefas[core]:
            c = cor_print if lbl=='print' else mapa.get(lbl,'gray')
            ax.broken_barh([(start, dur)], (y_ticks[i], altura), facecolors=c)
            ax.text(start+dur/2, y_ticks[i]+altura/2, lbl,
                    fontsize=6, va='center', ha='center', color='black')

    # limites de eixo x e marcações de print
    # ax.set_xlim(0, 400)
    # for tp in ticks_de_print:
    #     if 0 <= tp <= 400:
    #         ax.axvline(tp, color='gray', linestyle=':', alpha=0.5)

    ax.set_yticks(y_ticks)
    ax.set_yticklabels(y_labels)
    ax.set_xlabel('Tick')
    ax.set_title('Gantt por Núcleo Lógico (0–400)')

    # legenda
    handles = [mpatches.Patch(color=mapa[n], label=n) for n in unicas]
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
    tarefas = preparar_dados_logical_cores(dados, limite=240)
    plotar_gantt_logical_cores(tarefas, ticks_de_print)