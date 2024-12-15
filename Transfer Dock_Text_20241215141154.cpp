#include <WiFi.h>
#include <BluetoothA2DPSink.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <BluetoothSerial.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>
#include <time.h>
#include <HTTPClient.h>
#include <Adafruit_MAX4466.h>

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pin definitions
#define RGB_RED_PIN 25
#define RGB_GREEN_PIN 26
#define RGB_BLUE_PIN 27
#define MIC_PIN 34
#define PLAY_PAUSE_BTN 32
#define VOL_UP_BTN 33
#define VOL_DOWN_BTN 35

// I2S configurations
#define I2S_DOUT 22
#define I2S_BCLK 23
#define I2S_LRC 21

// Audio settings
#define SAMPLE_RATE 44100
#define BITS_PER_SAMPLE 16

// Bluetooth objects
BluetoothA2DPSink a2dp_sink;
BluetoothSerial SerialBT;

// MAX4466 Microphone
Adafruit_MAX4466 mic(MIC_PIN);

// Global variables
bool isListening = false;
bool isSetupMode = false;
int volume = 50;
String geminiApiKey = "AIzaSyAvVygCuBOoLGzduzyUECD4m8R0cu-U1ms";
const char* ntpServer = "pool.ntp.org";

// WiFi credentials
String ssid = "";
String password = "";

void setup() {
    Serial.begin(115200);

    // Initialize pins
    pinMode(RGB_RED_PIN, OUTPUT);
    pinMode(RGB_GREEN_PIN, OUTPUT);
    pinMode(RGB_BLUE_PIN, OUTPUT);
    pinMode(PLAY_PAUSE_BTN, INPUT_PULLUP);
    pinMode(VOL_UP_BTN, INPUT_PULLUP);
    pinMode(VOL_DOWN_BTN, INPUT_PULLUP);

    // Initialize display
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("Display initialization failed");
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    // Initialize I2S
    initI2S();

    // Initialize Bluetooth
    SerialBT.begin("Eemo_Setup");
    a2dp_sink.start("Eemo");

    // Check for setup mode
    checkSetupMode();
}

void loop() {
    if (isSetupMode) {
        handleSetupMode();
    } else {
        handleNormalOperation();
    }

    handleButtons();
    updateDisplay();
    delay(10);
}

void initI2S() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = (i2s_bits_per_sample_t)BITS_PER_SAMPLE,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRC,
        .data_out_num = I2S_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
}

void checkSetupMode() {
    if (digitalRead(PLAY_PAUSE_BTN) == LOW) {
        delay(3000); // Wait 3 seconds
        isSetupMode = true;
        Serial.println("Entering Setup Mode");
    }
}

void handleSetupMode() {
    if (SerialBT.available()) {
        String data = SerialBT.readString();  // Read the received data
        int commaIndex = data.indexOf(',');

        // Check if the received data is valid
        if (commaIndex > 0) {
            ssid = data.substring(0, commaIndex);          // Extract SSID
            password = data.substring(commaIndex + 1);     // Extract password

            Serial.println("Received SSID: " + ssid);
            Serial.println("Received Password: " + password);

            if (connectToWiFi()) {
                isSetupMode = false;
                SerialBT.println("WiFi connected successfully");
                configTime(0, 0, ntpServer);
            } else {
                SerialBT.println("WiFi connection failed");
            }
        } else {
            SerialBT.println("Invalid input format. Use SSID,PASSWORD");
        }
    }
}

bool connectToWiFi() {
    WiFi.begin(ssid.c_str(), password.c_str());
    int attempts = 0;

    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected successfully");
        return true;
    } else {
        Serial.println("WiFi connection failed");
        return false;
    }
}

void handleNormalOperation() {
    if (isListening) {
        handleVoiceCommand();
        setRGBColor(0, 255, 0); // Green when listening
    } else {
        setRGBColor(0, 0, 255); // Blue when idle
    }
}

void handleVoiceCommand() {
    // Placeholder for voice command handling logic
}

void handleButtons() {
    // Placeholder for button handling logic
}

void updateDisplay() {
    display.clearDisplay();
    display.setCursor(0, 0);

    if (isSetupMode) {
        display.println("Setup Mode");
        display.println("Connect to Eemo_Setup via Bluetooth");
    } else {
        display.println("Normal Operation Mode");
    }

    display.display();
}

void setRGBColor(int r, int g, int b) {
    analogWrite(RGB_RED_PIN, r);
    analogWrite(RGB_GREEN_PIN, g);
    analogWrite(RGB_BLUE_PIN, b);
}