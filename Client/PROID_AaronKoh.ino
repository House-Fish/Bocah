#include "DHT.h"
#include "credentials.h"

#define DHTPIN 13
#define DHTTYPE DHT11
#define LEDG 18  // Green LED on small PB-LED board
#define mintxinterval 15000

// Default band is EU868
// Comment the first line below and uncomment the second line to use AS923 band
#define EU868
// #define AS923

DHT dht(DHTPIN, DHTTYPE);

char recv_buf[512];
bool is_exist = false;
bool is_join = false;
char AT_Appkey[100];
long timer = 0;

void setup(void) {
  Serial.begin(115200);
  tts_init();
  dht.begin();

  pinMode(LEDG, OUTPUT);  // LOW to turn ON
}

void loop(void) {
  if (millis() > timer) {
    float temp = dht.readTemperature();
    float humi = dht.readHumidity();

    Serial.printf("\nTemperature: %.2f *C\t", temp);
    Serial.printf("Humidity: %.2f %%\n", humi);

    if (is_exist) {   // if LoRa device is found
      if (is_join) {  // true for once only after setup() to join network
        if (at_send_check_response("+JOIN: Network joined", 12000, "AT+JOIN\r\n")) {
          is_join = false;  // do not join network again in the next loop
        } else {
          at_send_check_response("+ID: AppEui", 1000, "AT+ID\r\n");
          Serial.print("JOIN failed!\r\n\r\n");
          delay(10000);
        }
      } else {  // combine sensor data with AT command to transmit(uplink) and receive (downlink) if any
        char cmd[128];
        sprintf(cmd, "AT+CMSGHEX=\"%04X%04X\"\r\n", (int)temp, (int)humi);
        if (at_send_check_response("Done", 10000, cmd)) {
          if (recv_parse(recv_buf) == 0) {
            Serial.println("No RX data received.");
          }          
        } else {
          Serial.print("Send failed!\r\n\r\n");
        }
        timer = millis() + max(mintxinterval, 20000);  // LoRa is not meant to transmit data too often (Fair Use Policy)
      }
    } else {
      timer = millis() + 1000;  // if no LoRa device is found, read sensor data every 1 second
    }
  }
}




/* Functions  */

void tts_init() {
  delay(5000);          // let LoRa E5 start up before initializing it
  Serial2.begin(9600);  // Setup communication with Lora-E5 module that is connected to UART2
  Serial.print("E5 LORAWAN INIT\r\n");

  if (at_send_check_response("+AT: OK", 100, "AT\r\n")) {
    is_exist = true;
    at_send_check_response("+ID: AppEui", 1000, "AT+ID\r\n");
    at_send_check_response("+MODE: LWOTAA", 1000, "AT+MODE=LWOTAA\r\n");

#ifdef EU868
    at_send_check_response("+DR: EU868", 1000, "AT+DR=EU868\r\n");
    at_send_check_response("+CH: NUM", 1000, "AT+CH=NUM,0-2\r\n");
#endif

#ifdef AS923
    at_send_check_response("+DR: AS923", 1000, "AT+DR=AS923\r\n");
    at_send_check_response("+CH: NUM", 1000, "AT+CH=NUM,0-1\r\n");
#endif

    sprintf(AT_Appkey, "\"AT+KEY=APPKEY,\"%s\"\r\n\"", Appkey);

    //at_send_check_response("+CH: NUM", 1000, "AT+CH=NUM,0-1\r\n");
    at_send_check_response("+KEY: APPKEY", 1000, AT_Appkey);
    at_send_check_response("+CLASS: C", 1000, "AT+CLASS=A\r\n");
    at_send_check_response("+PORT: 8", 1000, "AT+PORT=8\r\n");

    delay(200);
    Serial.println("LoRaWAN");
    is_join = true;
  } else {
    is_exist = false;
    Serial.print("No E5 module found.\r\n");
  }
}

// This function sends command (p_cmd) to LoRa E5 module,
// stores its response (recv_buf), and checks whether it is correct or not:
// returns 1 if the response is correct (according to p_ack)
// returns 0 if the response is wrong
int at_send_check_response(const char *p_ack, int timeout_ms, const char *p_cmd) {
  int ch;
  int index = 0;
  int startMillis = 0;
  memset(recv_buf, 0, sizeof(recv_buf));  // set recv_buf to zeros
  Serial2.printf(p_cmd);                  // send command to LoRa E5
  Serial.printf(p_cmd);
  delay(200);

  startMillis = millis();

  if (p_ack == NULL) {
    return 0;
  }

  do {
    while (Serial2.available() > 0) {  // as long as LoRa module receives data
      ch = Serial2.read();             // read the data
      recv_buf[index++] = ch;          // store in recv_buf
      Serial.print((char)ch);          // display in Serial Monitor
      delay(2);
    }

    if (strstr(recv_buf, p_ack) != NULL) {  // if p_ack is found in recv_buf, return 1
      return 1;
    }

  } while (millis() - startMillis < timeout_ms);  // Read the response from LoRa E5 within the timeout_ms
  return 0;
}

// This function takes the received message (recv_buf)
// and looks for the downlink data:
// returns 0 if there is no downlink data, otherwise
// returns 1
char recv_parse(char *p_msg) {
  if (p_msg == NULL) {
    return NULL;
  }
  char *p_start = NULL;

  char data;

  p_start = strstr(p_msg, "RX");
  if (p_start && sscanf(p_start, "RX: \"%2x\"", &data) == 1) {
    Serial.printf("rx: %d\n", data);
    toggleLEDG(data);
    return 1;
  }

  return 0; 
}

void toggleLEDG(char val) {
  if (val == 1)      // if RX data = 0x01
    digitalWrite(LEDG, LOW);  // ON LED
  else if (val == 0)
    digitalWrite(LEDG, HIGH);  // OFF LED
}
