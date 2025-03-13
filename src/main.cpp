#include <M5StickCPlus2.h>
#include <BluetoothSerial.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "Poco X3 Pro";
const char* password = "bobr kurva";
const char* udpAddress = "192.168.207.166";
const int udpPort = 12345;

BluetoothSerial SerialBT;

// Глобальные переменные
bool isBluetoothEnabled = true;
uint16_t textColor = WHITE;
uint16_t backgroundColor = BLACK;
int currentTextSize = 1;
WiFiUDP Udp;
AsyncWebServer server(80);
bool webServerEnabled = false;

// Функции для обработки команд
void processCommand(String command);
void displayMessage(String message);
void clearScreen();
void scrollText();
void scanWiFiNetworks();
void sendUDPMessage(String message);
void startWebServer();
void handleRoot(AsyncWebServerRequest *request);
void handleCommand(AsyncWebServerRequest *request);
void sendHTTPGetRequest(String url);
void autoReconnectWiFi();

void setup() {
    // Инициализация
    M5.begin();
    M5.Lcd.setRotation(3);
    M5.Lcd.fillScreen(backgroundColor);
    M5.Lcd.setTextColor(textColor);
    M5.Lcd.setTextSize(currentTextSize);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        M5.Lcd.print(".");
    }
    M5.Lcd.println("\nWiFi Connected!");

    SerialBT.begin("M5StickCPlus2_OS");
    displayMessage("Bluetooth: M5StickCPlus2_OS");
    displayMessage("OS Initialized");

    Udp.begin(udpPort);

    server.on("/", HTTP_GET, handleRoot);
    server.on("/cmd", HTTP_GET, handleCommand);
}

void loop() {
    if (isBluetoothEnabled && SerialBT.available()) {
        String command = SerialBT.readStringUntil('\n');
        command.trim();
        displayMessage("> " + command);
        processCommand(command);
    }

    M5.update();

    autoReconnectWiFi();
}

// Функция для обработки команд
void processCommand(String command) {
    if (command == "help") {
        String helpMessage = "Available commands:\n";
        helpMessage += "help - Show this message\n";
        helpMessage += "clear - Clear the screen\n";
        helpMessage += "info - Show device info\n";
        helpMessage += "text <message> - Display text on screen\n";
        helpMessage += "brightness <value> - Set screen brightness (0-255)\n";
        helpMessage += "toggle_bt - Toggle Bluetooth\n";
        helpMessage += "size <value> - Set text size\n"; 
        helpMessage += "scan - Scan available WiFi networks\n"; 
        helpMessage += "udp <message> - Send UDP message\n"; 
        helpMessage += "start_web - Start web server\n"; 
        helpMessage += "get <url> - Send HTTP GET request\n"; 
        helpMessage += "wifi_status - Check WiFi connection status\n"; 
        SerialBT.println(helpMessage);
    } else if (command == "clear") {
        clearScreen();
    } else if (command == "info") {
        String infoMessage = "M5StickC Plus 2 OS v1.0\n";
        infoMessage += String("Bluetooth: ") + String(isBluetoothEnabled ? "Connected" : "Disconnected") + "\n";
        infoMessage += String("WiFi Status: ") + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected") + "\n";
        SerialBT.println(infoMessage);
    } else if (command.startsWith("text ")) {
        String text = command.substring(5); 
        displayMessage(text);
    } else if (command.startsWith("brightness ")) {
        int value = command.substring(10).toInt();
        if (value >= 0 && value <= 255) {
            M5.Lcd.setBrightness(value); 
            displayMessage("Brightness set to: " + String(value));
        } else {
            SerialBT.println("Error: Brightness value must be between 0 and 255.");
        }
    } else if (command == "toggle_bt") {
        isBluetoothEnabled = !isBluetoothEnabled;
        if (isBluetoothEnabled) {
            SerialBT.begin("M5StickCPlus2_OS");
            displayMessage("Bluetooth enabled.");
        } else {
            SerialBT.end();
            displayMessage("Bluetooth disabled.");
        }
    } else if (command.startsWith("size ")) { 
        int newSize = command.substring(5).toInt();
        if (newSize > 0 && newSize <= 4) {
            currentTextSize = newSize;
            M5.Lcd.setTextSize(currentTextSize);
            displayMessage("Text size set to: " + String(newSize));
        } else {
            SerialBT.println("Error: Text size must be between 1 and 4.");
        }
    } else if (command == "scan") {
        scanWiFiNetworks();
    } else if (command.startsWith("udp ")) {
        String message = command.substring(4);
        sendUDPMessage(message);
    } else if (command == "start_web") {
        startWebServer();
    } else if (command.startsWith("get ")) {
        String url = command.substring(4);
        sendHTTPGetRequest(url);
    } else if (command == "wifi_status") {
        String status = (WiFi.status() == WL_CONNECTED) ? "Connected" : "Disconnected";
        displayMessage("WiFi Status: " + status);
    } else {
        SerialBT.println("Unknown command: " + command);
    }
}

