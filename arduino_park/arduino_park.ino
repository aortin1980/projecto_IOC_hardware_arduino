// Definición de pines
const int trigPin = D5;  // Pin Trig del HC-SR04
const int echoPin = D6;  // Pin Echo del HC-SR04
const int ledVerde = D1; // LED Verde
const int ledAmarillo = D2; // LED Amarillo
const int ledRojo = D3; // LED Rojo

// Variables para la conexión Wi-Fi
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "WIFI_Mob";        // Tu SSID
const char* password = "3nmicasano3ntras"; // Tu contraseña

// URL de la API
const String url = "http://TU_API_SERVER/api/parking/use"; // Cambia TU_API_SERVER por la dirección de tu servidor
const String plaza = "1";  // Cambia esto por el número de plaza que quieras enviar

// Variable de distancia mínima para activar el LED rojo (en cm)
const int distanciaMinima = 100;

// Variable para controlar si ya se hizo la petición
bool apiEnviada = false;

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
}

void loop() {
  // Verificar conexión a Wi-Fi
  if (WiFi.status() != WL_CONNECTED) {
    // Si no hay conexión a internet, encender el LED amarillo
    digitalWrite(ledAmarillo, HIGH);
  } else {
    digitalWrite(ledAmarillo, LOW);
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
  
Serial.println("Distancia:"+distancia);
Serial.println("DistanciaMinima:"+distanciaMinima);

  // Control de LEDs según la distancia detectada
  if (distancia < distanciaMinima) {
    // Si se detecta algo dentro del umbral, encender el LED rojo
    digitalWrite(ledRojo, HIGH);
    digitalWrite(ledVerde, LOW);

    // Hacer llamada API si no se ha hecho aún
    if (!apiEnviada && WiFi.status() == WL_CONNECTED) {
      enviarApi();
      apiEnviada = true;  // Marcar que ya se envió la API para no repetirlo
    }
  } else {
    // Si no se detecta nada, encender el LED verde y reiniciar el estado de la API
    digitalWrite(ledRojo, LOW);
    digitalWrite(ledVerde, HIGH);
    apiEnviada = false;  // Resetear el estado para la próxima detección
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
  
  // Ciclar hasta que haya conexión Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
      conectarWiFi();
    digitalWrite(ledVerde, HIGH);
    delay(200);
    digitalWrite(ledVerde, LOW);
    digitalWrite(ledAmarillo, HIGH);
    delay(200);
    digitalWrite(ledAmarillo, LOW);
    digitalWrite(ledRojo, HIGH);
    delay(200);
    digitalWrite(ledRojo, LOW);
  }
}

void enviarApi() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;  // Crear objeto WiFiClient
    HTTPClient http;

    String fullUrl = url + plaza;
    http.begin(client, fullUrl);  // Iniciar conexión con WiFiClient y la URL

    http.addHeader("Content-Type", "application/json");  // Definir el header
    int httpResponseCode = http.POST("{}");  // Enviar petición POST con un body vacío

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
