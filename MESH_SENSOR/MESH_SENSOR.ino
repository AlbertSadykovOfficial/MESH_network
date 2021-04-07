#include <ESP8266WiFi.h>
#include <ESP8266WiFiMesh.h>
#include <TypeConversionFunctions.h>
#include <assert.h>

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

    // Попытка ретрансляции, если пришло сообщение
    // 1й варинат (прямо при полчении) - НЕ РАБОТАЕТ
    Serial.println(F("|Request|"));
    Serial.print(F("DATA_TO_ME: "));
    Serial.println(request);
    
    send_message(request + " (translated by SLAVE)");
    String command = "Im not waiting commands, message has translated to next node";
    
    // String command = meshInstance.getMessage();
    Serial.print(F("DATA_FROM_ME: "));
    Serial.println(command);
  
    return (command);
}


transmission_status_t manageResponse(const String &response, ESP8266WiFiMesh &meshInstance) 
{
    transmission_status_t statusCode = TS_TRANSMISSION_COMPLETE;
    
    Serial.println(F("|manageResponse|"));
    String command = meshInstance.getMessage();
    Serial.print(F("DATA_FROM_ME: "));
    Serial.println(command);
    Serial.print(F("DATA_TO_ME: "));
    Serial.println(response);
    
    // Our last request got a response, so time to create a new request.
    //meshInstance.setMessage(String(F("Hello world request #")) + String(++requestNumber) + String(F(" from "))
     //                       + meshInstance.getMeshName() + meshInstance.getNodeID() + String(F(".")));
    
    // (void)meshInstance; // This is useful to remove a "unused parameter" compiler warning. Does nothing else.
    return statusCode;
}


// Подключение к устройсвам
void networkFilter(int numberOfNetworks, ESP8266WiFiMesh &meshInstance) {
    //Serial.println();
    //Serial.println("Networks:");
    for (int networkIndex = 0; networkIndex < numberOfNetworks; ++networkIndex) 
    {  
        // SSID устройства
        // Номер этотого устройства в списке
        String currentSSID = WiFi.SSID(networkIndex);
        int meshNameIndex = currentSSID.indexOf(meshInstance.getMeshName());
        
        if (meshNameIndex >= 0) {
          uint64_t targetNodeID = stringToUint64(currentSSID.substring(meshNameIndex + meshInstance.getMeshName().length()));
          //Serial.println("TargetNodeID = " + (targetNodeID));
          Serial.println(meshInstance.getNodeID());
          //if (targetNodeID < stringToUint64(meshInstance.getNodeID())) {
            ESP8266WiFiMesh::connectionQueue.push_back(NetworkInfo(networkIndex));
          //}
        }
    }
    Serial.println();
}

// Первоначальная настрйока (при первом запуске)
void setup() {
    WiFi.persistent(false);
    
    Serial.begin(115200);
    delay(50); // Ждем пока запустится Порт
    
    // Надо, чтобы удостовериться, что WiFi работает
    // Так же это предотвратит возможно бесконечное зависание
    WiFi.disconnect();
    
    Serial.println();
    Serial.println(F("Setting up mesh node..."));
    
    // Инифиализируем узел
    meshNode.begin();
    meshNode.activateAP(); // Each AP requires a separate server port.
    meshNode.setNodeID("SENSOR1"); // Задаем ему ID, его SSID будет MeshNode_SENSOR1
    meshNode.setStaticIP(IPAddress(192, 168, 4, 13)); // Устанавливаем статический IP для устрйоства
}

int32_t timeOfLastScan = -10000;


// Функция отправки сообщения
void send_message(String message)
{
      String request = String(F("Temperature = ")) + message + 
                       String(requestNumber) + 
                       String(F(" from ")) + 
                       meshNode.getNodeID()+ 
                       String(F(".")); 
      meshNode.attemptTransmission(request, false);
  
      // Проверка работоспособности
      if (ESP8266WiFiMesh::latestTransmissionSuccessful()) {
          Serial.println(F("Transmission successful."));
      }
      if (ESP8266WiFiMesh::latestTransmissionOutcomes.empty()) {
          Serial.println(F("No mesh AP found."));
      }
      Serial.println();
 }


void loop() {

    // Через каждые 2 секунды отслыаем данные (пока ранломайзер)
    if (millis() - timeOfLastScan > 3000 || (WiFi.status() != WL_CONNECTED && millis() - timeOfLastScan > 2000)) 
    {   
        timeOfLastScan = millis();
        send_message(String(random(0,100)));
    }  else {
        meshNode.acceptRequest();
    }
}
