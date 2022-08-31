# import sys
from time import sleep
import random
import csv
from time import localtime, strftime

# ModBus
# import modbus_tk
import modbus_tk.defines as cst
from modbus_tk import modbus_rtu
from modbus_tk import modbus_tcp
import serial

# MQTT
from paho.mqtt import client as mqtt_client

PORT = "com5"
broker = '191.52.39.88'
port = 1883
topic_tensaobat = "/tve/tensaobat"  # tensaobat (mV)
topic_tempbat = "/tve/tempbat"  # temperatura Bateria °C *100
topic_corrente = "/tve/corrente"  # corrente (mA)
topic_velocidade = "/tve/velocidade"  # velocidade Km/h *10
topic_tempvei = "/tve/tempvei"  # temperatura Veículo °C *100

# generate client ID randomly
client_id = f'Kersch-{random.randint(0, 1000)}'
username = ''
password = ''


def connect_mqtt():
    def on_connect(rc):  # def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n", rc)

    # Set Connecting Client ID
    client = mqtt_client.Client(client_id)
    client.username_pw_set(username, password)
    client.on_connect = on_connect
    client.connect(broker, port)
    return client


def r():
    return random.randrange(0, 40000)


def main():
    """main"""
    # logger = modbus_tk.utils.create_logger(name="console", record_format="%(message)s")
    str_data = strftime("%Y-%m-%d %H-%M-%S", localtime())

    # Create the ModBus servers
    global f
    porta = serial.Serial(PORT)
    porta.baudrate = 19200
    server_rtu = modbus_rtu.RtuServer(porta, error_on_missing_slave=False)
    server_tcp = modbus_tcp.TcpServer()

    # MQTT
    client = connect_mqtt()

    try:
        print("running...")

        server_rtu.start()
        server_tcp.start()

        slave_1 = server_rtu.add_slave(1)
        slave_1.add_block('0', cst.HOLDING_REGISTERS, 0, 100)

        slave_2 = server_tcp.add_slave(1)
        slave_2.add_block('0', cst.HOLDING_REGISTERS, 0, 100)

        f = open('./dados ' + str_data + '.csv', 'w', newline='')
        writer = csv.writer(f)

        titulos = ['Hora', 'Vc1', 'Vc2', 'Vc3', 'Vc4', 'Vc5', 'Vc6', 'Vc7', 'Vc8', 'Vc9', 'Vc10', 'Vc11', 'Vc12',
                   'Temp Bat', 'Velo', 'Corr', 'Temp Vei', 'VBat', 'Res', 'Res', 'cont']
        writer.writerow(titulos)

        passo = 0

        while True:
            # slave_1.set_values('0', 0, (
            #    r(), r(), r(), r(), r(), r(), r(), r(), r(), r(), r(), r(), r(), r(), r(), r(), r(), r(), r(), r()))
            valores = list(slave_1.get_values('0', 0, 20))
            # print(valores)
            valores_ajustados = []
            # pesos = [1.0495, 1.0125, 1.0000, 0.9438, 0.9882, 0.9882, 1.1351, 0.8750, 1.1200, 0.9438]
            pesos = [0.997, 0.975, 0.950, 0.92, 0.96, 0.939, 1.08, 0.86, 1.1, 0.897]
            cont = 0
            while cont < 20:
                if cont < 10:
                    valores_ajustados.append(int(valores[cont] * pesos[cont]))
                else:
                    valores_ajustados.append(int(valores[cont]))
                cont = cont + 1
            print(valores_ajustados)
            slave_2.set_values('0', 0, valores_ajustados)
            lista = [strftime("%H:%M:%S", localtime())]
            lista = lista + valores

            writer.writerow(lista)

            # publica no MQTT
            if passo == 1:
                # Publica a tesão total da bateria
                vbat = float(valores[16]) / 1000  # slave_1.get_values('0', 0, 1)[0]) / 1000
                result = client.publish(topic_tensaobat, vbat)
                status = result[0]
                if status != 0:
                    print(f"Failed to send message to topic {topic_tensaobat}")

                # Publica a temperatura da bateria
                temp = float(valores[12]) / 100  # slave_1.get_values('0', 12, 1)[0]) / 100
                result = client.publish(topic_tempbat, temp)
                status = result[0]
                if status != 0:
                    print(f"Failed to send message to topic {topic_tempbat}")

                # Publica a corrente do motor
                i = float(valores[14]) / 10  # float(slave_1.get_values('0', 14, 1)[0]) / 10
                result = client.publish(topic_corrente, i)
                status = result[0]
                if status != 0:
                    print(f"Failed to send message to topic {topic_corrente}")

                # Publica a velocidade do veículo
                velo = 0.0  # float(valores[13]) / 10  # float(slave_1.get_values('0', 14, 1)[0]) / 10
                result = client.publish(topic_velocidade, velo)
                status = result[0]
                if status != 0:
                    print(f"Failed to send message to topic {topic_velocidade}")

                # Publica a temperatura do veículo
                tempv = float(valores[15]) / 100  # float(slave_1.get_values('0', 16, 1)[0]) / 100
                result = client.publish(topic_tempvei, tempv)
                status = result[0]
                if status != 0:
                    print(f"Failed to send message to topic {topic_tempvei}")

                passo = 0
            else:
                passo = 1
            sleep(0.5)

    finally:
        server_rtu.stop()
        server_tcp.stop()
        f.close()


if __name__ == "__main__":
    main()

