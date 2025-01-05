#include "LoRaWan_APP.h"
#include <Wire.h>
#include "Adafruit_SCD30.h"

// SCD30 instance
Adafruit_SCD30 scd30;

/* OTAA parameters (your end device credentials) */
uint8_t devEui[] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x06, 0xCF, 0x8C };
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[] = { 0x39, 0x05, 0x33, 0x6E, 0x29, 0x10, 0x9B, 0x19, 0x36, 0xCC, 0xAB, 0x5D, 0x81, 0xE4, 0xCF, 0x21 };

/* ABP parameters (not used in OTAA mode, but kept for reference) */
uint8_t nwkSKey[] = { 0x15, 0xB1, 0xD0, 0xEF, 0xA4, 0x63, 0xDF, 0xBE, 0x3D, 0x11, 0x18, 0x1E, 0x1E, 0xC7, 0xDA, 0x85 };
uint8_t appSKey[] = { 0xD7, 0x2C, 0x78, 0x75, 0x8C, 0xDC, 0xCA, 0xBF, 0x55, 0xEE, 0x4A, 0x77, 0x8D, 0x16, 0xEF, 0x67 };
uint32_t devAddr = (uint32_t)0x007E6AE1;

/* LoRaWAN channel mask */
uint16_t userChannelsMask[6] = { 0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };

/* LoRaWAN region, select in Arduino IDE tools */
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/* LoRaWAN class, Class A and Class C are supported */
DeviceClass_t loraWanClass = CLASS_A;

/* Application data transmission duty cycle (value in ms) */
uint32_t appTxDutyCycle = 15000;

/* OTAA activation */
bool overTheAirActivation = true;

/* Adaptive data rate (ADR) enable */
bool loraWanAdr = true;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = true;

/* Application port */
uint8_t appPort = 2;

/* Number of trials to transmit the frame */
uint8_t confirmedNbTrials = 4;

/* Prepares the payload of the frame */
static void prepareTxFrame(uint8_t port)
{
    if (scd30.dataReady())
    {
        if (!scd30.read())
        {
            Serial.println("Failed to read data from SCD30!");
            return;
        }

        // Convert float values to integers for transmission
        uint16_t co2_int = (uint16_t)scd30.CO2;             // CO2 in ppm
        uint16_t temp_int = (uint16_t)(scd30.temperature * 10); // Temp in 0.1°C
        uint16_t hum_int = (uint16_t)(scd30.relative_humidity * 10); // Humidity in 0.1%

        // Prepare LoRaWAN payload
        appDataSize = 6;
        appData[0] = (co2_int >> 8) & 0xFF;
        appData[1] = co2_int & 0xFF;
        appData[2] = (temp_int >> 8) & 0xFF;
        appData[3] = temp_int & 0xFF;
        appData[4] = (hum_int >> 8) & 0xFF;
        appData[5] = hum_int & 0xFF;

        Serial.print("CO2: ");
        Serial.print(scd30.CO2);
        Serial.print(" ppm, Temp: ");
        Serial.print(scd30.temperature);
        Serial.print(" °C, Humidity: ");
        Serial.print(scd30.relative_humidity);
        Serial.println(" %");
    }
    else
    {
        Serial.println("SCD30 data not ready.");
    }
}

void setup() {
    Serial.begin(115200);

    // Initialize I2C for SCD30
    Wire.begin(41, 42); // SDA = 41, SCL = 42

    // Initialize the SCD30 sensor
    if (!scd30.begin())
    {
        Serial.println("Failed to initialize SCD30! Check wiring.");
        while (1);
    }

    Serial.println("SCD30 initialized successfully.");

    // LoRaWAN initialization
    Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
}

void loop()
{
    switch (deviceState)
    {
        case DEVICE_STATE_INIT:
        {
#if (LORAWAN_DEVEUI_AUTO)
            LoRaWAN.generateDeveuiByChipID();
#endif
            LoRaWAN.init(loraWanClass, loraWanRegion);
            LoRaWAN.setDefaultDR(3);
            break;
        }
        case DEVICE_STATE_JOIN:
        {
            LoRaWAN.join();
            break;
        }
        case DEVICE_STATE_SEND:
        {
            prepareTxFrame(appPort);
            LoRaWAN.send();
            deviceState = DEVICE_STATE_CYCLE;
            break;
        }
        case DEVICE_STATE_CYCLE:
        {
            txDutyCycleTime = appTxDutyCycle + randr(-APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND);
            LoRaWAN.cycle(txDutyCycleTime);
            deviceState = DEVICE_STATE_SLEEP;
            break;
        }
        case DEVICE_STATE_SLEEP:
        {
            LoRaWAN.sleep(loraWanClass);
            break;
        }
        default:
        {
            deviceState = DEVICE_STATE_INIT;
            break;
        }
    }
}
