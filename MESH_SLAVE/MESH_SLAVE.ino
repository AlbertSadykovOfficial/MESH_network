#include <ESP8266WiFi.h>
#include <ESP8266WiFiMesh.h>
#include <TypeConversionFunctions.h>
#include <assert.h>

// Переменные ретрансляции сообщений
bool retranslate = false;
String translate = "";

// Определяем префиксы устройства в сети (SSID) и пароль
const char exampleMeshName[] PROGMEM = "MeshNode_";
const char exampleWiFiPassword[] PROGMEM = "ChangeThisWiFiPassword_TODO";


unsigned int requestNumber = 0;
unsigned int responseNumber = 0;


// 1) Запрос
// 2) Ответ
// 3) Подпрограмма подклчючения к другим узлам
String manageRequest(const String &request, ESP8266WiFiMesh &meshInstance);
transmission_status_t manageResponse(const String &response, ESP8266WiFiMesh &meshInstance);
void networkFilter(int numberOfNetworks, ESP8266WiFiMesh &meshInstance);


// Создаем объект Узла
// В конструктор класса:
// 1) Задаем ему методы, которые он должен исполнять
// 2) Передаем ему префиксы имени и пароля
ESP8266WiFiMesh meshNode = ESP8266WiFiMesh(manageRequest, manageResponse, networkFilter, FPSTR(exampleWiFiPassword), FPSTR(exampleMeshName), "", true);


// Подпрограмма приема и обработки данных
String manageRequest(const String &request, ESP8266WiFiMesh &meshInstance) {
    String command = "";
  
    // Проверяем пришедшую команду на соответствие шаблонам
    // Если команда не удовлетворяет условиям, значит она
    // адресована не нам и нужно ее перслать другим устройствам
    // 2й варинат (Переслать в главном цикле) - НЕ РАБОТАЕТ
    if (request == "EXECUTE: START")
    {
        command = "STARTED Successefuly";
        digitalWrite(14, HIGH);
        digitalWrite(12, HIGH);
    }
    else if (request == "EXECUTE: STOP")
    {
        command = "STOPPED Successefuly";
        digitalWrite(14, LOW);
        digitalWrite(12, LOW);
    }
    else
    {
        retranslate = true;
        translate = request + " translated by SLAVE";
        command = "SLAVE:unknown command, I have translated to next node";
    }
    
    Serial.print(F("DATA_TO_ME: "));
    Serial.println(request);
    //String command = meshInstance.getMessage();
    Serial.print(F("DATA_FROM_ME: "));
    Serial.println(command);
  
    return command;
}


transmission_status_t manageResponse(const String &response, ESP8266WiFiMesh &meshInstance) 
{
    transmission_status_t statusCode = TS_TRANSMISSION_COMPLETE;
    String command = meshInstance.getMessage();
    Serial.print(F("DATA_FROM_ME: "));
    Serial.println(command);
    Serial.print(F("DATA_TO_ME: "));
    Serial.println(response);
  
    // Our last request got a response, so time to create a new request.
    //meshInstance.setMessage(String(F("Hello world request #")) + String(++requestNumber) + String(F(" from "))
      //                      + meshInstance.getMeshName() + meshInstance.getNodeID() + String(F(".")));
  
    // (void)meshInstance; // This is useful to remove a "unused parameter" compiler warning. Does nothing else.
    return statusCode;
}


// Подключение к устройсвам
void networkFilter(int numberOfNetworks, ESP8266WiFiMesh &meshInstance) {
    for (int networkIndex = 0; networkIndex < numberOfNetworks; ++networkIndex) 
    {  
        // SSID устройства
        // Номер этотого устройства в списке
        String currentSSID = WiFi.SSID(networkIndex);
        int meshNameIndex = currentSSID.indexOf(meshInstance.getMeshName());
      
        if (meshNameIndex >= 0) {
            uint64_t targetNodeID = stringToUint64(currentSSID.substring(meshNameIndex + meshInstance.getMeshName().length()));
            //Serial.println("TargetNodeID = " + (targetNodeID));
            if (targetNodeID < stringToUint64(meshInstance.getNodeID())) {
              ESP8266WiFiMesh::connectionQueue.push_back(NetworkInfo(networkIndex));
            }
        }
    }
    Serial.println();
}


// Первоначальная настрйока (при первом запуске)
void setup() {
    WiFi.persistent(false);
    
    Serial.begin(115000);
    delay(50); // Ждем пока запустится Порт

    // Надо, чтобы удостовериться, что WiFi работает
    // Так же это предотвратит возможно бесконечное зависание
    WiFi.disconnect();
    
    Serial.println();
    Serial.println(F("Setting up mesh node..."));
    
    // Инифиализируем узел
    meshNode.begin();
    meshNode.activateAP(); // Each AP requires a separate server port.
    meshNode.setNodeID("SLAVE1"); // Задаем ему ID, его SSID будет MeshNode_SLAVE1
    meshNode.setStaticIP(IPAddress(192, 168, 4, 12)); // Устанавливаем статический IP для устрйоства

    // Задаем выходы для устройства управления
    pinMode(12, OUTPUT);
    pinMode(14, OUTPUT);
}
int32_t timeOfLastScan = -10000;


// Функция отправки сообщения
void send_message(String command)
{
    String request = command;
    meshNode.attemptTransmission(request, false);
    
    timeOfLastScan = millis();
    
    // Проверка работоспособности
    if (ESP8266WiFiMesh::latestTransmissionSuccessful()) {
        Serial.println(F("Transmission successful."));
    }
    if (ESP8266WiFiMesh::latestTransmissionOutcomes.empty()) {
        Serial.println(F("No mesh AP found."));
    }
    Serial.println();

    translate = "";
}


void loop() {
    // Пробовал пересылку сообщений дальше по сети
    // При условии, что сообщение не для нас,
    // но пока не работает.
    if (retranslate == true)
    {
        //meshNode.connectToNode(String target_ssid, String message);
        retranslate = false;
        // send_message(translate);
        //meshNode.attemptScan(translate);
        Serial.println("Translate" + translate);
    }
    // Ожидаем управляющий сигнлалов от других устройств.
    meshNode.acceptRequest();
}
