/*
ESP32 Webserver con interfaz HTTP y Client UDP
Gabriel Carrera - 21216
Estuardo Castillo - 21559

Código para montar un servidor HTTP en disipositivo IoT. En este caso se escoge usar ESP32
considerar que el protocolo usado no es seguro para ambientes de producción pues el texto
enviado es plano y sin encriptación. Usar como prueba de concepto. 
*/

#include <WiFi.h> //Librería para utilizar WiFi
#include <WiFiUdp.h> //Librería para utilziar UDP
//-------------------------CAMBIAR SEGÚN ENTORNO----------------------------------------
const char* ssid = "NETGEAR_iemtbmUVG"; //nombre de la red
const char* password = "iemtbmUVG"; //Contraseña

const char * udpAddress = "10.0.0.11"; //IP address del RTU1
const char * udpAddress2 = "10.0.0.15"; //IP address del RTU2

const int udpPort = 2006; //Puerto local
//-------------------------------------------------------------------------------------

boolean connected = false; //Booleano para ver si está conectado
WiFiUDP udp; //Crear clases
WiFiServer server(80); //Puerto 80 (default)

void setup() {
  Serial.begin(115200); //Serial para visualizar en la terminal
  connectToWiFi(ssid, password); //Conectar al WifI

  // Connect to Wi-Fi
  WiFi.begin(ssid, password); //Iniciar conexión
  while (WiFi.status() != WL_CONNECTED) { //Esperar a que se conecte
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  Serial.println("Connected to WiFi"); //Indicar conexión
  Serial.print("IP Address: "); //Mostrar IP del ESP para la páginca web
  Serial.println(WiFi.localIP()); //Obtener IP
  
  // Start the server
  server.begin(); //Iniciar el websereer
  Serial.println("Server started"); //Conecteado
  if(connected) //Conexión
      {
        udp.beginPacket(udpAddress,udpPort);
        udp.printf("RTU1");
        udp.endPacket();
        delay(10);
        udp.beginPacket(udpAddress2,udpPort);
        udp.printf("RTU2");
        udp.endPacket();
      }
}

void loop() {
  // Check for a client
  WiFiClient client = server.available(); //Mantener el server corriendo
  
  if (client) {
    Serial.println("New client connected"); //Si un existe un cliente haciendo un request
    
    // Read the request
    String request = client.readStringUntil('\r'); //Leer
    Serial.println(request);
    client.flush(); //Limpiar

    //construir pagina en HTML
    //Se emplean requeests en HTML para realizar tareas especificas en el código de arduino
    //por ejemplo, la pagina tiene eventos para enviar requests y es dentro del código de arduino (no HTML) que se
    //envían los mensajes por UDP hacia RTUs

      String html = "<html><head>";
      //encabeezado y estilos
      html += " <meta charset=\"UTF-8\">";
      html += " <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
      html += "<style>body {font-family: 'Helvetica Neue', Helvetica, Arial, sans-serif; background: linear-gradient(to top, #ff7e5f, #feb47b); margin: 20px; display: flex; flex-direction: column; align-items: center;}";
      html += "h1 { color: #ffffff; text-align: center; margin-bottom: 20px; }";
      html += ".container { display: flow; width: 35%; max-width: 800px; justify-content: space-between; align-items: center; }";

      // Se usan contenedores (pensar como cajas) para elementos de la UI
      html += ".slider-container { display: flex; flex-direction: column; align-items: center; padding: 20px; border-radius: 15px; background-color: #000000; opacity: 0.5; box-shadow: 0 4px 8px rgba(0,0,0,0.1); position: relative; width: 300px; height: 50px; }";

      // Slider track styling
      html += ".slider { -webkit-appearance: none; width: 100%; height: 4px; background: linear-gradient(to right, #ff7e5f, #feb47b, #6a82fb); border-radius: 2px; cursor: pointer; }";
      html += ".slider::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 16px; height: 16px; background: #ff7e5f; border: none; border-radius: 50%; margin-top: -6px; cursor: pointer; transition: background 0.3s ease; }";
      html += ".slider::-webkit-slider-thumb:hover { background: #feb47b; }";

      html += ".slider::-moz-range-thumb { width: 16px; height: 16px; background: #ff7e5f; border: none; border-radius: 50%; cursor: pointer; transition: background 0.3s ease; }";
      html += ".slider::-moz-range-thumb:hover { background: #feb47b; }";

      // LED buttons container
      html += ".leds { display: flex; flex-direction: row; align-items: flex-end; }";
      html += ".led { border: 2px solid #ccc; border-radius: 8px; padding: 10px; margin: 10px 0;  width: 150px; text-align: center; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
      html += ".led button { width: 100%; padding: 10px; border: none; border-radius: 6px; font-family: 'Helvetica Neue', Helvetica, Arial, sans-serif; font-weight: bold; cursor: pointer; }";
      html += ".led1 { background-color: #000000; color: #fff; opacity: 0.5;}";
      html += ".led2 { background-color: #000000; color: #fff; opacity: 0.5;}";

      html += "</style></head><body>";
      html += "<h1>Interfaz para elemento IoT</h1>";
      html += "<div class='container'>";

      // Slider
      html += "<div class='slider-container'>";
      html += "<input type='range' min='-250' max='1250' value='0' id='servoSlider' class='slider' onchange='updateServo(this.value)'>";
      html += "<p id='sliderV' style='margin-top:15px; color: white;font-weight: bold;'>Angle: 0</p>";
      html += "</div>";

      //botones
      html += "<div class='leds'>";
      html += "<div class='led led1'>";
      html += "<button onclick='toggleLED()'>Toggle LED 1</button>";
      html += "</div>";
      html += "<div class='led led2'>";
      html += "<button onclick='toggleLED2()'>Toggle LED 2</button>";
      html += "</div>";
      //funciones en eventos
      html += "</div>";
      html += "<script>";
      html += "const slider = document.getElementById('servoSlider');";
      html += "const sliderV = document.getElementById('sliderV');";
      html += "sliderV.innerHTML = 'Angle: ' + mapServoValue(slider.value);";
      html += "slider.addEventListener('input', () => { sliderV.innerHTML = 'Angle: ' + mapServoValue(slider.value); });";
      html += "function mapServoValue(value) { return Math.round(map(value, -250, 1250, 0, 180)); }";
      //se decide hacer este mapeo en el envio de datos, la RTU interpreta este rango y lo convierte de nuevo a angulo
      html += "function map(value, inMin, inMax, outMin, outMax) { return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin; }";
      //evento toggle con request toggle para RTU1 y led2 para RTU2, no funciona si tienen misma raiz de mensaje (ejemplo, toggle1 y toggle2)
      html += "function toggleLED() { var xhr = new XMLHttpRequest(); xhr.open('GET', '/toggle', true); xhr.send(); }";
      html += "function toggleLED2() { var xhr = new XMLHttpRequest(); xhr.open('GET', '/led2', true); xhr.send(); }";
      html += "function updateServo(sliderValue) { var xhr = new XMLHttpRequest(); xhr.open('GET', '/servo?value=' + sliderValue, true); xhr.send(); }";
      html += "</script></body></html>";

    //Request del servidor, luego se envian por UDP para la dirección especificada
    if (request.indexOf("/toggle") != -1) //toggle para led 1
    {
        if(connected)
        {
          udp.beginPacket(udpAddress,udpPort); //Al RTU1
          udp.printf("RTU1: led_IoT"); //Encender Led Iot
          udp.endPacket(); //Finalizar
        }
    }

    if (request.indexOf("/led2") != -1) //toggle para led 2
    {
        if(connected)
        {
          udp.beginPacket(udpAddress2,udpPort); //Al RTU2
          udp.printf("RTU2: led_IoT"); //Encender Led Iot del RTU2
          udp.endPacket();
        }
    }

    if (request.indexOf("/servo") != -1) //Si es servo...
    {
      int sliderValue = request.substring(request.indexOf('=') + 1).toInt(); //Obtener el valor del slider en la página
      char buffer[30]; //Definir un buffer
      sprintf(buffer, "RTU1Servo:%d", sliderValue); //Hacer una string para enviar al RTU1
      Serial.println(buffer);
        if(connected)
        {
          udp.beginPacket(udpAddress,udpPort); //Al RTU1
          udp.printf(buffer); //Enviar string con ángulo del servo
          udp.endPacket();
        }
    }
  
    //Respuesta al cliente parte del protocolo HTTP
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println(html);
    
    //Cerrar cpnexión
    delay(1);
    Serial.println("Client disconnected");
    client.stop();
  }
}

//Función para conectar
void connectToWiFi(const char * ssid, const char * pwd){
  Serial.println("Connecting to WiFi network: " + String(ssid));
  WiFi.disconnect(true); //Borrar conexión anterior
  WiFi.onEvent(WiFiEvent); //Al conectarse ejecutar WiFiEvent
  WiFi.begin(ssid, pwd); //Iniciar
  Serial.println("Waiting for WIFI connection...");
}

void WiFiEvent(WiFiEvent_t event){
    switch(event) {
      case ARDUINO_EVENT_WIFI_STA_GOT_IP:
          //When connected set 
          Serial.print("WiFi connected! IP address: ");
          Serial.println(WiFi.localIP());  
          //initializes the UDP state
          //This initializes the transfer buffer
          udp.begin(WiFi.localIP(),udpPort);
          connected = true;
          break;
      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
          Serial.println("WiFi lost connection");
          connected = false;
          break;
      default: break;
    }
}