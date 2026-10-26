#include <Servo.h>

#define flameLeft A0
#define flameFront A1
#define flameRight A5

// Motor Pinleri
const int motorsag_en  = 3;  
const int motorsol_en  = 5;  
const int in_1  = 8; 
const int in_2  = 9; 
const int in_3  = 10; 
const int in_4  = 11; 

char bt;
int pump = 7;
int servo = 6;

// 🔴 ACIL STOP BUTONU (D2)
const int emergencyIn  = 2;  // Butonun bağlı olduğu pin

bool isDone = false;
volatile bool emergencyStop = false; 

// Manuel Mod Takibi
bool manuelModAktif = false; 

Servo myServo;

// ================================================================
// 🔥 AKILLI BEKLEME
// ================================================================
bool akilliBekle(int sure) {
  unsigned long baslangic = millis();
  while (millis() - baslangic < sure) {
    // 1. Donanım (Buton) kontrolü
    if (emergencyStop) { 
      herSeyiKapat(); return true; 
    }
    // 2. Yazılım (Bluetooth Z tuşu) kontrolü
    if (Serial.available()) {
      char c = Serial.peek(); 
      if (c == 'Z') return true; 
    }
  }
  return false; 
}

// 🔥 HER ŞEYİ KAPATAN FONKSİYON
void herSeyiKapat() {
  digitalWrite(pump, LOW); 
  digitalWrite(in_1, LOW); digitalWrite(in_2, LOW);
  digitalWrite(in_3, LOW); digitalWrite(in_4, LOW);
  analogWrite(motorsag_en, 0); analogWrite(motorsol_en, 0);
}

// -------- TEMEL MOTOR FONKSİYONLARI --------
void ileri(){ 
 digitalWrite(in_1, HIGH); digitalWrite(in_2, LOW);
 digitalWrite(in_3, LOW);  digitalWrite(in_4, HIGH);
 analogWrite(motorsag_en, 254); analogWrite(motorsol_en, 254);
}
void geri(){ 
 digitalWrite(in_1, LOW);  digitalWrite(in_2, HIGH);
 digitalWrite(in_3, HIGH); digitalWrite(in_4, LOW);
 analogWrite(motorsag_en, 254); analogWrite(motorsol_en, 254);
}
void dur(){ 
 digitalWrite(in_1, LOW); digitalWrite(in_2, LOW);
 digitalWrite(in_3, LOW); digitalWrite(in_4, LOW);
 analogWrite(motorsag_en, 0); analogWrite(motorsol_en, 0);
}
void sag(){
 digitalWrite(in_1, LOW);  digitalWrite(in_2, LOW);
 digitalWrite(in_3, LOW);  digitalWrite(in_4, HIGH);
 analogWrite(motorsag_en, 254); analogWrite(motorsol_en, 254);
}
void sol(){
 digitalWrite(in_1, HIGH); digitalWrite(in_2, LOW);
 digitalWrite(in_3, LOW);  digitalWrite(in_4, LOW);
 analogWrite(motorsag_en, 254); analogWrite(motorsol_en, 254);
}

// -------- ÖZEL HAREKETLER --------
void sagcapraz(){
 if(emergencyStop) return; 
 ileri();
 if(akilliBekle(100)) return; 
 sag();
 if(akilliBekle(200)) return; 
}
void solcapraz(){
 if(emergencyStop) return;
 ileri();
 if(akilliBekle(100)) return;
 sol();
 if(akilliBekle(200)) return;
}
void front(){
 if(emergencyStop) return;
 ileri();
 if(akilliBekle(200)) return;
}

// -------- SERVO HAREKETLERİ --------
void servo_sag(){
  for(int angle = 90; angle >= 30; angle -= 3){
    if(emergencyStop) { herSeyiKapat(); return; } 
    myServo.write(angle);
    if(akilliBekle(100)) return; 
  }
  for(int angle = 30; angle <= 90; angle += 3){
    if(emergencyStop) { herSeyiKapat(); return; }
    myServo.write(angle);
    if(akilliBekle(100)) return;
  }
}
void servo_sol(){
  for(int angle = 90; angle <= 150; angle += 3){
    if(emergencyStop) { herSeyiKapat(); return; }
    myServo.write(angle);
    if(akilliBekle(100)) return;
  }
  for(int angle = 150; angle >= 90; angle -= 3){
    if(emergencyStop) { herSeyiKapat(); return; }
    myServo.write(angle);
    if(akilliBekle(100)) return;
  }
}
void servo_front(){
  for(int angle = 40; angle <= 130; angle += 3){
    if(emergencyStop) { herSeyiKapat(); return; }
    myServo.write(angle);
    if(akilliBekle(100)) return;
  }
  for(int angle = 130; angle >= 40; angle -= 3){
    if(emergencyStop) { herSeyiKapat(); return; }
    myServo.write(angle);
    if(akilliBekle(100)) return;
  }
}

