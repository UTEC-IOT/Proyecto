#include <Keypad.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define ROW_NUM               4 
#define COLUMN_NUM            4 
#define DETECTA_PRESENCIA     1
#define INGRESA_CLAVE         2
#define ENVIA_CLAVE           3
#define ESPERA_CONFIRMACION   4
#define ABRE_PUERTA           5

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte pin_rows[ROW_NUM]      = {19, 18, 5, 17}; // GPIO19, GPIO18, GPIO5, GPIO17 connect to the row pins
byte pin_column[COLUMN_NUM] = {16, 4, 0, 2};   // GPIO16, GPIO4, GPIO0, GPIO2 connect to the column pins

Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

const char* ssid = "IOT";     // Nombre de la red WiFi
const char* password = "jorge123"; // Contraseña

String pswd = ""; // string para guardar los valores ingresados en el pad
int estado = 1; // Estado inicial para los cambios de estado

unsigned long startTime; // Tiempo en el que inicia el estado de escribir contraseña
unsigned long timeoutDuration = 10000; // 10 segundos
unsigned long startEspera;
unsigned long timeoutEspera = 10000; // 10 segundos

// Valores necesarios para conexión con MQTT
const char* mqttServer = "192.168.194.14";
const int mqttPort = 1883;
const char* mqttUser = "usuario_mqtt";
const char* mqttPassword = "contraseña_mqtt";
const char* topicClaveEnviada = "clave-enviada";
const char* topicEstadoClave = "estado-clave";
WiFiClient espClient;
PubSubClient client(espClient);

int respuesta = 0; // Flag que verifica que la contraseña sea correcta

void setup() {
  pinMode(26, INPUT);  // Entrada del PIR
  pinMode(25, OUTPUT); // LED que indica que puedes ingresar la contraseña
  pinMode(21, OUTPUT); // LED que indica que se accedió a la casa
  Serial.begin(9600);  // Baudios establecidos para visualizar lo que se imprime
  
  // Conexión a WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a la red WiFi...");
  }
  Serial.println("Conexión WiFi estable");

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  while (!client.connected()) {
    Serial.println("Conectando al servidor MQTT...");

    if (client.connect("NodeMCU", mqttUser, mqttPassword)) {
      Serial.println("Conexión al servidor MQTT establecida!");
      client.subscribe(topicEstadoClave);
    } else {
      Serial.print("Error al conectar al servidor MQTT. Estado: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void loop() {
  switch (estado) {
    case DETECTA_PRESENCIA: {
      if (!client.connected()) {
        reconnect();
      }
      client.loop();
      // Lectura del PIR
      int pinState = digitalRead(26);
      // Si detecta presencia
      if (pinState) {
        Serial.println("Se detectó presencia");
        pswd = "";                // Limpia el buffer de la contraseña
        estado = INGRESA_CLAVE;   // Cambio de estado a ingresar la clave
        // Tiempo inicial para mantener el estado de ingresar clave activo
        startTime = millis();
        Serial.println(startTime);
        digitalWrite(25, HIGH); // Se enciende el LED que indica el estado de ingresar la clave
        break;
      }
      break;
    }
    case INGRESA_CLAVE: {
      if (!client.connected()) {
        reconnect();
      }
      client.loop();
      char key = keypad.getKey(); // Lectura del pad
      // Verifica si ya pasó el lapso de tiempo del estado de ingresar contraseña
      if (millis() - startTime >= timeoutDuration) {
        digitalWrite(25, LOW); // Se apaga el LED del estado de ingresar contraseña
        Serial.println("Tiempo ha pasado");
        estado = DETECTA_PRESENCIA; // Cambio de estado al inicial de detectar presencia
        break;
      }
      // Verifica si se ingresó un valor en el pad
      if (key) {
        digitalWrite(21, HIGH);
        delay(300);
        digitalWrite(21, LOW);
        // Botón que indica que se enviará la clave
        if (key == '*') {
          Serial.println("Enviando clave");
          digitalWrite(25, LOW); // Se apaga el LED del estado de ingresar clave
          estado = ENVIA_CLAVE; // Cambio de estado a enviar contraseña
          break;
        } else if (key == '#') { // Botón para resetear la contraseña
          pswd = "";
        }
        // Se concatena el valor del pad a la contraseña
        pswd += key;
        Serial.println(pswd);
      }
      break;
    }
    case ENVIA_CLAVE: {
      Serial.println("Enviando contraseña");
      if (!client.connected()) {
        reconnect();
      }
      client.loop();

      client.publish(topicClaveEnviada, pswd.c_str());
      startEspera = millis();
      estado = ESPERA_CONFIRMACION;
      break;
    }
    case ESPERA_CONFIRMACION: {
      if (!client.connected()) {
        reconnect();
      }
      client.loop();
      // Timeout, encender el led por cierto tiempo indicando que no se pudo abrir
      if (millis() - startEspera == timeoutEspera) {
        Serial.println("Tiempo ha pasado");
        estado = DETECTA_PRESENCIA; // Cambio de estado al inicial de detectar presencia
        break;
      }
      if (respuesta == 0) {
        break;
      } else if(respuesta == 1){
        Serial.println("Contraseña correcta");
        respuesta = 0;
        estado = ABRE_PUERTA;
        break;
      } else {
        // Timeout, encender el led por cierto tiempo indicando que no se pudo abrir
        Serial.println("Contraseña incorrecta");
        estado = INGRESA_CLAVE;
        break;
      }
    }
    case ABRE_PUERTA: {
      // Se abre satisfactoriamente la puerta y se indica con el LED que se mantendrá prendido por 10 segundos y luego se vuelve al estado inicial
      Serial.println("Puerta abierta por 10 segundos");
      digitalWrite(21, HIGH);
      delay(10000);
      estado = DETECTA_PRESENCIA;
      digitalWrite(21, LOW);
      break;
    }
    default: {
      Serial.println("Hola");
      break;
    }
  }
}
