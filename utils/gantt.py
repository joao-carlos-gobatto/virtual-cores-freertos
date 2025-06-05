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
    
    for idx, entrada in enumerate(dados[:limite]):
        tick0, task0 = entrada['core0']
        tick1, task1 = entrada['core1']
        
        tarefas['core0'].append((f"#{idx+1} {task0}", tick0, 1))
        tarefas['core1'].append((f"#{idx+1} {task1}", tick1, 1))
    
    return tarefas

def plotar_gantt(tarefas):
    fig, ax = plt.subplots(figsize=(12, 3))

    # Cores alternadas
    cores_core0 = ['skyblue', 'dodgerblue']
    cores_core1 = ['salmon', 'indianred']

    y_labels = ['Core 0', 'Core 1']
    y_ticks = [3, 1]
    altura_barra = 0.8

    for i, core in enumerate(['core0', 'core1']):
        for idx, tarefa in enumerate(tarefas[core]):
            label, start, duration = tarefa
            if core == 'core0':
                cor = cores_core0[idx % len(cores_core0)]
            else:
                cor = cores_core1[idx % len(cores_core1)]
            
            ax.broken_barh([(start, duration)], (y_ticks[i], altura_barra), facecolors=cor)
            ax.text(start + 0.3, y_ticks[i] + altura_barra + 0.1, label, fontsize=7, va='bottom', ha='left')

    ax.set_yticks(y_ticks)
    ax.set_yticklabels(y_labels)
    ax.set_xlabel('Tick')
    ax.set_title('Diagrama de Gantt - Primeiras 10 Tarefas (Cores Alternadas)')

    # Legenda geral
    legenda = [mpatches.Patch(color='skyblue', label='Core 0'),
               mpatches.Patch(color='salmon', label='Core 1')]
    ax.legend(handles=legenda)

    plt.grid(True, axis='x', linestyle='--', alpha=0.7)
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    caminho_arquivo = 'entrada.txt'  # Altere conforme necessário
    dados = ler_arquivo(caminho_arquivo)
    tarefas = preparar_dados_gantt(dados, limite=10)
    plotar_gantt(tarefas)
