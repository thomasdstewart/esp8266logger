/*
 *  ESP8266 Logger
 *  Copyright 2021-2022 Thomas Stewart <thomas@stewarts.org.uk>
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
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>

#include "config.h"
#include "certs.h"

#ifdef USE_AHTXX
#include <AHTxx.h>
#endif

#ifdef USE_MAX6675
#include <MAX6675.h>
#endif

const char *ssid = SSID;
const char *pass = PASS;
const char *pushgateway_path = "/metrics/job/esp8266logger/";
const String url = String("https://") + pushgateway_host + ":" + pushgateway_port + pushgateway_path;
X509List cert(CA);

const int deepSleepTimeS = 60;

#ifdef USE_AHTXX
AHTxx aht10(AHTXX_ADDRESS_X38, AHT1x_SENSOR);
char temperature[10];
char humidity[10];
#endif

#ifdef USE_MAX6675
#define DO  D7 // SO
#define CS  D6 // CS 
#define CLK D5 // SCK 
MAX6675 thermocouple;
char temperature[10];
#endif

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
    } else {
        Serial.println();
        Serial.println("Failed to connect");
        return false;
    }

    configTime(3 * 3600, 0, "0.uk.pool.ntp.org", "1.uk.pool.ntp.org");

    Serial.print("Waiting for NTP time sync: ");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println();

    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    String current_time = asctime(&timeinfo);
    current_time.trim();
    Serial.print("Current time: ");
    Serial.println(current_time);

    return true;
}

//https://github.com/enjoyneering/AHTxx/blob/main/examples/AHT10_Serial/AHT10_Serial.ino
//https://github.com/RobTillaart/MAX6675/blob/master/examples/Demo_getRawData/Demo_getRawData.ino
boolean readTemp() {
#ifdef USE_AHTXX
    if(aht10.begin() != true) {
        Serial.println("AHT1x not connected or fail to load calibration coefficient");
        return false;
      }

    float t = aht10.readTemperature();
    if (t != AHTXX_ERROR) {
        dtostrf(t, 0, 2, temperature);
        Serial.print("Temperature: ");
        Serial.println(temperature);
    } else {
        Serial.print("Read error");
        return false;
    }

    float h = aht10.readHumidity() / 100;
    if (h != AHTXX_ERROR) {
        dtostrf(h, 0, 2, humidity);
        Serial.print("Humidity: ");
        Serial.println(humidity);
    } else {
        Serial.print("Read error");
        return false;
    }
#endif

#ifdef USE_MAX6675
    Serial.println("Waiting for MAX6675 to stabalise");
    delay(500);

    thermocouple.begin(CLK, CS, DO);
    int status = thermocouple.read();
    if (status != STATUS_OK) {
        Serial.print("Read error");
        return false;
    }

    float t = thermocouple.getTemperature();
    dtostrf(t, 0, 2, temperature);
    Serial.print("Temperature: ");
    Serial.println(temperature);
#endif
    return true;
}

//https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi/examples
void makeRequest() {

#ifdef USE_AHTXX
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
#endif

#ifdef USE_MAX6675
    // # TYPE esp8266logger_temperature_celsius gauge
    // # HELP Temperature in Celsius
    // esp8266logger_temperature_celsius{name="01"} 25.5

    char line1[100], line2[100], line3[100], buf[600];
    sprintf(line1, "# TYPE esp8266logger_temperature_celsius gauge\n");
    sprintf(line2, "# HELP Temperature in Celsius\n");
    sprintf(line3, "esp8266logger_temperature_celsius{name=\"%s\"} %s\n", LOGGER_NAME, temperature);

    strcpy(buf, line1);
    strcat(buf, line2);
    strcat(buf, line3);
#endif

    Serial.print("URL: ");
    Serial.println(url);

    Serial.println("Sending data: ");
    Serial.printf("%s\r%s\r%s\r", line1, line2, line3);
#ifdef USE_AHTXX
    Serial.printf("%s\r%s\r%s\r", line4, line5, line6);
#endif
    Serial.println();

    WiFiClientSecure client;
    HTTPClient http;

    client.setTrustAnchors(&cert);
    //client->setInsecure();
    
    http.begin(client, url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.POST(buf);

    delay(50);

    if (httpCode > 0) {
        Serial.print("HTTP response code: ");
        Serial.println(httpCode);

        String payload = http.getString();
        Serial.print("HTTP response payload: ");
        Serial.println(payload);
    } else {
        Serial.print("Request failed, error: ");
        Serial.println(http.errorToString(httpCode));

        char sslErrorMsg[80];
	int sslError = client.getLastSSLError(sslErrorMsg, sizeof(sslErrorMsg));
	if (sslError) {
            Serial.printf("SSL error: %d: %s\r\n", sslError, sslErrorMsg);
	}
    }

    http.end();
}

// vim: sts=4 sw=4
