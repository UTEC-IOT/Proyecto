import paho.mqtt.client as mqtt
import requests

# Token y chat_id de Telegram para enviar notificaciones
TOKEN = "6217735283:AAG2MWraTpAyVyLvC3eCtd4jg7fNr1GZXV0"
chat_id = "6168857588"

# Mensajes de notificación
message1 = "Hola, te notifico que alguien intentó acceder por el cerrojo electrónico"
message2 = "Hola, te notifico que alguien accedió a tu hogar"

# URLs de las solicitudes HTTP para enviar mensajes a través de la API de Telegram
url1 = f"https://api.telegram.org/bot{TOKEN}/sendMessage?chat_id={chat_id}&text={message1}"
url2 = f"https://api.telegram.org/bot{TOKEN}/sendMessage?chat_id={chat_id}&text={message2}"

# Configuración del servidor MQTT
mqttBroker = "192.168.194.14"
mqttPort = 1883
topicClaveEnviada = "clave-enviada"
topicEstadoClave = "estado-clave"
password = "1"

# Función que se ejecuta al conectarse al servidor MQTT
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Conexión al servidor MQTT establecida!")
        client.subscribe(topicClaveEnviada)
    else:
        print("Error al conectar al servidor MQTT. Código de error: " + str(rc))

# Función que se ejecuta al recibir un mensaje desde el servidor MQTT
def on_message(client, userdata, msg):
    print("Mensaje recibido desde el topic: " + msg.topic)
    if msg.topic == topicClaveEnviada:
        print(msg.payload.decode())
        if msg.payload.decode() == password:
            print("Contraseña correcta")
            print(requests.get(url2).json())  # Envía el mensaje de notificación 2 a Telegram
            client.publish(topicEstadoClave, "OK")  # Publica "OK" en el topic estado-clave
        else:
            print("Contraseña incorrecta")
            print(requests.get(url1).json())  # Envía el mensaje de notificación 1 a Telegram
            client.publish(topicEstadoClave, "NO")  # Publica "NO" en el topic estado-clave

# Crear instancia del cliente MQTT y configurar las funciones de callback
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

# Conexión al servidor MQTT
client.connect(mqttBroker, mqttPort)

# Bucle principal para mantener la conexión y procesar los mensajes recibidos
while True:
    client.loop()
