#include <../Config.h>
#include <HardwareSerial.h>
#include <Wifi.h>
#include <PubSubClient.h>


const char *SSID = WIFI_SSID;
const char *PSK = WIFI_PW;
const char *MQTT_BROKER = MQTT_SERVER;
const char *MQTT_BROKER_IP = MQTT_SERVER_IP;
const char *MQTT_BROKER_PORT = MQTT_SERVER_PORT;


String dataReceived;
bool isDataReceived = false;
int byteSend;
char itoaBuf[64];

unsigned int desiredMaxCurrent = 0;
unsigned int desiredMaxVoltage = 0;

long rssi = 0;
int output = -1;
int voltmeter = -1;
int voltpreset = -1;
int ameter = -1;
int currentpreset = -1;
int constmode = -1;
int temperature = -1;
int voltmax = -1;
int currentmax = -1;


WiFiClient espClient;
PubSubClient client(espClient);

// extract and return the "operand" from a message returned by DPM
//
int processString(String str) {
    //Serial.println("Processing String "+ str);
    str.remove(0, 7);
    str.replace(".", "");
    return atoi(str.c_str());
}

// send command consisting of format+command to DPM
// check if DPM returns returncode
// TODO: hangs if DPM returns something other than desired returncode
//
int sendCommand(String format, String returncode, String command)
{
    dataReceived = "";
  
    //Serial.println("Sending Command: " + format + command);
    Serial2.println(format+command);
    delay(30);

     if (Serial2.available())
     {
        //Serial.println("Available");
        while(dataReceived.indexOf(returncode) == -1)
        {
            dataReceived = Serial2.readStringUntil('\n');
        //    Serial.println(dataReceived);
        }
        //if (dataReceived.indexOf(format) != -1)
        isDataReceived = true;
    }
 
    if (isDataReceived){
        //delay(111);
      
        isDataReceived = false;
        return processString(dataReceived);
        
    }

    return -1; //error Code

}


// PubSub Callback
//
void mqttCallback(char *_topic, byte *payload, unsigned int length) {
    String topic = String(_topic);
    char data_str[length + 1];
    memcpy(data_str, payload, length);
    data_str[length] = '\0';

    //Serial.println("Received topic '" + topic + "' with data '" + (String)data_str + "'");

    if (topic == "pv1/dcdc/command/power") {
        if((String)data_str == "1")
            sendCommand(":01w12=","01ok","1,"); //output on
        else if((String)data_str == "0")
            sendCommand(":01w12=","01ok","0,"); //output off
        else
            return; // do nothing

    } else if (topic == "pv1/dcdc/command/max-current") {
        desiredMaxCurrent = atoi(data_str);
        
        // check against device limits
        if(desiredMaxCurrent > currentmax)
            return;
        if(desiredMaxCurrent <= 0)
            return;
        sendCommand(":01w11=","01ok",String(desiredMaxCurrent)+",");
        return;
        
    } else if (topic == "pv1/dcdc/command/max-voltage") {
        desiredMaxVoltage = atoi(data_str);
        
        // check against device limits
        if(desiredMaxVoltage > voltmax)
            return;
        if(desiredMaxVoltage <= 0)
            return;
        sendCommand(":01w10=","01ok",String(desiredMaxVoltage)+",");
        return;
        
    }
}

void setupMqtt() {
    client.setServer(MQTT_BROKER, (uint16_t)strtol(MQTT_BROKER_PORT, NULL, 10));
    client.setCallback(mqttCallback);
}


void setup()  {
    Serial.begin(115200);
    Serial2.begin(9600);
  
    Serial.println("Setting up WIFI" + String(SSID));
    WiFi.setAutoReconnect(true);
    WiFi.hostname("DPM8624-001");
    WiFi.begin(SSID, PSK);

    setupMqtt();

}

void loop() {

    bool wifiConnected = true;
    bool wasWifiConnected = WiFi.status() == WL_CONNECTED;
    int retryWifi = 0;
    while (WiFi.status() != WL_CONNECTED) {
        Serial.println("Connecting to WiFi...");
        if (retryWifi > 10) {
            wifiConnected = false;
            break;
        } else {
            retryWifi++;
        }
        delay(500);
    }

    bool mqttConnected = wifiConnected;
    bool wasMqttConnected = client.connected();
    int retryMqtt = 0;
    while (wifiConnected && !client.connected()) {
        Serial.println("Connecting to MQTT broker...");
        client.connect("PV1-DCDC-ESP8266");
        if (retryMqtt > 3) {
            mqttConnected = false;
            break;
        } else {
            if (retryMqtt == 2) {
                Serial.println("MQTT DNS not resolved, trying IP...");
                setupMqtt();
            }
            retryMqtt++;
        }
        delay(500);
    }
    
    if (!mqttConnected) {
        Serial.println("Failed to connect to MQTT broker");
    } else if (!wasMqttConnected && mqttConnected) {
        Serial.println("Reconnected to MQTT broker");
        client.subscribe("pv1/dcdc/command/power");
        client.subscribe("pv1/dcdc/command/max-current");
        client.subscribe("pv1/dcdc/command/max-voltage");
    }

    client.loop();
    
    delay(111);
    //Serial.println(" ");

    // reading DC/DC Status 
    output = sendCommand(":01r12=",":01r12=","0,");
    voltmeter = sendCommand(":01r30=",":01r30=","0,");
    voltpreset = sendCommand(":01r10=",":01r10=","0,");
    ameter = sendCommand(":01r31=",":01r31=","0,");
    currentpreset = sendCommand(":01r11=",":01r11=","0,");
    constmode = sendCommand(":01r32=",":01r32=","0,");
    temperature = sendCommand(":01r33=",":01r33=","0,");
    voltmax = sendCommand(":01r00=",":01r00=","0,");
    currentmax = sendCommand(":01r01=",":01r01=","0,");

    rssi = WiFi.RSSI();
    
    Serial.println("RSSI: " + String(rssi));

    client.publish("pv1/dcdc/rssi", itoa(rssi, itoaBuf, 10));
    client.publish("pv1/dcdc/output", itoa(output, itoaBuf, 10));
    client.publish("pv1/dcdc/voltmeter", itoa(voltmeter, itoaBuf, 10));
    client.publish("pv1/dcdc/voltpreset", itoa(voltpreset, itoaBuf, 10));
    client.publish("pv1/dcdc/voltmax", itoa(voltmax, itoaBuf, 10));
    client.publish("pv1/dcdc/ameter", itoa(ameter, itoaBuf, 10));
    client.publish("pv1/dcdc/currentpreset", itoa(currentpreset, itoaBuf, 10));
    client.publish("pv1/dcdc/currentmax", itoa(currentmax, itoaBuf, 10));
    client.publish("pv1/dcdc/constmode", itoa(constmode, itoaBuf, 10));
    client.publish("pv1/dcdc/temperature", itoa(temperature, itoaBuf, 10));
    

    delay(900);
}
