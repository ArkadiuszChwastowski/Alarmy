#include <SoftwareSerial.h>





#define SIM800_TX_PIN 2
#define SIM800_RX_PIN 3
SoftwareSerial gsm(SIM800_TX_PIN, SIM800_RX_PIN);

#define przekaznik A3
#define kontaktron 9

#define LEDred 7
#define LEDgreen 6
#define LEDblue 5
#define buzzer 10

int index = 0;
bool stan_kontaktron = true;
bool stan_przekaznik = false;
String number = "";
String message = "";

char incomingByte;
String incomingData;
bool atCommand = true;

unsigned long aktualnyCzas = 0;
unsigned long zapamietanyCzas = 0;

unsigned long aktualnyCzasLED = 0;
unsigned long zapamietanyCzasLED = 0;

unsigned long aktualnyCzasKON = 0;
unsigned long zapamietanyCzasKON = 0;
unsigned long zapamietanyCzasDELwiadomosci = 0;






void setup()
{
  Serial.begin(9600);
  Serial.println("początek SETUPU");
  gsm.begin(9600);

  pinMode(przekaznik, OUTPUT);
  pinMode(LEDred, OUTPUT);
  pinMode(LEDgreen, OUTPUT);
  pinMode(LEDblue, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(kontaktron, INPUT_PULLUP);


  for (int i; i <= 15; i++) {
    Serial.print(i);
    Serial.print(",");
    delay(1000);
  }

  while (!gsm.available()) {
    gsm.println("AT");
    delay(500);
    Serial.println("connecting....");
  }
  Serial.println("Connected..");
  gsm.println("AT+CMGF=1");  //Set SMS Text Mode
  delay(500);
  gsm.println("AT+CNMI=3,2,0,0,0");  //procedure, how to receive messages from the network
  delay(500);
  gsm.println("AT+CMGD=1,4\r\n");

  Serial.println("Ready to received Commands..");

  sygnalWlaczeniaAlarmu();

}
//######################################################################### L O O P ###################################################################################################
void loop() {


  Serial.println("poczatek LOOP'a");
  delay(500);
  gsm_sprawdzenieSMS();

  aktualnyCzasLED = millis();
  if (((aktualnyCzasLED - zapamietanyCzasLED) >= 5000UL) && stan_przekaznik == false) {
    digitalWrite(LEDgreen, HIGH);
    delay(10);
    digitalWrite(LEDgreen, LOW);
    zapamietanyCzasLED = aktualnyCzasLED;
  } else if (((aktualnyCzasLED - zapamietanyCzasLED) >= 5000UL) && stan_przekaznik == true) {
    digitalWrite(LEDred, HIGH);
    delay(10);
    digitalWrite(LEDred, LOW);
    zapamietanyCzasLED = aktualnyCzasLED;
  }

  
  
    sprawdzenieKontaktron();
   
}

//############################################################################################################################################################################


//________________________________________SPRAWDZENIE SMS I ZADEKLAROWANIE CO MA BYĆ ZROBIONE PO OTRZYMANIU SMS ODPOWIEDNIEJ TREŚCI________________________________________________________
void gsm_sprawdzenieSMS() {
  if (gsm.available()) {

    delay(500); //TEN DELAY JEST  WAŻNY BO IM KRÓCEJ, A BYŁO 100, TYM GORZEJ DZIAŁA TEN PROGRAM, SMSMy W CAŁY ŚWIAT ...
    
    while (gsm.available()) {
      incomingByte = gsm.read();
      incomingData += incomingByte;
    }

    delay(10);
    if (atCommand == false) {
      receivedMessage(incomingData);
    } else {
      atCommand = false;
    }

    //delete messages to save memory
    if (incomingData.indexOf("OK") == -1) {
      gsm.println("AT+CMGDA=\"DEL ALL\"");
      delay(100);
      atCommand = true;
    }

    incomingData = "";
    digitalWrite(LEDred, LOW);
  }
}

void receivedMessage(String inputString) {

  //Get The number of the sender
  index = inputString.indexOf('"') + 1;
  inputString = inputString.substring(index);
  index = inputString.indexOf('"');
  number = inputString.substring(0, index);
  Serial.println("Number: " + number);

  //Get The Message of the sender
  index = inputString.indexOf("\n") + 1;
  message = inputString.substring(index);
  message.trim();
  Serial.println("Message: " + message);
  message.toUpperCase(); // uppercase the message received

  //                                                ---------------DEKLARACJE CO MA SIE DZIAĆ PO OTRZYMANIU DANEJ TREŚCI SMS--------------------------

  if (message.indexOf("?") > -1) {

    digitalWrite(LEDblue, HIGH);
    delay(10);
    digitalWrite(LEDblue, LOW);
    delay(1000);
    gsm.write("AT+CMGS=\"+48xxxxxxxxx\"\r\n");             //60 SEKUND ALARM ON SMS "ALARM WLACZONY"
    delay(100);
    gsm.write("X-SlaveOFF, Q-SlaveON, W-MasKonOFF, E-MasKonON");
    
    gsm.write((char)26);
    delay(100);
    gsm.write("AT+CMGD=1,4\r\n");
    digitalWrite(buzzer, HIGH);
    delay(10);
    digitalWrite(buzzer, LOW);
  }
  else if (message.indexOf("X") > -1) {
    stan_przekaznik = true;
    digitalWrite(LEDblue, HIGH);
    delay(10);
    digitalWrite(LEDblue, LOW);
    delay(1000);
    digitalWrite(przekaznik, HIGH);
    gsm.write("AT+CMGS=\"+48xxxxxxxxx\"\r\n");             //60 SEKUND ALARM ON SMS "ALARM WLACZONY"
    delay(100);
    gsm.write("SLAVE wylaczony!");
   
    gsm.write((char)26);
    delay(100);
    gsm.write("AT+CMGD=1,4\r\n");

    
  }
  else if (message.indexOf("Q") > -1) {
    stan_przekaznik = false;
    digitalWrite(LEDblue, HIGH);
    delay(10);
    digitalWrite(LEDblue, LOW);
    delay(1000);
    digitalWrite(przekaznik, LOW);
    gsm.write("AT+CMGS=\"+48xxxxxxxxx\"\r\n");
    delay(100);
    gsm.write("SLAVE wlaczony!");
   
    gsm.write((char)26);
    delay(100);
    gsm.write("AT+CMGD=1,4\r\n");
    
  }
  else if (message.indexOf("W") > -1) {

    digitalWrite(LEDblue, HIGH);
    delay(10);
    digitalWrite(LEDblue, LOW);
    delay(1000);
    stan_kontaktron = false;
    gsm.write("AT+CMGS=\"+48xxxxxxxxx\"\r\n");
    delay(100);
    gsm.write("Master kontaktron wylaczony");
    
    gsm.write((char)26);
    delay(100);
    gsm.write("AT+CMGD=1,4\r\n");
   
  }
  else if (message.indexOf("E") > -1) {

    digitalWrite(LEDblue, HIGH);
    delay(10);
    digitalWrite(LEDblue, LOW);
    delay(1000);
    stan_kontaktron = 1;
    gsm.write("AT+CMGS=\"+48xxxxxxxxx\"\r\n");
    delay(100);
    gsm.write("Master kontaktron wlaczony");
    
    gsm.write((char)26);
    delay(100);
    gsm.write("AT+CMGD=1,4\r\n");
    
  }
  delay(50);// Added delay between two readings
}

//--------------------------------------------------------------------------------------



void sprawdzenieKontaktron() {

  if (stan_kontaktron == true) {          //jesli zmienna kontaktron, ktora moge przyslac smsem ==1
    if (digitalRead(kontaktron) == LOW) {
      //Serial.println("kontaktron zwarty do masy");
    } else if (digitalRead(kontaktron) == HIGH) {
      delay(1000);
      //Serial.println("Stan wysoki na kontaktronie");
      aktualnyCzas = millis();
      if ((aktualnyCzas - zapamietanyCzas) >= 50000UL) {
        gsm.write("AT+CMGS=\"+48xxxxxxxxx\"\r\n");
        delay(100);
        gsm.write("Alarm MASTER kontaktron garderoba!");
        delay(100);
        gsm.write((char)26);
        delay(100);
        gsm.write("AT+CMGD=1,4\r\n");//ka
        zapamietanyCzas = aktualnyCzas;
      }
    }
  } else if (stan_kontaktron == false) {
    Serial.println("nie sprawdzam kontaktronu");
  }
}

//-----------------------------------------------------------------------------------------------------

void sygnalWlaczeniaAlarmu() {
  
  digitalWrite(buzzer, HIGH);
  delay(25);
  digitalWrite(buzzer, LOW);
  delay(100);
  digitalWrite(buzzer, HIGH);
  delay(25);
  digitalWrite(buzzer, LOW);
  delay(100);
  

  gsm.write("AT+CMGS=\"+48xxxxxxxxx\"\r\n");
  delay(100);
  gsm.write("ALARM WLACZONY");
  delay(100);
  gsm.write((char)26);
  delay(100);
}