void clearScreen() {
    M5.Lcd.fillScreen(backgroundColor);
    M5.Lcd.setCursor(0, 0);
}

void displayMessage(String message) {
    int y = M5.Lcd.getCursorY();
    int screenHeight = M5.Lcd.height();

    if (y + currentTextSize * 8 > screenHeight) {
        scrollText();
    }

    M5.Lcd.println(message); 
    SerialBT.println(message); 
}

void scrollText() {
    M5.Lcd.scroll(-8, 0); 
    M5.Lcd.fillRect(0, M5.Lcd.height() - 8, M5.Lcd.width(), 8, backgroundColor); 
}

void scanWiFiNetworks() {
    int n = WiFi.scanNetworks();
    if (n == -1) {
        displayMessage("Ошибка сканирования");
        return;
    }
    displayMessage("Найдены сети (" + String(n) + "):");
    for (int i = 0; i < n; i++) {
        displayMessage(String(i+1) + ". " + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + "dBm)");
    }
}

void sendUDPMessage(String message) {
    Udp.beginPacket(udpAddress, udpPort);
    Udp.write((uint8_t*)message.c_str(), message.length()); 
    Udp.endPacket();
    displayMessage("UDP: " + message + " → " + udpAddress);
}

void startWebServer() {
    if (!webServerEnabled) {
        server.begin();
        webServerEnabled = true;
        displayMessage("Веб-сервер запущен!");
    }
}

void handleRoot(AsyncWebServerRequest *request) {
    request->send(200, "text/html", 
        "<h1>M5StickC OS</h1>"
        "<a href='/cmd?cmd=clear'>Очистить экран</a><br>"
        "<a href='/cmd?cmd=text%20Hello%20World'>Приветствие</a><br>"
        "<a href='/cmd?cmd=scan'>Сканировать сети</a><br>"
        "<a href='/cmd?cmd=start_web'>Запустить веб-сервер</a><br>"
        "<a href='/cmd?cmd=wifi_status'>Статус WiFi</a>"
    );
}

void handleCommand(AsyncWebServerRequest *request) {
    String cmd = request->getParam("cmd")->value();
    processCommand(cmd);
    request->send(200, "text/plain", "Команда выполнена: " + cmd);
}

void sendHTTPGetRequest(String url) {
    if (!WiFi.isConnected()) return;
    WiFiClient client;
    if (!client.connect(url.c_str(), 80)) {
        displayMessage("Не удалось подключиться к " + url);
        return;
    }
    client.print("GET / HTTP/1.1\r\nHost: " + url + "\r\n\r\n");
    while(client.available()) {
        String line = client.readStringUntil('\n');
        displayMessage(line);
    }
    client.stop();
}

void autoReconnectWiFi() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 5000) {
        if (WiFi.status() != WL_CONNECTED) {
            displayMessage("Попытка восстановить WiFi...");
            WiFi.reconnect();
        }
        lastCheck = millis();
    }
}