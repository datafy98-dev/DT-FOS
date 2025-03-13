#include <M5StickCPlus2.h>
#include <BluetoothSerial.h>
#include <WiFi.h>
#include <WiFiUdp.h>

String ssid = "Poco X3 Pro";
String password = "bobr kurva";
String udpAddress = "192.168.207.166";
int udpPort = 12345;

BluetoothSerial SerialBT;

// Глобальные переменные
bool isBluetoothEnabled = true;
uint16_t textColor = WHITE;
uint16_t backgroundColor = BLACK;
int currentTextSize = 1;
WiFiUDP Udp;
char udpBuffer[256];

// Функции для обработки команд (прототипы)
void processCommand(String command);
void displayMessage(String message);
void clearScreen();
void scrollText();
void scanWiFiNetworks();
void sendUDPMessage(String address, int port, String message);
void sendTCPMessage(String host, int port, String message);
void sendHTTPPostRequest(String url, String data);
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
    WiFi.begin(ssid.c_str(), password.c_str());
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        M5.Lcd.print(".");
    }
    M5.Lcd.println("\nWiFi Connected!");

    SerialBT.begin("DT-FOS");
    displayMessage("Bluetooth: DT-FOS");
    displayMessage("OS Initialized");
    displayMessage("OS Version: V0.2");

    Udp.begin(udpPort);
}

void loop() {
    if (isBluetoothEnabled && SerialBT.available()) {
        String command = SerialBT.readStringUntil('\n');
        command.trim();
        displayMessage("> " + command);
        processCommand(command);
    }

    M5.update();

    // Обработка входящих UDP-пакетов
    if (Udp.parsePacket()) {
        int len = Udp.read(udpBuffer, 255);
        if (len > 0) {
            udpBuffer[len] = 0;
            String message = "UDP received: ";
            message += udpBuffer;
            displayMessage(message);
        }
    }

    autoReconnectWiFi();
}

// Определение функции displayMessage
void displayMessage(String message) {
    int y = M5.Lcd.getCursorY();
    int screenHeight = M5.Lcd.height();

    if (y + currentTextSize * 8 > screenHeight) {
        scrollText();
    }

    M5.Lcd.println(message); 
    SerialBT.println(message); 
}

// Остальные функции
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
    String infoMessage = "DT-FOS V0.2\n";
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
    SerialBT.begin("DT-FOS");
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
    sendUDPMessage(udpAddress,udpPort,message);
    } else if (command == "wifi_status") {
    String status = (WiFi.status() == WL_CONNECTED) ? "Connected" : "Disconnected";
    displayMessage("WiFi Status: " + status);
    } 
    else if (command.startsWith("wifi ")) {
      String params = command.substring(5);
      int spaceIndex = params.indexOf(' ');
      if (spaceIndex > 0) {
          String newSSID = params.substring(0, spaceIndex);
          String newPassword = params.substring(spaceIndex+1);
          ssid = newSSID;
          password = newPassword;
          WiFi.disconnect();
          WiFi.begin(ssid.c_str(), password.c_str());
          displayMessage("Connecting to " + ssid + "...");
      } else {
          displayMessage("Ошибка: Нужен SSID и пароль");
      }
  } 
  else if (command == "ip") {
      if (WiFi.isConnected()) {
          displayMessage("IP: " + WiFi.localIP().toString());
      } else {
          displayMessage("WiFi не подключен");
      }
  } 
  else if (command.startsWith("udp_set ")) {
      String params = command.substring(8);
      int space = params.indexOf(' ');
      if (space > 0) {
          String newAddr = params.substring(0, space);
          int newPort = params.substring(space+1).toInt();
          udpAddress = newAddr;
          udpPort = newPort;
          displayMessage("UDP настроен на " + newAddr + ":" + String(newPort));
      } else {
          displayMessage("Ошибка: Формат: udp_set <addr> <port>");
      }
  } 
  else if (command.startsWith("udp ")) {
      String message = command.substring(4);
      sendUDPMessage(udpAddress, udpPort, message);
  } 
  else if (command.startsWith("tcp ")) {
      String params = command.substring(4);
      int space1 = params.indexOf(' ');
      int space2 = params.indexOf(' ', space1+1);
      if (space1 > 0 && space2 > 0) {
          String host = params.substring(0, space1);
          int port = params.substring(space1+1, space2).toInt();
          String msg = params.substring(space2+1);
          sendTCPMessage(host, port, msg);
      } else {
          displayMessage("Ошибка: Формат: tcp <host> <port> <message>");
      }
  } 
  else if (command.startsWith("post ")) {
      String params = command.substring(5);
      int space = params.indexOf(' ');
      if (space > 0) {
          String url = params.substring(0, space);
          String data = params.substring(space+1);
          sendHTTPPostRequest(url, data);
      } else {
          displayMessage("Ошибка: Формат: post <url> <data>");
      }
  }  else {
    SerialBT.println("Unknown command: " + command);
}
}

void clearScreen() {
    M5.Lcd.fillScreen(backgroundColor);
    M5.Lcd.setCursor(0, 0);
}

void scrollText() {
    M5.Lcd.scroll(0, -8); 
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

void sendUDPMessage(String address, int port, String message) {
    Udp.beginPacket(address.c_str(), port);
    Udp.write((uint8_t*)message.c_str(), message.length()); 
    Udp.endPacket();
    displayMessage("UDP: " + message + " → " + address + ":" + String(port));
}

void sendTCPMessage(String host, int port, String message) {
    WiFiClient client;
    if (!client.connect(host.c_str(), port)) {
        displayMessage("Не удалось подключиться к " + host);
        return;
    }
    client.print(message);
    while(client.available()) {
        String line = client.readStringUntil('\n');
        displayMessage(line);
    }
    client.stop();
}

void sendHTTPPostRequest(String url, String data) {
    if (!WiFi.isConnected()) return;
    WiFiClient client;
    if (!client.connect(url.c_str(), 80)) {
        displayMessage("Не удалось подключиться к " + url);
        return;
    }
    client.print("POST / HTTP/1.1\r\nHost: " + url + "\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: " + data.length() + "\r\n\r\n" + data);
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