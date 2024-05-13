#include "bme_utils.h"
#include "configuration.h"
#include "gps_utils.h"
#include "display.h"
#include <logger.h>

#define SEALEVELPRESSURE_HPA (1013.25)
#define CORRECTION_FACTOR (8.2296)      // for meters

extern Configuration    Config;
extern logging::Logger  logger;

float newHum, newTemp, newPress, newGas;

uint32_t    bmeLastReading      = -60000;
int         wxModuleType        = 0;
uint8_t     wxModuleAddress     = 0x00;


Adafruit_BME280     bme280;
Adafruit_BME680     bme680;
#ifdef HELTEC_V3_GPS
Adafruit_BMP280     bmp280(&Wire1);
#else
Adafruit_BMP280     bmp280;
#endif

   

namespace BME_Utils {    

    void getWxModuleAddres() {
        uint8_t err, addr;
        for(addr = 1; addr < 0x7F; addr++) {
            #ifdef HELTEC_V3_GPS
            Wire1.beginTransmission(addr);
            err = Wire1.endTransmission();
            #else
            Wire.beginTransmission(addr);
            err = Wire.endTransmission();
            #endif
            if (err == 0) {
                if (addr == 0x76 || addr == 0x77) {
                    wxModuleAddress = addr;
                    //
                    Serial.println("Sensor encontrado : " + String(wxModuleAddress,DEC));
                    //
                    return;
                }
            }
        }
    }

