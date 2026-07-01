#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <WebServer.h>
#include "Free_Fonts.h"
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <EEPROM.h>
TFT_eSPI tft = TFT_eSPI();
#define CS_PIN  21
#define TIRQ_PIN 22
XPT2046_Touchscreen ts(CS_PIN, TIRQ_PIN);
#include <ArduinoJson.h>
#include <qrcode_espi.h>
#define EEPROM_SIZE 512
#define RXD2 16
#define TXD2 17

const char *midtransAuth = "<Midtrans-Auth>";

String ssid;
String pass;
bool setwifi=false;
int ledState = LOW;  // Current state of the LED
unsigned long previousMillis = 0;  // Will store the last time LED was updated

TFT_eSPI display = TFT_eSPI();
QRcode_eSPI qrcode (&display);

WebServer server(80);


const char *rootCACertificate = "-----BEGIN CERTIFICATE-----\n"
                                "MIICCTCCAY6gAwIBAgINAgPlwGjvYxqccpBQUjAKBggqhkjOPQQDAzBHMQswCQYD\n"
                                "VQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEUMBIG\n"
                                "A1UEAxMLR1RTIFJvb3QgUjQwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAwMDAw\n"
                                "WjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2Vz\n"
                                "IExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjQwdjAQBgcqhkjOPQIBBgUrgQQAIgNi\n"
                                "AATzdHOnaItgrkO4NcWBMHtLSZ37wWHO5t5GvWvVYRg1rkDdc/eJkTBa6zzuhXyi\n"
                                "QHY7qca4R9gq55KRanPpsXI5nymfopjTX15YhmUPoYRlBtHci8nHc8iMai/lxKvR\n"
                                "HYqjQjBAMA4GA1UdDwEB/wQEAwIBhjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQW\n"
                                "BBSATNbrdP9JNqPV2Py1PsVq8JQdjDAKBggqhkjOPQQDAwNpADBmAjEA6ED/g94D\n"
                                "9J+uHXqnLrmvT/aDHQ4thQEd0dlq7A/Cr8deVl5c1RxYIigL9zC2L7F8AjEA8GE8\n"
                                "p/SgguMh1YQdc4acLa/KNJvxn7kjNuK8YAOdgLOaVsjh4rsUecrNIdSUtUlD\n"
                                "-----END CERTIFICATE-----\n";




                                



// HTML code untuk form
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Set Wi-Fi</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <h2>Konfigurasi Wi-Fi</h2>
  <form action="/submit" method="POST">
    <label for="input1">Ssid :</label><br>
    <input type="text" id="input1" name="input1"><br><br>
     <label for="input2">Password :</label><br>
    <input type="text" id="input2" name="input2"><br><br>
    <input type="submit" value="Submit">
  </form>
</body>
</html>
)rawliteral";


void handleRoot() {
server.send(200, "text/html", index_html);
}

void handleFormSubmit() {
  if (server.hasArg("input1")) {
    ssid = server.arg("input1"); 
    writeEEPROM(400,ssid);  
    pass = server.arg("input2");
    writeEEPROM(420,pass);
    showPage(7);
    Serial.println("Received input1: " + ssid);
    Serial.println("Received input2: " + pass);
  } 
  server.send(200, "text/html", index_html);
}

void writeEEPROM(int addrOffset, const String &strToWrite) {
  byte len = strToWrite.length();
  EEPROM.write(addrOffset, len); // Write the length of the string
  for (int i = 0; i < len; i++) {
    EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
  }
  EEPROM.commit(); // Commit changes to EEPROM
}

String readEEPROM(int addrOffset) {
  int len = EEPROM.read(addrOffset); // Read the length of the string
  char text[len + 1];
  for (int i = 0; i < len; i++) {
    text[i] = EEPROM.read(addrOffset + 1 + i);
  }
  text[len] = '\0'; // Null-terminate the string
  return String(text);
}

