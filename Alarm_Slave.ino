//Zamiarem tego projektu jest przerobić mój stary alarm oparty na Switch Case Brake, na program
//który oparty będzie na loopie odwołującym się do podprogramów oraz, na zastosowaniu w tym wszystkim
//watchdoga

//kilka tygodni dalej ... póki co watchdog okazuje sie niewypałem i nie implementuje go do projektu.

//okazało się, że moja płytka, gdzie nie ma arduino, a jest sam mikrokontroler Atmega z clockiem, jest wrażliwa na promieniowanie anteny sim800l
//mikrokontroler resetuje się nawet kiedy założona jest antenka zwykła, ta  okrągła... dlatego wszędzie muszą być dołączone anteny długie.


//Jest problem z czujkami ruchu -  aktywują się po włączeniu alarmu do sieci. Kiedy testowałem alarm lata temu, ten pierwszy, to czujki były cały czas podłączone
//pod zasilanie i tego efektu nie widziałem. Przy kolejnym projekcie, kiedy zrobiłem alarm, dałem długi okres włączania ze względu na SIM800L i dzięki temu nie zauważyłem
// że czujki się aktywują. Teraz w tym alarmie daję minutę na włączenie się loopa, po to właśnie aby nie było alarmów związanych
//z niestabilną pracą czujek PIR po "podłączeniu do sieci". Ta minuta bedzie testowana ale raczej dziala.


/*BARDZO WAŻNA RZECZ... przerabiając program te zmienne powyżej zrobiłem jako globalne. Przy programie w postaci switch case...to nie szkodziło, ale teraz, kiedy
    działa loop i podprogramy to powodowało, że zmienna pinAlarmuPozycja zapamiętywała już wartość 4 po pierwszym wpisywaniu kodu i kiedy trzeba było
    kod wpisać drugi raz to od razu włączał się alarm. Działo się tak ponieważ zgadzała się litera na klawiaturze ale pinpozycja już nie. ROZUMIESZ?
    czyli sekwencja  otworzyłem drzwi, wpisuję kod po raz pierwszy (wszystko jest ok), przechodzę do zawieszenia alarmu(wszystko jest ok) naciskam przycisk i alarm
    znów działa(wszystko jest ok), otwieram drzwi i wpisuję ponownie kod(i od razu alarm) bo pin pozycja w programie zliczona jest jako 4 a nie 1.
    Kiedy zrobiłem zmienną lokalną wszytko jest ok*/

#include <Wire.h>
#include <TimeLib.h>
#include<SoftwareSerial.h>
#include <Keypad.h>


#define r 5
#define g 6
#define b 7
#define buzzer 8
#define garderobaKontaktron 9
#define drzwiKontaktron 10
#define sypialniaPir 11
#define pracowniaPir 12
#define salonPir 13
#define przekaznik A5

//====================================================================================================================================
SoftwareSerial gsm(2, 3); //pod te piny podłączone jest SIM800L pod 2 arduino TX w simie i pod 3 arduino RX w simie
//Ponieważ antena sima gryzie się z clockiem mikrokontrolera to trzeba będzie albo clock dać w inne miejsce, albo właśnie moduł Sim800 przenieść i
//zmienić piny na jakieś inne. Może nawet analogowe, które są na dole lub digital ale z dołu po lewej mikrokontrolera, aby można było
//dać SIMa tam gdzie teraz są kondensatory, daleeeeko od clocka.

const byte ROWS = 1;
const byte COLS = 2;
byte rowPins[ROWS] = {A0};
byte colPins[COLS] = {A1, A2};  //ZMIENNE OGÓLNE
char keys[ROWS][COLS] = {
  {'S', 'T'}
};
Keypad klawiatura = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );



int  ileCzasuMinelo = 0;
char pinCyfra1 = 'S';//START
char pinCyfra2 = 'T';//STOP   //KLAWIATURA


char incomingByte;
String incomingData;
bool atCommand = true;
int index = 0;
String number = "";
String message = "";

unsigned long aktualnyCzasALARM = 0;      //zmienne potrzebne do odliczania 15 sekund bez delaya w alarmie
unsigned long zapamietanyCzasALARMpisy = 0;
unsigned long zapamietanyCzasALARMpipr = 0;
unsigned long zapamietanyCzasALARMpisa = 0;
unsigned long zapamietanyCzasALARMkoga = 0;
unsigned long zapamietanyCzasALARMkodr = 0;

