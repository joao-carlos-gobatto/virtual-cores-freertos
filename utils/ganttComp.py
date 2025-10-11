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
            try:
                if int(tick) < 0:
                    # negativo indica lost comp time -> não plotar barra
                    continue
            except Exception:
                pass
            if task != 'idle':
                nome_tarefa = 'print' if task == '-1' else task
                chave = (real, virt)
                tarefas.setdefault(chave, []).append((nome_tarefa, tick, largura))
    return tarefas

def plotar_gantt_logical_cores(tarefas, ticks_de_print, lost_map=None):
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
    all_x = []
    for i, core in enumerate(cores):
        for nome, inicio, duracao in tarefas[core]:
            cor = cor_print if nome == 'print' else mapa_cores.get(nome, 'gray')
            ax.broken_barh([(inicio, duracao)], (y_ticks[i], altura), facecolors=cor, zorder=2)
            ax.text(inicio + duracao / 2, y_ticks[i] + altura / 2, nome,
                    fontsize=16, va='center', ha='center', color='black', zorder=3)
            try:
                all_x.append(int(inicio))
                all_x.append(int(inicio + duracao))
            except Exception:
                pass

    ax.set_yticks(y_ticks)
    ax.set_yticklabels(y_labels)
    ax.set_xlabel('Tick')
    ax.set_title('Gantt por Núcleo Lógico (com agrupamento por núcleo físico)')

    # Legenda
    handles = [mpatches.Patch(color=mapa_cores[n], label=n) for n in nomes_unicos]
    handles.append(mpatches.Patch(color=cor_print, label='print'))
    ax.legend(handles=handles, loc='lower right', bbox_to_anchor=(1, -0.15),
              ncol=4, frameon=False)

    # Determinar intervalo X visível (com margem pequena)
    if all_x:
        x_min = min(all_x)
        x_max = max(all_x)
        margem = max(1, int((x_max - x_min) * 0.001))  # 2% de margem, pelo menos 1
        ax.set_xlim(x_min - margem, x_max + margem)
    else:
        ax.set_xlim(0, 10)

    # Desenhar linhas verticais para ticks de print (se houver) — FILTRANDO para o intervalo visível
    if ticks_de_print:
        try:
            visible_min, visible_max = ax.get_xlim()
            ticks_filtered = sorted(set(int(t) for t in ticks_de_print))
            ticks_filtered = [t for t in ticks_filtered if (visible_min - 1) <= t <= (visible_max + 1)]
            for t in ticks_filtered:
                ax.axvline(t, linestyle='--', linewidth=1.0, alpha=0.3, zorder=1)
        except Exception:
            pass

    # Desenhar linhas vermelhas e anotação "Lost comp time" para ticks negativos (valores transformados em positivos)
    if lost_map:
        try:
            visible_min, visible_max = ax.get_xlim()
            # obter chaves (ticks) e filtrar para os visíveis
            unique_lost = sorted(k for k in lost_map.keys() if (visible_min - 1) <= k <= (visible_max + 1))
            if len(y_ticks) > 0:
                ymin, ymax = ax.get_ylim()
                # posição vertical próxima à borda inferior (2% acima do mínimo)
                y_range = ymax - ymin
                y_pos = ymin + 0.02 * y_range
            else:
                y_pos = 0.1

            for t in unique_lost:
                ax.axvline(t, color='red', linestyle='-', linewidth=1.5, zorder=4)

                # preparar string das tasks associadas (mapear '-1' -> 'print' para legibilidade)
                tasks_set = set()
                for raw in lost_map.get(t, []):
                    name = raw.strip()
                    if name == '-1':
                        name = 'print'
                    tasks_set.add(name)
                tasks_list = sorted(tasks_set)
                tasks_label = ', '.join(tasks_list) if tasks_list else 'unknown'

                # montar label final (horizontal, baixinho)
                label = f"Deadline Miss TaskID ({tasks_label})"

                ax.text(
                    t,
                    y_pos,
                    label,
                    rotation=0,         # texto horizontal
                    va='bottom',        # começa a partir de y_pos para cima
                    ha='center',        # centraliza horizontalmente sobre a linha
                    color='red',
                    fontsize=10,
                    zorder=5,
                    clip_on=False,      # evita corte do rótulo
                    bbox={
                        'facecolor': 'khaki',
                        'edgecolor': 'red',
                        'boxstyle': 'round,pad=0.3',
                        'alpha': 0.95
                    }
                )
        except Exception:
            pass

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

    # --- ALTERAÇÃO: mapear tick negativo (abs) -> conjunto de tasks associadas
    lost_map = defaultdict(set)
    for entrada in dados:
        for tick, task, real, virt in entrada:
            try:
                if int(tick) < 0:
                    lost_map[abs(int(tick))].add(task.strip())
            except Exception:
                pass
    # -----------------------------------------------------------------------------------------------

    tarefas = preparar_dados_logical_cores(dados, limite=1000, largura=3)
    plotar_gantt_logical_cores(tarefas, ticks_de_print, lost_map)