// -------- BUTON KESMESİ --------
// Butona basıldığında (FALLING) burası çalışır
void acilDurdurmaKesmesi(){
  emergencyStop = true;
  herSeyiKapat(); 
}

// -------- SETUP --------
void setup(){
  Serial.begin(9600);
  pinMode(flameLeft, INPUT); pinMode(flameFront, INPUT); pinMode(flameRight, INPUT);
  pinMode(pump, OUTPUT);
  pinMode(in_1, OUTPUT); pinMode(in_2, OUTPUT);
  pinMode(in_3, OUTPUT); pinMode(in_4, OUTPUT);

  // ⚠️ ÖNEMLİ: INPUT_PULLUP modu kullanıyoruz.
  // Butonun diğer ucu GND'ye bağlı olmalı.
  pinMode(emergencyIn, INPUT_PULLUP); 
  
  myServo.attach(servo);

  // Butona basınca pin LOW'a düşeceği için FALLING (Düşen Kenar) kullanıyoruz
  attachInterrupt(digitalPinToInterrupt(emergencyIn), acilDurdurmaKesmesi, FALLING);
}

// -------- LOOP --------
void loop(){

  // 🔴 1. ACİL STOP (KİLİTLİ DURUM)
  if(emergencyStop){
    herSeyiKapat(); 
    
    // Kilidi açmak için Bluetooth bekle
    if(Serial.available()){
      char c = Serial.read();
      // F, B, L, R, S tuşlarından biri gelirse kilidi aç ve devam et
      if(c=='F' || c=='B' || c=='L' || c=='R' || c=='S'){
        emergencyStop = false;           
        manuelModAktif = false; // Resetlenince otomatik başla
        isDone = false;                  
      }
      
      // Eğer komut hareket komutuysa ve kilit açıldıysa hemen uygula
      if(!emergencyStop){
        if(c=='F') { ileri(); manuelModAktif = true; }
        else if(c=='B') { geri(); manuelModAktif = true; }
        else if(c=='L') { sol(); manuelModAktif = true; }
        else if(c=='R') { sag(); manuelModAktif = true; }
        else if(c=='S') { dur(); manuelModAktif = false; }
      }
    }
  } 
  
  // 🟢 2. NORMAL ÇALIŞMA
  else {
    
    // --- BLUETOOTH OKUMA ---
    if(Serial.available()){
      bt = Serial.read();

      if(bt == 'Z'){
        emergencyStop = true; // Yazılımsal stop
        herSeyiKapat();
        return; 
      }
      
      // Manuel hareket komutları
      else if(bt=='F') { ileri(); manuelModAktif = true; }
      else if(bt=='B') { geri(); manuelModAktif = true; }
      else if(bt=='L') { sol(); manuelModAktif = true; }
      else if(bt=='R') { sag(); manuelModAktif = true; }
      else if(bt=='S') { dur(); manuelModAktif = false; } 
    }

    // --- MANUEL MODDA İSEK YANGINA BAKMA ---
    if(manuelModAktif) {
       return; 
    }

    // --- OTOMATİK YANGIN SÖNDÜRME MODU ---
    int L = analogRead(flameLeft);
    int F = analogRead(flameFront);
    int R = analogRead(flameRight);
    bool flameDetected = (L < 80 || F < 50 || R < 50);

    if(flameDetected && !isDone){
        
        digitalWrite(pump, HIGH); 

        if(R < 50){
           sagcapraz(); 
           if(!emergencyStop) { dur(); servo_sag(); }
        }
        else if(L < 80){
           solcapraz();
           if(!emergencyStop) { dur(); servo_sol(); }
        }
        else if(F < 50){
           front();
           if(!emergencyStop) { dur(); servo_front(); }
        }

        herSeyiKapat();
        if(!emergencyStop) isDone = true;   
    } 
    else {
       herSeyiKapat();
       if(!flameDetected) isDone = false; 
    }
  }
}