unsigned long zapamietanyCzasDIODAzawieszenie = 0;
unsigned long zapamietanyCzasDIODAzielona = 0;


unsigned long aktualnyCzasPIRSalon = 0;
unsigned long zapamietanyCzasPIRSalon = 0;

unsigned long aktualnyCzasPIRSypialnia = 0;
unsigned long zapamietanyCzasPIRSypialnia = 0;

unsigned long aktualnyCzasPIRPracownia = 0;
unsigned long zapamietanyCzasPIRPracowania = 0;


//===================================================================================================================================
void setup() {
  Serial.begin(9600);
  gsm.begin(9600);

  pinMode(drzwiKontaktron, INPUT_PULLUP);
  pinMode(garderobaKontaktron, INPUT_PULLUP);

  pinMode(sypialniaPir, INPUT);
  pinMode(pracowniaPir, INPUT);
  pinMode(salonPir, INPUT);

  pinMode(r, OUTPUT);
  pinMode(g, OUTPUT);                         //PINMODE
  pinMode(b, OUTPUT);

  pinMode(buzzer, OUTPUT);
  pinMode(przekaznik, OUTPUT);



  digitalWrite(sypialniaPir, LOW);
  digitalWrite(pracowniaPir, LOW);
  digitalWrite(salonPir, LOW);


  digitalWrite(r, LOW);
  digitalWrite(g, LOW);
  digitalWrite(b, LOW);

  delay(30000); // 30 sekund na zalogowanie się do sieci SIM800L



  //--------------------------------ta część jest niezbędna do wysyłania smsów---------------------------------------------------------------------
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
  gsm.println("AT+CMGD=1,4\r\n");//kasuje wszystkie sms-y z pamięci
  Serial.println("Ready to received Commands..");
  //----------------------------------------------------------------------------------------------------------------------------------------------------
  delay(30000); //drugie 30 sekund na ustabilizowanie się czujek PIR po podaniu na nie napięcia.

  sygnalWlaczeniaAlarmu();
}



//#################################################################################LOOP##############################################################################################
void loop() {
  //Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>jestem w loop");
  monitorowanieCzujek();
  delay(500);
  gsm_sprawdzenieSMS();
  delay(500);
}
//####################################################################################################################################################################################