    void setup() {
        if (Config.bme.active) {
            getWxModuleAddres();
            if (wxModuleAddress != 0x00) {
                bool wxModuleFound = false;
                #ifdef HELTEC_V3_GPS
                    if (bme280.begin(wxModuleAddress, &Wire1)) {
                        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "BME", " BME280 sensor found");
                        wxModuleType = 1;
                        wxModuleFound = true;
                    } 
                    if (!wxModuleFound) {
                        if (bme680.begin(wxModuleAddress, &Wire1)) {
                            logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "BME", " BME680 sensor found");
                            wxModuleType = 3;
                            wxModuleFound = true;
                        }
                    }
                #else
                    if (bme280.begin(wxModuleAddress)) {
                        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "BME", " BME280 sensor found");
                        wxModuleType = 1;
                        wxModuleFound = true;
                    }
                    if (!wxModuleFound) {
                        if (bme680.begin(wxModuleAddress)) {
                            logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "BME", " BME680 sensor found");
                            wxModuleType = 3;
                            wxModuleFound = true;
                        }
                    }
                #endif
                if (!wxModuleFound) {
                    if (bmp280.begin(wxModuleAddress)) {
                        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "BME", " BMP280 sensor found");
                        wxModuleType = 2;
                        wxModuleFound = true;
                    }
                }
                if (!wxModuleFound) {
                    show_display("ERROR", "", "BME/BMP sensor active", "but no sensor found...", "", 2000);
                    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "BME", " BME/BMP sensor Active in config but not found! Check Wiring");
                } else {
                    switch (wxModuleType) {
                        case 1:
                            bme280.setSampling(Adafruit_BME280::MODE_FORCED,
                                        Adafruit_BME280::SAMPLING_X1,
                                        Adafruit_BME280::SAMPLING_X1,
                                        Adafruit_BME280::SAMPLING_X1,
                                        Adafruit_BME280::FILTER_OFF
                                        );
                            logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "BME", " BME280 Module init done!");
                            break;
                        case 2:
                            bmp280.setSampling(Adafruit_BMP280::MODE_FORCED,
                                        Adafruit_BMP280::SAMPLING_X1,
                                        Adafruit_BMP280::SAMPLING_X1,
                                        Adafruit_BMP280::FILTER_OFF
                                        ); 
                            logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "BMP", " BMP280 Module init done!");
                            break;
                        case 3:
                            bme680.setTemperatureOversampling(BME680_OS_1X);
                            bme680.setHumidityOversampling(BME680_OS_1X);
                            bme680.setPressureOversampling(BME680_OS_1X);
                            bme680.setIIRFilterSize(BME680_FILTER_SIZE_0);
                            logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "BME", " BMP680 Module init done!");
                            break;
                    }
                }
            }            
        }
    }

    String generateTempString(float bmeTemp, String type) {
        String strTemp;
        if (type=="OLED") {
            strTemp = String((int)bmeTemp);
        } else {
            strTemp = String((int)((bmeTemp * 1.8) + 32));
        }
        switch (strTemp.length()) {
            case 1:
                if (type=="OLED") {
                    return "  " + strTemp;
                } else {
                    return "00" + strTemp;
                }
                break;
            case 2:
                if (type=="OLED") {
                    return " " + strTemp;
                } else {
                    return "0" + strTemp;
                }
                break;
            case 3:
                return strTemp;
                break;
            default:
                return "-999";
        }
    }

    String generateHumString(float bmeHum, String type) {
        String strHum;
        strHum = String((int)bmeHum);
        switch (strHum.length()) {
            case 1:
                if (type=="OLED") {
                    return " " + strHum;
                } else {
                    return "0" + strHum;
                }
                break;
            case 2:
                return strHum;
                break;
            case 3:
                if ((int)bmeHum == 100) {
                    if (type=="OLED") {
                        return "  ";
                    } else {
                        return "00";
                    }
                } else {
                    return "-99";
                }
                break;
            default:
                return "-99";
        }
    }

    String generatePresString(float bmePress, String type) {
        String strPress;
        strPress = String((int)bmePress);
        switch (strPress.length()) {
            case 1:
                if (type=="OLED") {
                    return "000" + strPress;
                } else {
                    return "000" + strPress + "0";
                }
                break;
            case 2:
                if (type=="OLED") {
                    return "00" + strPress;
                } else {
                    return "00" + strPress + "0";
                }
                break;
            case 3:
                if (type=="OLED") {
                    return "0" + strPress;
                } else {
                    return "0" + strPress + "0";
                }
                break;
            case 4:
                if (type=="OLED") {
                    return strPress;
                } else {
                    return strPress + "0";
                }
                break;
            case 5:
                return strPress;
                break;
            default:
                return "-99999";
        }
    }

    String readDataSensor(String type) {
        String wx, tempStr, humStr, presStr;
        uint32_t lastReading = millis() - bmeLastReading;
        if (lastReading > 60 * 1000) {
            switch (wxModuleType) {
                case 1: // BME280
                    bme280.takeForcedMeasurement();
                    newTemp     = bme280.readTemperature();
                    newPress    = (bme280.readPressure() / 100.0F);
                    newHum      = bme280.readHumidity();
                    break;
                case 2: // BMP280
                    bmp280.takeForcedMeasurement();
                    newTemp     = bmp280.readTemperature();
                    newPress    = (bmp280.readPressure() / 100.0F);
                    newHum      = 0;
                    break;
                case 3: // BME680
                    bme680.performReading();
                    delay(50);
                    if (bme680.endReading()) {
                        newTemp     = bme680.temperature;
                        newPress    = (bme680.pressure / 100.0F);
                        newHum      = bme680.humidity;
                        newGas      = bme680.gas_resistance / 1000.0; // in Kilo ohms
                    }
                    break;
            }
            bmeLastReading = millis();
        }
        
        if (isnan(newTemp) || isnan(newHum) || isnan(newPress)) {
            Serial.println("BME/BMP Module data failed");
            if (type == "OLED") {
                wx = " - C    - %    - hPa";
            } else {
                wx = ".../...g...t...r...p...P...h..b.....";
            }
            return wx;
        } else {
            tempStr = generateTempString(newTemp + Config.bme.temperatureCorrection, type);
            if (wxModuleType == 1 || wxModuleType == 3) {
                humStr  = generateHumString(newHum,type);
            } else if (wxModuleType == 2) {
                humStr  = "..";
            }
            presStr = generatePresString(newPress + (Config.bme.heightCorrection/CORRECTION_FACTOR), type);
            if (type == "OLED") {
                if (wxModuleType == 1 || wxModuleType == 3) {
                    wx = tempStr + "C   " + humStr + "%   " + presStr + "hPa";
                } else if (wxModuleType == 2) {
                    wx = "T: " + tempStr + "C " + "P: " + presStr + "hPa";
                }
            } else {
                wx = ".../...g...t" + tempStr + "r...p...P...h" + humStr + "b" + presStr;
                if (wxModuleType == 3) {
                    wx += "Gas: " + String(newGas) + "Kohms";
                }
            }
            return wx;
        }
    }

}