void clearEEPROM() {
  for (int i = 0; i < 399; i++) {
    EEPROM.write(i, 0); // Write 0 to each address
  }
  EEPROM.commit(); // Commit changes to EEPROM
}

unsigned long setClock() {
  configTime(0, 0, "pool.ntp.org");

  Serial.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    Serial.print(F("."));
    yield();
    nowSecs = time(nullptr);
  }
  Serial.println(nowSecs);
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
  Serial.println();
  return(nowSecs);
}

WiFiMulti WiFiMulti;

int currentPage = 1;
String txcode;
bool error=false;
String txdetails;
int txtype;
int statuscode;
int txstatus;
String value;
String txid;

void drawButton(int x, int y, int w, int h, const char *label) {
  tft.fillRoundRect(x, y, w, h, 10, TFT_BLUE);
  tft.drawRoundRect(x, y, w, h, 10, TFT_WHITE);
  int16_t x1, y1;
  uint16_t w1, h1;
  tft.drawString(label, x+w/2, y+h/2);
}


int reqtx(String x, int y){
  int code;
  const char *txdoc;
  String payload;
  txid="txid"+String(setClock());
  writeEEPROM(3, txid);
   if (y==1){
           payload="{\"payment_type\": \"qris\", \"transaction_details\": {\"order_id\": \"" +txid+ "\", \"gross_amount\": "+x+"}, \"qris\": {\"acquirer\": \"gopay\"}}";
          }
        else if (y==2){
          payload="{\"payment_type\": \"bank_transfer\", \"transaction_details\": { \"gross_amount\": "+x+", \"order_id\": \""+txid+"\" }, \"bank_transfer\":{ \"bank\": \"bni\" }}";
          }
        else if (y==3){
          payload= "{\"payment_type\": \"bank_transfer\", \"transaction_details\": { \"gross_amount\": "+x+", \"order_id\": \""+txid+"\" }, \"bank_transfer\":{ \"bank\": \"bri\" }}";
          }
        else if (y==4){
          payload="{\"payment_type\": \"bank_transfer\", \"transaction_details\": { \"gross_amount\": "+x+", \"order_id\": \""+txid+"\" }, \"bank_transfer\":{ \"bank\": \"permata\" }}";  
          }
  NetworkClientSecure *client = new NetworkClientSecure;
  if (client) {
    client->setCACert(rootCACertificate);

    {
      HTTPClient https;
      Serial.printf("[HTTPS] begin...\n");
      if (https.begin(*client, "https://api.midtrans.com/v2/charge")) {  
        Serial.printf("[HTTPS] POST REQUEST...\n");
        // start connection and send HTTP header
        https.addHeader("Content-Type", "application/Json");
        https.addHeader("Accept", "application/json");
        https.addHeader("Authorization", midtransAuth);
        
        int httpCode = https.POST(payload);
        code=httpCode;
        if (httpCode > 0) {
          Serial.printf("[HTTPS] POST RESPONSE... code: %d\n", httpCode);
          if (httpCode == HTTP_CODE_OK || httpCode == 201) {
            String json = https.getString();
            Serial.println("Response Json: "+ json);
            StaticJsonDocument<768> doc;
            DeserializationError error = deserializeJson(doc, json);

            if (error) {
              Serial.print(F("deserializeJson() failed: "));
              Serial.println(error.f_str());
            } else{
            if (y==1){
               txdoc = doc["qr_string"];
              }
            else if (y==2){
               txdoc = doc["va_numbers"][0]["va_number"];
              }
            else if (y==3){
               txdoc = doc["va_numbers"][0]["va_number"]; 
              }
            else if (y==4){
               txdoc = doc["permata_va_number"]; 
              }
              txcode=txdoc;
              writeEEPROM(40,txcode);
            }
          }
          
        } else {
          Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
        https.end();
        
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }
    }
    delete client;
    
  } else {
    Serial.println("Unable to create client");
  }
  return(code);
  }
 
int statustx (String x){
  int code;
  NetworkClientSecure *client = new NetworkClientSecure;
  if (client) {
    client->setCACert(rootCACertificate);

    {
      HTTPClient https;
      Serial.print("[HTTPS] begin...\n");
      if (https.begin(*client, "https://api.midtrans.com/v2/" +x+ "/status")) {  // HTTPS
        Serial.print("[HTTPS] GET REQUEST...\n");
        // start connection and send HTTP header
        https.addHeader("Content-Type", "application/Json");
        https.addHeader("Accept", "application/json");
        https.addHeader("Authorization", midtransAuth);

        int httpCode = https.GET();
        code=httpCode;
        if (httpCode > 0) {
          Serial.printf("[HTTPS] GET RESPONSE... code: %d\n", httpCode);
          if (httpCode == HTTP_CODE_OK || httpCode == 201) {
            String json = https.getString();
            Serial.println("Response Json: "+ json);
            StaticJsonDocument<768> doc;
            DeserializationError error = deserializeJson(doc, json);

            if (error) {
              Serial.print(F("deserializeJson() failed: "));
              Serial.println(error.f_str());
            }
            const char* codetx = doc["status_code"]; 
            txstatus=atoi(codetx);
            
            const char* transaction_time = doc["transaction_time"];
            String txtime = transaction_time;
            String date = txtime.substring(8, 10) + "/" + txtime.substring(5, 7) + "/" + txtime.substring(0, 4);        
            String times = txtime.substring(11);
            const char* gross_amount = doc["gross_amount"];
            String payment_type;
            if (txtype==1){payment_type="QRIS";}
            else if(txtype==2){payment_type="BNI";}
            else if(txtype==3){payment_type="BRI";}
            else if(txtype==4){payment_type="PERMATA";}
            
            txdetails= txid + "," + date +","+ times +"," + String(gross_amount) +","+ payment_type;
          }
          
        } else {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
        https.end();
        
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }
    }
    delete client;
    
  } else {
    Serial.println("Unable to create client");
  }
  return(code);
  }



void showPage(int page) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);

  switch (page) {
    case 1:
      tft.setTextDatum(4);
      tft.setFreeFont(&FreeSansBold18pt7b);
      tft.drawString("DIGITAL PAYMENT", 240, 20);
      tft.setFreeFont(&FreeSansBold18pt7b);
      tft.drawString("JAYA MANUNGGAL", 240, 50);
      tft.setFreeFont(&FreeSansBold12pt7b);
      tft.setTextDatum(3);
      tft.drawString("1. QRIS Scan", 20, 120);
      tft.drawString("2. Transfer Bank BNI VA", 20, 150);
      tft.drawString("3. Transfer Bank BRI VA", 20, 180);
      tft.drawString("4. Transfer Bank PERMATA VA", 20, 210);
      tft.setTextDatum(4);
      drawButton(180, 250, 120, 50, "BAYAR");
      drawButton(440, 10, 30, 30, "#");
      clearEEPROM();
      break;
      
    case 2:
    value="";
      tft.drawString("Masukan Nominal :", 120, 20);
      tft.drawRect(10, 60, 220, 50, TFT_WHITE); 
      drawButton(10, 260, 120, 50, "BATAL");
drawButton(240, 8, 70, 70, "1");
drawButton(320, 8, 70, 70, "2");
drawButton(400, 8, 70, 70, "3");
drawButton(240, 86, 70, 70, "4");
drawButton(320, 86, 70, 70, "5");
drawButton(400, 86, 70, 70, "6");
drawButton(240, 164, 70, 70, "7");
drawButton(320, 164, 70, 70, "8");
drawButton(400, 164, 70, 70, "9");
drawButton(240, 242, 70, 70, "DEL");
drawButton(320, 242, 70, 70, "0");
drawButton(400, 242, 70, 70, "OK");

      break;
      
    case 3:          
    tft.drawString("Metode Bayar :", 240, 20);
    drawButton(140, 80, 200, 50, "Qris");
    drawButton(140, 140, 200, 50, "BNI VA");
    drawButton(140, 200, 200, 50, "BRI VA");
    drawButton(140, 260, 200, 50, "PERMATA VA");
    drawButton(10, 10, 120, 50, "EDIT");
    break;
    
    case 4:
    EEPROM.write(0, 4);
    EEPROM.commit();
     if (txtype==1){
        // enable debug qrcode
        //qrcode.debug();
        //display.init();
        // Initialize QRcode display using library
        qrcode.init();
        // create qrcode
        qrcode.create(txcode);
        Serial.println("QR String: "+ txcode);
        }
      else if (txtype==2){
          tft.drawRect(10, 80, 300, 50, TFT_WHITE);
          tft.drawRect(10, 160, 300, 50, TFT_WHITE);
          tft.drawRect(10, 240, 300, 50, TFT_WHITE);                    
          tft.setTextDatum(3);
          tft.drawString("Nama Bank :", 10, 65);
          tft.drawString("Nomor Rekening :", 10, 145);
          tft.drawString("Jumlah Yang Dibayarkan :", 10, 225);
          tft.setTextDatum(4);
          tft.drawString("BNI", 160, 105);
          Serial.println("Bank : BNI");
          tft.drawString(txcode, 160, 185);
          Serial.println("Nomor Rekening: "+ txcode);
          tft.drawString(value, 160, 265);
          Serial.println("Jumlah Yang Dibayarkan: "+ value);
        }
      else if (txtype==3){
          tft.drawRect(10, 80, 300, 50, TFT_WHITE);
          tft.drawRect(10, 160, 300, 50, TFT_WHITE);
          tft.drawRect(10, 240, 300, 50, TFT_WHITE);                    
          tft.setTextDatum(3);
          tft.drawString("Nama Bank :", 10, 60);
          tft.drawString("Nomor Rekening :", 10, 140);
          tft.drawString("Jumlah Yang Dibayarkan :", 10, 220);
          tft.setTextDatum(4);
          tft.drawString("BRI", 160, 105);
          Serial.println("Bank : BRI");
          tft.drawString(txcode, 160, 185);
          Serial.println("Nomor Rekening: "+ txcode);
          tft.drawString(value, 160, 265);
          Serial.println("Jumlah Yang Dibayarkan: "+ value);
        }
      else if (txtype==4){
          tft.drawRect(10, 80, 300, 50, TFT_WHITE);
          tft.drawRect(10, 160, 300, 50, TFT_WHITE);
          tft.drawRect(10, 240, 300, 50, TFT_WHITE);                    
          tft.setTextDatum(3);
          tft.drawString("Nama Bank :", 10, 60);
          tft.drawString("Nomor Rekening :", 10, 140);
          tft.drawString("Jumlah Yang Dibayarkan :", 10, 220);
          tft.setTextDatum(4);
          tft.drawString("PERMATA", 160, 105);
          Serial.println("Bank : PERMATA");
          tft.drawString(txcode, 160, 185);
          Serial.println("Nomor Rekening: "+ txcode);
          tft.drawString(value, 160, 265);
          Serial.println("Jumlah Yang Dibayarkan: "+ value);
        }
    tft.setFreeFont(&FreeSansBold9pt7b);
    drawButton(337, 260, 130, 50, "KONFIRMASI");
    tft.setFreeFont(&FreeSansBold12pt7b);
    drawButton(340, 10, 120, 50, "UBAH");   
    break;
    
    case 5:
    EEPROM.write(0, 5);
    EEPROM.commit();
    tft.setFreeFont(&FreeSansBold18pt7b);
    tft.drawString("Status Transaksi :", 240, 30);
    tft.setFreeFont(&FreeSansBold12pt7b);
    drawButton(10, 260, 120, 50, "KEMBALI");
    Serial.println(txstatus);
    
    if (txstatus==200){
          tft.setFreeFont(&FreeSansBold18pt7b);
          tft.drawString("Transaksi Sukses", 240, 150);
          Serial.println("Transaksi Sukses");
          tft.setFreeFont(&FreeSansBold12pt7b);
          Serial2.println(txdetails);
          Serial.println("Cetak Bukti Transaksi");
          Serial.println(txdetails);
          delay(3000);
          currentPage = 1;
          showPage(currentPage);
    }else{
          drawButton(165, 240, 150, 70, "REFRESH"); 
          tft.setFreeFont(&FreeSansBold18pt7b);
          tft.drawString("Transaksi Pending", 240, 150);
          Serial.printf("Transaksi Pending");
          tft.setFreeFont(&FreeSansBold12pt7b);
          }
    break;
    
    case 6:
    if(WiFi.status() != WL_CONNECTED){      
      drawButton(10, 250, 120, 50, "RESTART");
      }else{
      drawButton(10, 250, 120, 50, "BATAL");
      }
     
     drawButton(350, 250, 120, 50, "SETTING");
    break;

   case 7:
   tft.drawString("SSID:", 122, 10);
   tft.drawString("PASSWORD:", 347, 10);
   tft.drawRect(10, 30, 225, 30, TFT_WHITE); 
   tft.drawRect(245, 30, 225, 30, TFT_WHITE); 
    tft.drawString(ssid, 120, 45);
    tft.drawString(pass, 360, 45);

   tft.setFreeFont(&FreeSans12pt7b);
   tft.drawString("SSID: Set Wi-Fi", 120, 135);
   tft.drawString("Password: 12345678", 355, 135);
   tft.drawRect(10, 120, 225, 30, TFT_WHITE); 
   tft.drawRect(245, 120, 225, 30, TFT_WHITE); 
   tft.drawRect(10, 195, 220, 30, TFT_WHITE); 
   tft.drawString("192.168.4.1", 120, 210);

   tft.setTextDatum(ML_DATUM);
   tft.drawString("1. Hubungkan HP/PC dengan Wi-Fi berikut:", 10, 105);
   tft.drawString("2. Buka alamat berikut pada browser:", 10, 180);
    tft.setTextDatum(MC_DATUM);
   tft.setFreeFont(&FreeSansBold12pt7b);
   drawButton(180, 250, 120, 50, "APPLY");
    break;
  }
}