//sygnał właczenia alarmu jest w SETUPIE ... jeśli alarm z jakiegoś powodu bedzie się resetował to trzeba będzie to wyrzucić aby nie było smsów przy każdym resecie
void sygnalWlaczeniaAlarmu() {
  //Serial.println("wlaczanie alarmu");
  for (int i = 0; i < 8; i++) {
    digitalWrite(buzzer, HIGH);
    digitalWrite(b, HIGH);
    delay(50);
    digitalWrite(buzzer, LOW);
    digitalWrite(b, LOW);
    delay(300);
  }
  digitalWrite(buzzer, HIGH);
  delay(25);
  digitalWrite(buzzer, LOW);
  delay(100);
  digitalWrite(buzzer, HIGH);
  delay(25);
  digitalWrite(buzzer, LOW);
  delay(100);
  digitalWrite(buzzer, HIGH);
  delay(25);
  digitalWrite(buzzer, LOW);
  delay(100);
  digitalWrite(buzzer, HIGH);
  delay(50);
  digitalWrite(buzzer, LOW);
  delay(100);

  gsm.write("AT+CMGS=\"+48xxxxxxxxx\"\r\n");//wpisz swój nr telefonu
  delay(100);
  gsm.write("ALARM WLACZONY");
  delay(100);
  gsm.write((char)26);
  delay(100);
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


void monitorowanieCzujek() {
  //Serial.println("jestem w stanie monitorowania");


  aktualnyCzasALARM = millis();                                       //miga dioda zielona
  if ((aktualnyCzasALARM - zapamietanyCzasDIODAzielona) >= 2000UL) {
    digitalWrite(g, HIGH);
    delay(10);
    digitalWrite(g, LOW);
    digitalWrite(r, LOW);
    digitalWrite(b, LOW);
    zapamietanyCzasDIODAzielona = aktualnyCzasALARM;
  }

  if (digitalRead(drzwiKontaktron) == HIGH)
  {
    gsm.write("AT+CMGS=\"+48xxxxxxxxx\"\r\n");//wpisz swój nr telefonu
    delay(100);
    gsm.write("Otwarto drzwi wejsciowe!");
    delay(100);
    gsm.write((char)26);
    delay(100);
    //Serial.println("aktywowany alarm kontaktron dzrzwi wejsciowe");
    rozbrajanieAlarmu();
  }


  if (digitalRead(garderobaKontaktron) == HIGH)
  {
    alarmKontaktronGarderoba();
  }


  if (digitalRead(sypialniaPir) == HIGH)
  {
    alarmPirSypialnia();
  }


  if (digitalRead(pracowniaPir) == HIGH) {
    alarmPirPracownia();
  }


  if (digitalRead(salonPir) == HIGH)
  {
    alarmPirSalon();
  }
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void rozbrajanieAlarmu() {
  char klawisz;
  bool nacisniecia_prawda_falsz = false;
  int  nacisniecia = 0;
  unsigned long zapamietanyCzasDiodaRozbrajanie = 0;
  unsigned long zapamietanyCzasROZBRAJANIE = 0;

  aktualnyCzasALARM = millis(); //pobieramy czas aby odnieść się do niego w srodku while
  zapamietanyCzasROZBRAJANIE = aktualnyCzasALARM; //do zmiennej wpisujemy ten czas, on sie już nie zmieni i wchodzimy do while

  while (nacisniecia_prawda_falsz == false) {
   // Serial.println("wszedłem while rozbrajanie <<<<<<<<<<<<<<<<---------------");

    aktualnyCzasALARM = millis();
    if ((aktualnyCzasALARM - zapamietanyCzasDiodaRozbrajanie) >= 250UL) {//migniecie diody
      digitalWrite(r, HIGH);
      delay(100);
      digitalWrite(r, LOW);
      zapamietanyCzasDiodaRozbrajanie = aktualnyCzasALARM;
    }

   // Serial.println("czekam na nacisniecie klawisza");
    klawisz = klawiatura.getKey();

    if (klawisz) {
      nacisniecia++;
      digitalWrite(r, LOW); //czerwona gasnie
      digitalWrite(buzzer, HIGH);
      digitalWrite(g, HIGH);//sygnal buzzera zapala zielona i gasnie
      delay(200);
      digitalWrite(g, LOW);
      digitalWrite(buzzer, LOW);
      Serial.print("klawisz po nacisnieciu wynosi: "); Serial.println(klawisz);

      if (nacisniecia == 1 && klawisz == pinCyfraX) {//zamiast X podstaw odpowiednie cyfry kodu jaki chcesz mieć. 
        delay(1000);
      } else if (nacisniecia == 2 && klawisz == pinCyfraX) {
        delay(1000);
      } else if (nacisniecia == 3 && klawisz == pinCyfraX) {
        delay(1000);
      } else if (nacisniecia == 4 && klawisz == pinCyfraX) {

        gsm.write("AT+CMGS=\"+48xxxxxxxxx\"\r\n");//wpisz swój nr telefonu
        delay(100);
        gsm.write("Rozbrojono alarm!");
        delay(100);
        gsm.write((char)26);
        delay(100);

        zawieszenieDzialaniaAlarmu();
        nacisniecia_prawda_falsz = true;
        break;
        /* Ten break jest bardzo ważny, co robi?
        przerywa działanie while i nie jest sprawdzane to wszystko co jest poniżej. Kiedy go nie ma to dzieja sie nastepujace rzeczy:
        - alarm wchodzi do zawieszenia dzialania
        - po nacisnieciu przycisku alarm wraca i odczytuje instrukcje nacisniecia_prawda_falsz = true i już wie,  że nie wroci do while
        - ale ... po pominieciu else jest jeszcze warunek czasu. Poniewaz ten warunek bedzie prawdziwy praktycznie zawsze ... bo wpisanie kodu,
        wejscie do zawieszenia, wyjscie z zawieszenia... to bedzie ponad 20 sekund to odpali się alarm kontaktron drzwi i nastepnie wyjdziemy
        z while. Czyli dostniemy sms'a o alarmie. 
        */
      } else {
        alarmKontaktronDrzwi();
        nacisniecia_prawda_falsz = true;
      }
    }
    aktualnyCzasALARM = millis();                                          //teraz pobieramy znowu czas, to bedzie czas biegnacy w whilu
    if ((aktualnyCzasALARM - zapamietanyCzasROZBRAJANIE) >= 20000UL) {    //i non stop w whilu sprawdzamy roznice czasu teraz i czasu przed whilem      
      alarmKontaktronDrzwi();
      nacisniecia_prawda_falsz = true;      
    }
  }
 // Serial.println("wyszedłem z while rozbrajanie ------------------->>>>>>>>>>>>>>>>>>>>>>>>>>>");
}



//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void zawieszenieDzialaniaAlarmu() {
  bool zawieszenie = true;
  char klawisz;

  while (zawieszenie == true) {
    aktualnyCzasALARM = millis();
    if ((aktualnyCzasALARM - zapamietanyCzasDIODAzawieszenie) >= 5000UL) {
      digitalWrite(g, LOW);
      digitalWrite(r, LOW);
      digitalWrite(b, HIGH);
      delay(50);
      digitalWrite(b, LOW);
      zapamietanyCzasDIODAzawieszenie = aktualnyCzasALARM;
    }


    klawisz = klawiatura.getKey();
    //Serial.print("w zawieszeniu nacisnieto klawisz"); Serial.println(klawisz);
    delay(200);
    if (klawisz) {
      digitalWrite(buzzer, HIGH);
      delay(50);
      digitalWrite(buzzer, LOW);
      delay(100);
      //Serial.println(klawisz);
      zawieszenie = false;//kiedy zmieniam stan zawieszenia to while powinien sie zakonczyc i powinnismy wrocic do loopa
    }
  }
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------
void alarmKontaktronDrzwi() {

  //Serial.println("wejscie do funkcji alarm kontaktron drzwi wejsciowe");

  aktualnyCzasALARM = millis();
  if ((aktualnyCzasALARM - zapamietanyCzasALARMkodr) >= 10000UL) {
    //Serial.println("alarm KONTAKTRON drzwi wejsciowe");
    digitalWrite(g, LOW);
    digitalWrite(r, HIGH);
    digitalWrite(buzzer, HIGH);
    delay(3);
    digitalWrite(buzzer, LOW);
    delay(100);
    gsm.write("AT+CMGF=1\r\n");
    delay(100);
    gsm.write("AT+CMGS=\"+48xxxxxxxxx\"\r\n");
    delay(100);
    gsm.write("KONTAKTRON drzwi wejsciowe");
    delay(100);
    gsm.write((char)26);

    zapamietanyCzasALARMkodr = aktualnyCzasALARM;
  }
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void alarmKontaktronGarderoba() {
  //Serial.println("wchodzę do alarm garderoba");
  aktualnyCzasALARM = millis();
  if ((aktualnyCzasALARM - zapamietanyCzasALARMkoga) >= 45000UL) {
    //Serial.println("wchodzę do alarm garderoba środek IFa <<<<<<--------------");
    //Serial.println("alarm KONTAKTRON garderoba");
    digitalWrite(g, LOW);
    digitalWrite(r, HIGH);
    digitalWrite(buzzer, HIGH);
    delay(3);
    digitalWrite(buzzer, LOW);
    delay(100);
    gsm.write("AT+CMGF=1\r\n");
    delay(100);
    gsm.write("AT+CMGS=\"+48xxxxxxxxx\"\r\n");
    delay(100);
    gsm.write("KONTAKTRON garderoba");
    delay(100);
    gsm.write((char)26);

    zapamietanyCzasALARMkoga = aktualnyCzasALARM;
  }
  //Serial.println("wychodzę z alarm garderoba, IF'a wyszełem-------->>>>>>>>>>>>>>");
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void alarmPirSypialnia() {
  aktualnyCzasALARM = millis();
  if ((aktualnyCzasALARM - zapamietanyCzasALARMpisy) >= 10000UL) {
    //Serial.println("alarm PIR sypialnia");
    digitalWrite(g, LOW);
    digitalWrite(r, HIGH);
    digitalWrite(buzzer, HIGH);
    delay(3);
    digitalWrite(buzzer, LOW);
    delay(100);
    gsm.write("AT+CMGF=1\r\n");
    delay(100);
    gsm.write("AT+CMGS=\"+48xxxxxxxxx\"\r\n");
    delay(100);
    gsm.write("PIR sypialnia");
    delay(100);
    gsm.write((char)26);

    zapamietanyCzasALARMpisy = aktualnyCzasALARM;
  }
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void alarmPirPracownia() {
  //Serial.println("alarmPirPracownia()");
  aktualnyCzasALARM = millis();
  if ((aktualnyCzasALARM - zapamietanyCzasALARMpipr) >= 10000UL) {
    //Serial.println("wszedłem do IF'a <<<<<<<<<<<<<<---------------------------");
    digitalWrite(g, LOW);
    digitalWrite(r, HIGH);
    digitalWrite(buzzer, HIGH);
    delay(3);
    digitalWrite(buzzer, LOW);
    delay(100);
    gsm.write("AT+CMGF=1\r\n");
    delay(100);
    gsm.write("AT+CMGS=\"+48xxxxxxxxx\"\r\n");
    delay(100);
    gsm.write("PIR pracownia");
    delay(100);
    gsm.write((char)26);

    zapamietanyCzasALARMpipr = aktualnyCzasALARM;
  }
  //Serial.println("wyszedłem z IF'a --------------------------->>>>>>>>>>>>>>>>>>>>>");
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void alarmPirSalon() {
  aktualnyCzasALARM = millis();
  if ((aktualnyCzasALARM - zapamietanyCzasALARMpisa) >= 10000UL) {
    //Serial.println("alarm PIR salon");
    digitalWrite(g, LOW);
    digitalWrite(r, HIGH);
    digitalWrite(buzzer, HIGH);
    delay(3);
    digitalWrite(buzzer, LOW);
    delay(100);
    gsm.write("AT+CMGF=1\r\n");
    delay(100);
    gsm.write("AT+CMGS=\"+48xxxxxxxxx\"\r\n");
    delay(100);
    gsm.write("PIR salon");
    delay(100);
    gsm.write((char)26);

    zapamietanyCzasALARMpisa = aktualnyCzasALARM;
  }
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------

void gsm_sprawdzenieSMS() {
  //Serial.println("sprawdzenie SMS przychodzące");
  if (gsm.available()) {
    delay(500);
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
      gsm.println("AT+CMGD=1,4\r\n");
      delay(100);
      atCommand = true;
    }
    incomingData = "";
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

    digitalWrite(b, HIGH);
    delay(10);
    digitalWrite(b, LOW);
    delay(1000);
    gsm.write("AT+CMGS=\"+48xxxxxxxxx\"\r\n");             //60 SEKUND ALARM ON SMS "ALARM WLACZONY"
    delay(100);
    gsm.write("X-MasterOFF, Q-MasterON");

    gsm.write((char)26);
    delay(100);
    gsm.write("AT+CMGD=1,4\r\n");
    digitalWrite(buzzer, HIGH);
    delay(10);
    digitalWrite(buzzer, LOW);
    delay(500);
    digitalWrite(buzzer, HIGH);
    delay(10);
    digitalWrite(buzzer, LOW);
  }
  else if (message.indexOf("X") > -1) {

    digitalWrite(b, HIGH);
    delay(10);
    digitalWrite(b, LOW);
    delay(1000);
    digitalWrite(przekaznik, HIGH);
    gsm.write("AT+CMGS=\"+48xxxxxxxxx\"\r\n");             //60 SEKUND ALARM ON SMS "ALARM WLACZONY"
    delay(100);
    gsm.write("MASTER wylaczony!!!");

    gsm.write((char)26);
    delay(100);
    gsm.write("AT+CMGD=1,4\r\n");

    digitalWrite(buzzer, HIGH);
    delay(10);
    digitalWrite(buzzer, LOW);
    delay(500);
    digitalWrite(buzzer, HIGH);
    delay(10);
    digitalWrite(buzzer, LOW);
  }
  else if (message.indexOf("Q") > -1) {

    digitalWrite(b, HIGH);
    delay(10);
    digitalWrite(b, LOW);
    delay(1000);
    digitalWrite(przekaznik, LOW);
    gsm.write("AT+CMGS=\"+48xxxxxxxxx\"\r\n");
    delay(100);
    gsm.write("MASTER wlaczony....");

    gsm.write((char)26);
    delay(100);
    gsm.write("AT+CMGD=1,4\r\n");
    digitalWrite(buzzer, HIGH);
    delay(10);
    digitalWrite(buzzer, LOW);
    delay(500);
    digitalWrite(buzzer, HIGH);
    delay(10);
    digitalWrite(buzzer, LOW);
  }
}
