import serial
import time

# CONFIGURAÇÕES
PORTA = "COM3"     # altere para a porta correta
BAUD = 115200
ARQUIVO = "dados_serial.txt"

def reset_esp(ser: serial.Serial, pulse_time=0.1):
    """Reinicia o ESP32 via RTS/DTR, se o hardware suportar."""
    try:
        ser.setRTS(True)   # força reset (EN = 0)
        ser.setDTR(False)  # modo normal
        time.sleep(pulse_time)
        ser.setRTS(False)  # libera reset (EN = 1)
        time.sleep(0.1)
    except Exception as e:
        print("Falha ao aplicar pulso de reset:", e)

def main():
    ser = serial.Serial()
    ser.port = PORTA
    ser.baudrate = BAUD
    ser.timeout = 0.5
    ser.dtr = False
    ser.rts = False

    try:
        ser.open()
    except Exception as e:
        print("Erro ao abrir porta serial:", e)
        return

    print("Porta aberta:", PORTA)
    print("Reiniciando ESP...")
    reset_esp(ser)
    time.sleep(1)  # aguarda inicialização do ESP

    print("Iniciando leitura de dados... (Ctrl+C para parar)")

    with open(ARQUIVO, "w", encoding="utf-8") as f:
        try:
            while True:
                linha_bytes = ser.readline()
                if linha_bytes:
                    linha = linha_bytes.decode("utf-8", errors="ignore").strip()
                    print(linha)
                    f.write(linha + "\n")
                    f.flush()
                else:
                    time.sleep(0.1)
        except KeyboardInterrupt:
            print("\nLeitura interrompida pelo usuário.")
        finally:
            ser.close()
            print("Serial fechada.")

if __name__ == "__main__":
    main()