void handleTouch(int x, int y) {
  // Page 1: Next button
  if (currentPage == 1 && x >= 180 && x <= 300 && y >= 250 && y <= 300) {
    currentPage = 2;
    digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
    showPage(currentPage);
  }

   // Page 1:# button
  if (currentPage == 1 && x >= 400 && x <= 470 && y >= 0 && y <= 50) {
    currentPage = 6;
    digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
    showPage(currentPage);
    tft.setFreeFont(&FreeSans12pt7b);
    tft.drawString("Setting untuk mengubah konfigurasi Wi-Fi", 240, 160);
    tft.setFreeFont(&FreeSansBold12pt7b);
  }
  
  // Page 2: BACK button 
  else if (currentPage == 2 && x >= 10 && x <= 130 && y >= 260 && y <= 310) {
    currentPage = 1;
    digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
    showPage(currentPage);
  }
  
//KEYPAD
else if (currentPage == 2 && x >= 240 && x <= 310 && y >= 8 && y <= 78) {
  digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
  value += "1";
  tft.fillRect(11, 61, 218, 48, TFT_BLACK);
  tft.drawString(value, 120, 85);
}
else if (currentPage == 2 && x >= 320 && x <= 390 && y >= 8 && y <= 78) {
  digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
  value += "2";
  tft.fillRect(11, 61, 218, 48, TFT_BLACK);
  tft.drawString(value, 120, 85);
}
else if (currentPage == 2 && x >= 400 && x <= 470 && y >= 8 && y <= 78) {
  digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
  value += "3";
  tft.fillRect(11, 61, 218, 48, TFT_BLACK);
  tft.drawString(value, 120, 85);
}
else if (currentPage == 2 && x >= 240 && x <= 310 && y >= 86 && y <= 156) {
  digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
  value += "4";
  tft.fillRect(11, 61, 218, 48, TFT_BLACK);
  tft.drawString(value, 120, 85);
}
else if (currentPage == 2 && x >= 320 && x <= 390 && y >= 86 && y <= 156) {
  digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
  value += "5";
  tft.fillRect(11, 61, 218, 48, TFT_BLACK);
  tft.drawString(value, 120, 85);
}
else if (currentPage == 2 && x >= 400 && x <= 470 && y >= 86 && y <= 156) {
  digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
  value += "6";
  tft.fillRect(11, 61, 218, 48, TFT_BLACK);
  tft.drawString(value, 120, 85);
}
else if (currentPage == 2 && x >= 240 && x <= 310 && y >= 164 && y <= 234) {
  digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
  value += "7";
  tft.fillRect(11, 61, 218, 48, TFT_BLACK);
  tft.drawString(value, 120, 85);
}
else if (currentPage == 2 && x >= 320 && x <= 390 && y >= 164 && y <= 234) {
  digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
  value += "8";
  tft.fillRect(11, 61, 218, 48, TFT_BLACK);
  tft.drawString(value, 120, 85);
}
else if (currentPage == 2 && x >= 400 && x <= 470 && y >= 164 && y <= 234) {
  digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
  value += "9";
  tft.fillRect(11, 61, 218, 48, TFT_BLACK);
  tft.drawString(value, 120, 85);
}
else if (currentPage == 2 && x >= 240 && x <= 310 && y >= 242 && y <= 312) {
  digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
  value.remove(value.length() - 1);
  tft.fillRect(11, 61, 218, 48, TFT_BLACK);
  tft.drawString(value, 120, 85);
}
else if (currentPage == 2 && x >= 320 && x <= 390 && y >= 242 && y <= 312) {
  digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
  value += "0";
  tft.fillRect(11, 61, 218, 48, TFT_BLACK);
  tft.drawString(value, 120, 85);
}
else if (currentPage == 2 && x >= 400 && x <= 470 && y >= 242 && y <= 312) {
  digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
  if (value.toInt()>999){
      writeEEPROM(20, value);
      currentPage = 3;
      showPage(currentPage);
  } 
      else {
      tft.drawString("Minimum Transaksi", 120,120 );
      tft.drawString("1000", 120,140 );
  }
}
//KEYPAD
  
// Page 3: QRIS button
  else if (error==false && currentPage == 3 && x >= 140 && x <= 340 && y >= 80 && y <= 130) {
    digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
    txtype = 1;
    EEPROM.write(1, txtype);
    EEPROM.commit();
    statuscode=reqtx(value,txtype);
    Serial.printf("Status Code: %d\n", statuscode);
      if (statuscode==200){
        currentPage = 4;
        showPage(currentPage);
      }
    else{
        error=true;
        tft.fillScreen(TFT_BLACK);
        drawButton(140, 140, 200, 50, "RESTART");
        tft.drawString("Error Status Code:", 240,20);
        tft.drawNumber(statuscode,240,50);
        Serial.println("Error Status Code:"+statuscode);
      }
  }

// Page 3: BNI button
  else if (error==false &&currentPage == 3 && x >= 140 && x <= 340 && y >= 140 && y <= 190) {
    digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
    txtype = 2;
    EEPROM.write(1, txtype);
    EEPROM.commit();
    statuscode=reqtx(value,txtype);
    Serial.printf("Status Code: %d\n", statuscode);
    if (statuscode==200){
        currentPage = 4;
        showPage(currentPage);
      }
    else{
        error=true;
        tft.fillScreen(TFT_BLACK);
        drawButton(140, 140, 200, 50, "RESTART");
        tft.drawString("Error Status Code:", 240,20);
        tft.drawNumber(statuscode,240,50);
        Serial.println("Error Status Code:"+statuscode);
      }
  }

// Page 3: BRI button
  else if (error==false && currentPage == 3 && x >= 140 && x <= 340 && y >= 200 && y <= 250) {
    digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
    txtype = 3;
    EEPROM.write(1, txtype);
    EEPROM.commit();
    statuscode=reqtx(value,txtype);
    Serial.printf("Status Code: %d\n", statuscode);
    if (statuscode==200){
        currentPage = 4;
        showPage(currentPage);
      }
    else{
        error=true;
        tft.fillScreen(TFT_BLACK);
        drawButton(140, 140, 200, 50, "RESTART");
        tft.drawString("Error Status Code:", 240,20);
        tft.drawNumber(statuscode,240,50);
        Serial.println("Error Status Code:"+statuscode);
      }
  }

// Page 3: PERMATA button
  else if (error==false && currentPage == 3 && x >= 140 && x <= 340 && y >= 260 && y <= 310) {
    digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
    txtype = 4;
    EEPROM.write(1, txtype);
    EEPROM.commit();
    statuscode=reqtx(value,txtype);
    Serial.printf("Status Code: %d\n", statuscode);
    if (statuscode==200){
        currentPage = 4;
        showPage(currentPage);
      }
    else{
        error=true;
        tft.fillScreen(TFT_BLACK);
        drawButton(140, 140, 200, 50, "RESTART");
        tft.drawString("Error Status Code:", 240,20);
        tft.drawNumber(statuscode,240,50);
        Serial.println("Error Status Code:"+statuscode);
      }
  }  
  
// Page 3: BACK button
  else if (currentPage == 3 && x >= 10 && x <= 130 && y >= 10 && y <= 60) {
    digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
    currentPage = 2;
    showPage(currentPage);
  } 

   // Page 4: BACK button
  else if (currentPage == 4 && x >= 340 && x <= 460 && y >= 10 && y <= 60) {
    digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
    currentPage = 3;
    showPage(currentPage);
  }

   // Page 4: CONFIRM button
  else if (currentPage == 4 && x >= 337 && x <= 467 && y >= 260 && y <= 310) {
    digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
    currentPage = 5;
    statuscode=statustx(txid);
    Serial.printf("Status Code: %d\n", statuscode);
    if (statuscode==200){
        showPage(currentPage);
      }
    else{
        error=true;
        tft.fillScreen(TFT_BLACK);
        drawButton(140, 140, 200, 50, "RESTART");
        tft.drawString("Error Status Code:", 240,20);
        tft.drawNumber(statuscode,240,50);
        Serial.println("Error Status Code:"+statuscode);
      }
  }

// Page 5: BACK button
  else if (currentPage == 5 && x >= 10 && x <= 130 && y >= 260 && y <= 310) {
    digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
    currentPage = 4;
    showPage(currentPage);
  }

    // Page 5: REFRESH button
  else if (txstatus != 200 && currentPage == 5 && x >= 165 && x <= 315 && y >= 240 && y <= 310) {
    digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
    statuscode=statustx(txid);
    
    if (statuscode==200){
        currentPage = 5;
        showPage(currentPage);
      }
    else{
        error=true;
        tft.fillScreen(TFT_BLACK);
        drawButton(140, 140, 200, 50, "RESTART");
        tft.drawString("Error Status Code:", 240,20);
        tft.drawNumber(statuscode,240,50);
        Serial.println("Error Status Code:"+statuscode);
      }
  }

//Page 6 Restart Button
else if (WiFi.status() != WL_CONNECTED&&currentPage == 6 && x >= 10 && x <= 110 && y >= 250 && y <= 300) {
  digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
        esp_restart();
      }
      
//Page 6 BACK button
else if (WiFi.status() == WL_CONNECTED&&currentPage == 6 && x >= 10 && x <= 110 && y >= 250 && y <= 300) {
  currentPage = 1;
  digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
        showPage(currentPage);
      }

//Page 6 Set Wi-Fi button 
else if (currentPage == 6 && x >= 350 && x <= 470 && y >= 250 && y <= 300) {
 WiFi.disconnect();
  digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);

    
  // Mengatur ESP32 sebagai Access Point
  WiFi.softAP("Set Wi-Fi", "12345678");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Route untuk halaman utama
  server.on("/", handleRoot);

  // Route untuk menerima data dari form
  server.on("/submit", HTTP_POST, handleFormSubmit);

  // Memulai server
  server.begin();
  setwifi=true;
    currentPage = 7;
        showPage(currentPage);
      }

