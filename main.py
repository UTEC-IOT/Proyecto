import paho.mqtt.client as mqtt
import requests

TOKEN = "6217735283:AAG2MWraTpAyVyLvC3eCtd4jg7fNr1GZXV0"
chat_id = "6168857588"
message1 = "Hola, te notifico que alguien intento acceder por el cerrojo electronico"
message2 = "Hola, te notifico que alguien accedio a tu hogar"
url1 = f"https://api.telegram.org/bot{TOKEN}/sendMessage?chat_id={chat_id}&text={message1}"
url2 = f"https://api.telegram.org/bot{TOKEN}/sendMessage?chat_id={chat_id}&text={message2}"

mqttBroker = "192.168.194.14"
mqttPort = 1883
topicClaveEnviada = "clave-enviada"
topicEstadoClave = "estado-clave"
password = "1"

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Conexión al servidor MQTT establecida!")
        client.subscribe(topicClaveEnviada)
    else:
        print("Error al conectar al servidor MQTT. Código de error: " + str(rc))

def on_message(client, userdata, msg):
    print("Mensaje recibido desde el topic: " + msg.topic)
    if msg.topic == topicClaveEnviada:
        print(msg.payload.decode())
        if msg.payload.decode() == password:
            print("Contraseña correcta")
            print(requests.get(url2).json()) # this sends the message
            client.publish(topicEstadoClave, "OK")
        else:
            print("Contraseña incorrecta")
            print(requests.get(url1).json()) # this sends the message
            client.publish(topicEstadoClave, "NO")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect(mqttBroker, mqttPort)

while True:
    client.loop()






