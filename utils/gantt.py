import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

def ler_arquivo(filepath):
    dados = []
    with open(filepath, 'r') as file:
        for linha in file:
            if linha.startswith('$'):
                linha = linha.strip().replace('$', '')
                partes = linha.split(',')
                if len(partes) == 4:
                    tick0 = int(partes[0].strip())
                    task0 = partes[1].strip()
                    tick1 = int(partes[2].strip())
                    task1 = partes[3].strip()
                    dados.append({'core0': (tick0, task0), 'core1': (tick1, task1)})
    return dados

def preparar_dados_gantt(dados, limite=10):
    tarefas = { 'core0': [], 'core1': [] }
    
    ticks_core0 = []
    tasks_core0 = []

    ticks_core1 = []
    tasks_core1 = []

    for entrada in dados[:limite]:
        tick0, task0 = entrada['core0']
        tick1, task1 = entrada['core1']

        ticks_core0.append(tick0)
        tasks_core0.append(task0)

        ticks_core1.append(tick1)
        tasks_core1.append(task1)

    # Ordenar por tick para detectar intervalos corretamente
    pares_core0 = sorted(zip(ticks_core0, tasks_core0), key=lambda x: x[0])
    pares_core1 = sorted(zip(ticks_core1, tasks_core1), key=lambda x: x[0])

    # Preencher Core 0 com tarefa "print" nos espaços entre tarefas
    for i in range(len(pares_core0)):
        tick_atual, task_atual = pares_core0[i]

        if i > 0:
            tick_anterior, _ = pares_core0[i - 1]
            intervalo = tick_atual - (tick_anterior + 1)
            if intervalo > 0:
                tarefas['core0'].append(("print", tick_anterior + 1, intervalo))

        tarefas['core0'].append((task_atual, tick_atual, 1))

    # Core 1 sem "print"
    for tick, task in pares_core1:
        tarefas['core1'].append((task, tick, 1))

    return tarefas

def plotar_gantt(tarefas):
    fig, ax = plt.subplots(figsize=(12, 3))

    # Paletas separadas
    paleta_azul = ['#4fbeda', '#3bacc8', '#289ab6', '#1488a3']
    paleta_vermelho = ['#e63b16', '#ec552d', '#f37044', '#f98a5a']
    cor_print = '#629351'

    y_labels = ['Core 0', 'Core 1']
    y_ticks = [3, 1]
    altura_barra = 0.8

    # Mapear tarefas únicas (excluindo "print") para cores por núcleo
    tarefas_unicas_core0 = []
    tarefas_unicas_core1 = []

    for label, _, _ in tarefas['core0']:
        if label != 'print' and label not in tarefas_unicas_core0:
            tarefas_unicas_core0.append(label)
    for label, _, _ in tarefas['core1']:
        if label != 'print' and label not in tarefas_unicas_core1:
            tarefas_unicas_core1.append(label)

    mapa_cores_core0 = {
        nome: paleta_azul[i % len(paleta_azul)]
        for i, nome in enumerate(tarefas_unicas_core0)
    }
    mapa_cores_core1 = {
        nome: paleta_vermelho[i % len(paleta_vermelho)]
        for i, nome in enumerate(tarefas_unicas_core1)
    }

    for i, core in enumerate(['core0', 'core1']):
        for label, start, duration in tarefas[core]:
            if label == 'print':
                cor = cor_print
            elif core == 'core0':
                cor = mapa_cores_core0.get(label, 'gray')
            else:
                cor = mapa_cores_core1.get(label, 'gray')

            ax.broken_barh([(start, duration)], (y_ticks[i], altura_barra), facecolors=cor)
            ax.text(start + duration / 2, y_ticks[i] + altura_barra / 2,
                    label, fontsize=7, va='center', ha='center', color='black')

    ax.set_yticks(y_ticks)
    ax.set_yticklabels(y_labels)
    ax.set_xlabel('Tick')
    ax.set_title('Diagrama de Gantt - Cores por Núcleo e por Tarefa')

    # Criar legenda
    legendas = []
    for label, cor in mapa_cores_core0.items():
        legendas.append(mpatches.Patch(color=cor, label=f'{label} (Core 0)'))
    for label, cor in mapa_cores_core1.items():
        legendas.append(mpatches.Patch(color=cor, label=f'{label} (Core 1)'))
    legendas.append(mpatches.Patch(color=cor_print, label='print'))

    ax.legend(handles=legendas,
              loc='lower right',
              bbox_to_anchor=(1, -0.25),
              ncol=4,
              frameon=False)

    plt.grid(True, axis='x', linestyle='--', alpha=0.7)
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    caminho_arquivo = 'entrada.txt'  # Altere conforme necessário
    dados = ler_arquivo(caminho_arquivo)
    tarefas = preparar_dados_gantt(dados, limite=70)
    plotar_gantt(tarefas)