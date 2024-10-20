#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// Definición de pines
const int trigPin = D5;  // Pin Trig del HC-SR04
const int echoPin = D6;  // Pin Echo del HC-SR04
const int ledVerde = D1; // LED Verde
const int ledAmarillo = D2; // LED Amarillo
const int ledRojo = D3; // LED Rojo
// Variables para la conexión Wi-Fi
const char* ssid = "WIFI_SEC";        // Tu SSID
const char* password = "3nmicasano3ntras";         // Tu contraseña

// URL de la API
const String url = "http://192.168.1.181:8080/api/parkings/status"; // Endpoint del servidor
const String plaza = "1";  // Número de plaza de estacionamiento
String HoraEntrada;
// Variable de distancia mínima para activar el LED rojo (en cm)
const int distanciaMinima = 100;

// Variables para controlar si ya se hicieron las peticiones
bool apiEnviada = false;
bool apiSalidaEnviada = false;



// Cliente NTP para obtener la hora actual
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);  // Zona horaria UTC (puedes ajustar el offset en segundos)

void setup() {
  // Inicializar comunicación serial
  Serial.begin(115200);

  // Configurar pines como salida y entrada
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledAmarillo, OUTPUT);
  pinMode(ledRojo, OUTPUT);

  // Test de LEDs al arrancar
  testLeds();

  // Intentar conectar a la red Wi-Fi
  conectarWiFi();

  // Iniciar cliente NTP
  timeClient.begin();
}

void loop() {
  // Verificar conexión a Wi-Fi
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(ledAmarillo, HIGH);  // Encender LED amarillo si no hay WiFi
  } else {
    digitalWrite(ledAmarillo, LOW);  // Apagar LED amarillo si hay WiFi
  }

  // Leer distancia desde el sensor HC-SR04
  long duracion;
  int distancia;

  // Generar pulso de activación en el trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Leer la duración del pulso en echoPin
  duracion = pulseIn(echoPin, HIGH);

  // Calcular la distancia en centímetros
  distancia = duracion * 0.034 / 2;
  
  Serial.println("Distancia: " + String(distancia));

  // Control de LEDs y lógica según la distancia detectada
  if (distancia < distanciaMinima) {
    // Si se detecta algo dentro del umbral, encender el LED rojo (espacio ocupado)
    digitalWrite(ledRojo, HIGH);
    digitalWrite(ledVerde, LOW);

    // Hacer llamada API para indicar espacio ocupado si no se ha hecho aún
    if (!apiEnviada && WiFi.status() == WL_CONNECTED) {
      enviarApiEspacioOcupado();
      apiEnviada = true;   // Marcar que ya se envió la API para el espacio ocupado
      apiSalidaEnviada = false; // Resetear para la próxima salida
    }
  } else {
    // Si no se detecta nada, encender el LED verde (espacio libre) y actualizar el historial
    digitalWrite(ledRojo, LOW);
    digitalWrite(ledVerde, HIGH);

    // Hacer llamada API para indicar que el espacio está libre si no se ha hecho aún
    if (!apiSalidaEnviada && WiFi.status() == WL_CONNECTED) {
      enviarApiEspacioLibre();
      apiSalidaEnviada = true;  // Marcar que ya se envió la API para la salida
      apiEnviada = false;   // Resetear para la próxima entrada
    }
  }

  // Pausa antes de la siguiente medición
  delay(500);
}

void conectarWiFi() {
  WiFi.begin(ssid, password);

  Serial.println();
  Serial.print("Conectando a la red WiFi: ");
  Serial.println(ssid);

  // Espera hasta que esté conectado o se cumpla el tiempo máximo
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 30) {
    delay(1000);
    Serial.print("Intento de conexión Wi-Fi ");
    Serial.println(intentos + 1);
    intentos++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Conectado a la red WiFi.");
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("No se pudo conectar a la red Wi-Fi.");
  }
}

// Función para hacer el test de LEDs al arrancar
void testLeds() {
  // Encender LEDs uno por uno
  digitalWrite(ledVerde, HIGH);
  delay(500);
  digitalWrite(ledVerde, LOW);
  
  digitalWrite(ledAmarillo, HIGH);
  delay(500);
  digitalWrite(ledAmarillo, LOW);
  
  digitalWrite(ledRojo, HIGH);
  delay(500);
  digitalWrite(ledRojo, LOW);
}

