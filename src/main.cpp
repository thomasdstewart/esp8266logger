/*
 *  ESP8266 Logger
 *  Copyright 2021 Thomas Stewart <thomas@stewarts.org.uk>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <AHT10.h>
#include "config-sample.h"
#include "config.h"

const char *ssid = SSID;
const char *pass = PASS;
const char *url = URL;
const uint8_t url_fp[20] = URL_FP;
const int deepSleepTimeS = 60;
AHT10 myAHT10(AHT10_ADDRESS_0X38);
char temperature[10];
char humidity[10];

boolean initWifi();
boolean readTemp();
void makeRequest();

void setup() {
    Serial.begin(115200); 
    Serial.println();
    Serial.println("Woke up");

    if(initWifi() && readTemp()) {
        makeRequest();
    }
   
    Serial.println("Deep sleeping");
    ESP.deepSleep(deepSleepTimeS * 1000000);
}

void loop() {
}

boolean initWifi() {
    Serial.print("Connecting to: ");
    Serial.print(ssid); 
    Serial.print(" "); 
    WiFi.begin(ssid, pass);  

    int wifitimeout = 10 * 4;
    while(WiFi.status() != WL_CONNECTED && (wifitimeout-- > 0)) {
        delay(250);
        Serial.print(".");
    }

    if(WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("Connected with IP: "); 
        Serial.println(WiFi.localIP());
        return true;
    } else {
        Serial.println();
        Serial.println("Failed to connect");
        return false;
    }
}

//https://github.com/enjoyneering/AHT10/blob/master/examples/AHT10_Any_MCU_Serial/AHT10_Any_MCU_Serial.ino
boolean readTemp() {
    float t, h;

    if(myAHT10.begin() != true) {
        Serial.println("AHT10 not connected or fail to load calibration coefficient");
        return false;
      }

    t = myAHT10.readTemperature();
    h = myAHT10.readHumidity() / 100;

    dtostrf(t, 0, 2, temperature);
    Serial.print("Temperature: ");
    Serial.println(temperature);

    dtostrf(h, 0, 2, humidity);
    Serial.print("Humidity: ");
    Serial.println(humidity);

    return true;
}

//https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266HTTPClient/examples
void makeRequest() {
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

    client->setFingerprint(url_fp);
    //client->setInsecure();

    HTTPClient https;

    Serial.print("POST to URL: ");
    Serial.println(url);
    if (https.begin(*client, url)) {
        https.addHeader("Content-Type", "application/x-www-form-urlencoded");

        // # TYPE esp8266logger_temperature_celsius gauge
        // # HELP Temperature in Celsius
        // esp8266logger_temperature_celsius{name="01"} 25.5
        // # TYPE esp8266logger_humidity_ratio gauge
        // # HELP Humidity in percent
        // esp8266logger_humidity_ratio{name="01"} 0.687

        char line1[100], line2[100], line3[100], line4[100], line5[100], line6[100], buf[600];
        sprintf(line1, "# TYPE esp8266logger_temperature_celsius gauge\n");
        sprintf(line2, "# HELP Temperature in Celsius\n");
        sprintf(line3, "esp8266logger_temperature_celsius{name=\"%s\"} %s\n", LOGGER_NAME, temperature);
        sprintf(line4, "# TYPE esp8266logger_humidity_ratio gauge\n");
        sprintf(line5, "# HELP Humidity in percent\n");
        sprintf(line6, "esp8266logger_humidity_ratio{name=\"%s\"} %s\n", LOGGER_NAME, humidity);

        strcpy(buf, line1);
        strcat(buf, line2);
        strcat(buf, line3);
        strcat(buf, line4);
        strcat(buf, line5);
        strcat(buf, line6);

        Serial.print("Sending data: ");
        Serial.println(buf);
        int httpCode = https.POST(buf);

        if (httpCode > 0) {
            Serial.print("HTTP response code: ");
            Serial.println(httpCode);

            const String& payload = https.getString();
            Serial.print("received payload: ");
            Serial.println(payload);
        } else {
            Serial.print("Request failed, error: ");
            Serial.println(httpCode);
        }

        https.end();
    } else {
        Serial.println("Unable to connect");
    }
}

// vim: sts=4 sw=4
