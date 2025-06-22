#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#define TRIG_PIN 5
#define ECHO_PIN 18
#define LIGHT_PIN 34
#define BUZZER_PIN 22
#define BUTTON_PIN 23
#define LED_R 2
#define LED_G 4

char payload[128];
const char *ssid = "MyWIFI";
const char *password = "Aa123123";

const char *mqtt_server = "192.168.97.65";
const int mqtt_port = 1883;

const char *mqtt_user = "MQTT1";
const char *mqtt_password = "123456";
const char *mqtt_topic_pub = "esp32/sensor";
const char *mqtt_topic_sub = "esp32/control";

WiFiClient espClient;
PubSubClient client(espClient);

// 警报状态管理
bool isAlarming = false;
unsigned long alarmStartTime = 0;
const unsigned long ALARM_DURATION = 3000; // 3秒警报时间

// MQTT消息回调函数
void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("收到消息，主题: ");
    Serial.println(topic);

    String message;
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }
    Serial.print("内容: ");
    Serial.println(message);
}

void setup_wifi()
{
    delay(100);
    Serial.println();
    Serial.print("连接WiFi中: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi已连接");
    Serial.println("IP地址: ");
    Serial.println(WiFi.localIP());
}

void reconnect()
{
    while (!client.connected())
    {
        Serial.print("连接到MQTT中...");
        if (client.connect("ESP32Client", mqtt_user, mqtt_password))
        {
            Serial.println("连接成功");
            client.subscribe(mqtt_topic_sub);
            Serial.print("已订阅主题: ");
            Serial.println(mqtt_topic_sub);
        }
        else
        {
            Serial.print("连接失败，状态码: ");
            Serial.print(client.state());
            Serial.println("，5秒后重试");
            delay(5000);
        }
    }
}

float readUltrasonicDistance()
{
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    if (duration == 0)
        return -1.0;

    return duration * 0.034 / 2.0;
}

void uploadRequest()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        http.begin("http://192.168.97.18/take-photo");
        http.setTimeout(5000);

        int httpResponseCode = http.GET();
        if (httpResponseCode > 0)
        {
            Serial.printf("📸 请求拍照成功，状态码: %d\n", httpResponseCode);
        }
        else
        {
            Serial.printf("❌ 请求失败: %s\n", http.errorToString(httpResponseCode).c_str());
        }
        http.end();
    }
    else
    {
        Serial.println("WiFi未连接，无法发送拍照请求");
    }
}

volatile bool buttonPressed = false;
volatile unsigned long lastInterruptTime = 0;

void IRAM_ATTR handleButtonPress() {
    unsigned long currentTime = millis();
    if (currentTime - lastInterruptTime > 200) {
        buttonPressed = true;
        lastInterruptTime = currentTime;
    }
}

// 新增：LED控制函数
void setLEDStatus(bool redOn, bool greenOn) {
    digitalWrite(LED_R, redOn ? HIGH : LOW);
    digitalWrite(LED_G, greenOn ? HIGH : LOW);
    
    // 添加调试输出
    Serial.print("LED状态 - 红灯: ");
    Serial.print(redOn ? "开" : "关");
    Serial.print(", 绿灯: ");
    Serial.println(greenOn ? "开" : "关");
}

// 新增：处理警报逻辑
void handleAlarm(bool shouldAlarm) {
    unsigned long currentTime = millis();
    
    if (shouldAlarm && !isAlarming) {
        // 开始警报
        isAlarming = true;
        alarmStartTime = currentTime;
        digitalWrite(BUZZER_PIN, LOW);  // 蜂鸣器响
        setLEDStatus(true, false);      // 红灯开，绿灯关
        Serial.println("⚠️ 警报开始！");
    }
    else if (isAlarming) {
        // 检查是否应该结束警报
        if (currentTime - alarmStartTime >= ALARM_DURATION) {
            // 结束警报
            isAlarming = false;
            digitalWrite(BUZZER_PIN, HIGH); // 关闭蜂鸣器
            Serial.println("✅ 警报结束");
        }
        // 警报期间保持红灯亮
        setLEDStatus(true, false);
    }
    else if (!shouldAlarm && !isAlarming) {
        // 正常状态：绿灯亮，红灯关
        setLEDStatus(false, true);
    }
}

void setup()
{
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, FALLING);
    
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    
    // 初始化状态
    digitalWrite(BUZZER_PIN, HIGH); // 关闭蜂鸣器
    setLEDStatus(false, true);      // 初始状态：绿灯亮

    setup_wifi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
}

void loop()
{
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();

    float distance = readUltrasonicDistance();
    int lightValue = analogRead(LIGHT_PIN);

    Serial.print("距离: ");
    Serial.print(distance);
    Serial.print(" cm | 光照: ");
    Serial.println(lightValue);

    // 判断是否需要警报
    bool shouldAlarm = false;
    
    // 光照条件检查
    if (lightValue > 1500) {
        shouldAlarm = true;
        Serial.println("🔆 光照过强触发警报");
    }
    
    // 距离条件检查（只有在距离有效时才判断）
    if (distance > 0 && distance < 5) {
        shouldAlarm = true;
        Serial.println("📏 距离过近触发警报");
    }
    
    // 处理警报和LED状态
    handleAlarm(shouldAlarm);

    // 处理按钮按下事件
    if (buttonPressed) {
        buttonPressed = false;
        Serial.println("🔘 按钮中断触发，发送上传请求...");
        uploadRequest();
    }

    // 发布MQTT消息
    StaticJsonDocument<128> doc;
    doc["distance"] = distance;
    doc["light"] = lightValue;
    doc["alarm_active"] = isAlarming; // 添加警报状态信息
    serializeJson(doc, payload);

    if (client.publish(mqtt_topic_pub, payload))
    {
        Serial.println("📤 MQTT 发布成功");
    }
    else
    {
        Serial.println("❌ MQTT 发布失败");
    }

    delay(1000); // 减少延时，提高响应速度
}