// Función para enviar API cuando el espacio está ocupado
void enviarApiEspacioOcupado() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;  // Crear objeto WiFiClient
    HTTPClient http;

    String fullUrl = url;  // La URL del servidor
    http.begin(client, fullUrl);  // Iniciar conexión con WiFiClient y la URL

    // Crear un objeto JSON
    StaticJsonDocument<256> doc; // Cambia el tamaño si es necesario
    doc["parkingSpaces"]["space_id"] = 1;
    doc["parkingSpaces"]["lot_id"] = 1;
    doc["parkingSpaces"]["space_number"] = 1;
    doc["parkingSpaces"]["is_reserved"] = false;  // Cambiar a booleano
    doc["parkingSpaces"]["is_occupied"] = true;   // Cambiar a booleano
    doc["parkingHistory"]["client_id"] = 4;
    doc["parkingHistory"]["lot_id"] = 1;


    // Formato de fecha y hora
    String entryDate = getDate(); // Ajusta según sea necesario
    String entryTime = getTime(); // Devuelve "HH:mm:ss"
    HoraEntrada = entryDate + " " + entryTime;
    doc["parkingHistory"]["entry_time"] = HoraEntrada; // Añadir la entrada en el formato correcto

    // Asegúrate de que exit_time esté en el formato correcto o sea null
    doc["parkingHistory"]["exit_time"] = nullptr; // o reemplaza con exitTime en formato correcto si se necesita

    doc["parkingHistory"]["total_cost"] = 0;

    // Serializar el objeto JSON a String
    String jsonPayload;
    serializeJson(doc, jsonPayload);

    Serial.println("Payload: " + jsonPayload);  // Imprimir el payload
    
    http.addHeader("Content-Type", "application/json");  // Definir el header
    int httpResponseCode = http.POST(jsonPayload);  // Enviar la solicitud POST con el JSON

    if (httpResponseCode > 0) {
      String response = http.getString();  // Obtener respuesta
      Serial.println(httpResponseCode);  // Mostrar código de respuesta
      Serial.println(response);  // Mostrar respuesta
    } else {
      Serial.print("Error en la petición POST: ");
      Serial.println(httpResponseCode);
    }
    http.end();  // Cerrar la conexión
  } else {
    Serial.println("No conectado a WiFi, no se puede enviar la solicitud");
  }
}

// Función para enviar API cuando el espacio está libre
void enviarApiEspacioLibre() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;  // Crear objeto WiFiClient
    HTTPClient http;

    // Formato de fecha y hora
    String entryDate = getDate(); // Ajusta según sea necesario
    String entryTime = getTime(); // Devuelve "HH:mm:ss"
    
    String fullUrl = url;  // La URL del servidor
    http.begin(client, fullUrl);  // Iniciar conexión con WiFiClient y la URL

    // Crear un objeto JSON
    StaticJsonDocument<256> doc; // Cambia el tamaño si es necesario
    doc["parkingSpaces"]["space_id"] = 1;
    doc["parkingSpaces"]["lot_id"] = 1;
    doc["parkingSpaces"]["space_number"] = 1;
    doc["parkingSpaces"]["is_reserved"] = false;  // Cambiar a booleano
    doc["parkingSpaces"]["is_occupied"] = false;   // Cambiar a booleano
    doc["parkingHistory"]["client_id"] = 4;
    doc["parkingHistory"]["lot_id"] = 1;
    doc["parkingHistory"]["entry_time"] = HoraEntrada; // Guardada.
    doc["parkingHistory"]["exit_time"] = entryDate + " " + entryTime; // Obtener tiempo en formato ISO
    doc["parkingHistory"]["total_cost"] = 100;      // Ajusta el costo según corresponda

    // Serializar el objeto JSON a String
    String jsonPayload;
    serializeJson(doc, jsonPayload);

    Serial.println("Payload: " + jsonPayload);  // Imprimir el payload
    http.addHeader("Content-Type", "application/json");  // Definir el header
    int httpResponseCode = http.POST(jsonPayload);  // Enviar la solicitud POST con el JSON

    if (httpResponseCode > 0) {
      String response = http.getString();  // Obtener respuesta
      Serial.println(httpResponseCode);  // Mostrar código de respuesta
      Serial.println(response);  // Mostrar respuesta
    } else {
      Serial.print("Error en la petición POST: ");
      Serial.println(httpResponseCode);
    }
    http.end();  // Cerrar la conexión
  } else {
    Serial.println("No conectado a WiFi, no se puede enviar la solicitud");
  }
}

// Función para obtener la hora actual desde el servidor NTP
String getTime() {
  timeClient.update();
  return timeClient.getFormattedTime(); // Asegúrate de que esto esté formateado correctamente
}

// Función para obtener la fecha actual en formato "YYYY-MM-DD"
String getDate() {
  time_t rawTime = timeClient.getEpochTime(); // Obtener tiempo en segundos desde la época
  struct tm * timeInfo = localtime(&rawTime); // Convertir a estructura tm

  // Formatear la fecha como "YYYY-MM-DD"
  char buffer[11];
  sprintf(buffer, "%04d-%02d-%02d", timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday);
  return String(buffer);
}