// Page 7 apply button
  else if (currentPage==7 && x >= 180 && x <= 300 && y >= 250 && y <= 300) {
  digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
        esp_restart();
      }

else if (error == true && x >= 140 && x <= 340 && y >= 140 && y <= 190) {
  digitalWrite(25, HIGH);
    delay(100);
    digitalWrite(25, LOW);
        esp_restart();
      }
}



void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
   EEPROM.begin(EEPROM_SIZE);
tft.init();
  tft.setRotation(1); // Set the orientation of the display
  uint16_t calData[5] = { 297, 3524, 287, 3493, 7 };
  tft.setTouch(calData);
  ts.begin();
  ts.setRotation(3);
  tft.setFreeFont(&FreeSansBold12pt7b);
  tft.setTextDatum(4);
  tft.fillScreen(TFT_BLACK);

   pinMode(25, OUTPUT);
   pinMode(26, OUTPUT);

  
  int restorepage;
  restorepage=EEPROM.read(0);
  ssid=readEEPROM(400);
  ssid.trim();
  pass=readEEPROM(420);
  pass.trim();
  Serial.println(ssid.length());
    Serial.println(pass.length());
if (ssid.length()==111||pass.length()==91){
        currentPage=6;
      showPage(currentPage);
      tft.drawString("Konfigurasi Wi-Fi belum diatur", 240, 160);
  }else{
   WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid.c_str(), pass.c_str());
  // wait for WiFi connection
  Serial.print("Waiting for WiFi to connect...");
  tft.drawString("Menghubungkan WiFi", 240, 160);
  while ((WiFiMulti.run() != WL_CONNECTED)) {
    Serial.print(".");
    ledState = (ledState == LOW) ? HIGH : LOW;
    digitalWrite(26, ledState);
    if(millis()>10000){
      break;
      }
  }

  if (WiFi.status() != WL_CONNECTED) {
      currentPage=6;
      showPage(currentPage);
      tft.drawString("Gagal menghubungkan Wi-Fi", 240, 160);
   }
   else {
      Serial.println("connected");
      tft.fillScreen(TFT_BLACK);
      tft.drawString("Mensinkronkan Waktu", 240, 160);
      setClock();
        if (restorepage==5||restorepage==4){
          txtype=EEPROM.read(1);
          txid=readEEPROM(3);
          value=readEEPROM(20);
          txcode=readEEPROM(40);
          currentPage=restorepage;
          showPage(currentPage);
        }else{
           showPage(currentPage);
        }
  }
  }
}

void loop() {
  
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    // Map the touchscreen coordinates to the display coordinates
    int16_t x = map(p.x, 297, 3524, 0, tft.width());
    int16_t y = map(p.y, 287, 3493, 0, tft.height()); 
    handleTouch(x, y);
    delay(200);
  }

  if (WiFi.status() != WL_CONNECTED) {
    if (millis()/1000 - previousMillis >= 1) {
    previousMillis = millis()/1000;
    ledState = (ledState == LOW) ? HIGH : LOW;
    Serial.println(ledState);
  }
  } else {
    ledState = HIGH;
  }

  digitalWrite(26, ledState);
  
  if(setwifi==true){
    server.handleClient();
  }
}
