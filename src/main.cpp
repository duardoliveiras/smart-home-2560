#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <DallasTemperature.h>
#include <IRremote.h>
#include <EtherCard.h>
IRsend irsend; // instancia o objeto emissor de sinal IR

LiquidCrystal_I2C lcd(0x27,16,2);  // instancia o LCD

#define RECEPTOR 0 // 1 para rebecer sinal IR do controle

#if RECEPTOR // variaveis e funcoes para recepcao de sinal IR
#define maxLen 800
#define pinIR 2
#define LEDPIN 13
volatile  unsigned int irBuffer[maxLen];
volatile unsigned int x = 0;

void IR_Interrupt_Handler() {
  if (x > maxLen) return; 
  irBuffer[x++] = micros(); 

}
#endif

#define gas A0
#define rain A1
#define valorNormal 300

static byte myip[] = {192,168,0,100}; // endereco IP 
static byte gwip[] = {192,168,0,1}; // endereco Gateway
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 }; // endereco MAC



const int temperatura_ext = 4;
const int temperatura_int = 6;

const int pinPIR = 3;
const int buzzer = 5;
const int button = 10;

String clima = "";
String alertaGas = "";

int tentativas = 0;


unsigned long tempo_inicial;
unsigned long tempo_atual;
const unsigned long tempo_tela1 = 20000; // 20 segundos
const unsigned long tempo_tela2 = 20000;
const unsigned long tempo_tela3 = 20000;

int alrm = 0;
int count = 0;


unsigned int S_pwr[349]={4392, 4484, 524, 1628, 604, 484, 604, 1604, 576, 1604, 600, 488, 604, 488, 600, 1556, 624, 488, 604, 488, 576, 1580, 624, 488, 576, 512, 580, 1576, 628, 1556, 600, 512, 576, 1604, 604, 1576, 576, 516, 524, 1656, 524, 1656, 600, 1576, 604, 1580, 600, 1576, 604, 1576, 604, 488, 600, 1556, 624, 488, 600, 492, 600, 488, 600, 492, 600, 492, 572, 516, 600, 488, 604, 1576, 604, 488, 600, 488, 576, 516, 576, 512, 600, 492, 600, 488, 600, 1580, 604, 488, 600, 1580, 600, 1580, 572, 1608, 600, 1580, 572, 1608, 572, 1608, 572, 5240, 4492, 4360, 624, 1580, 600, 488, 604, 1576, 604, 1576, 600, 492, 576, 512, 524, 1632, 624, 488, 528, 564, 600, 1580, 600, 492, 524, 564, 576, 1604, 524, 1656, 524, 544, 596, 1608, 600, 1580, 596, 492, 600, 1556, 624, 1580, 524, 1656, 600, 1580, 524, 1656, 600, 1552, 552, 564, 600, 1580, 524, 568, 520, 568, 600, 468, 596, 516, 524, 568, 600, 488, 576, 516, 524, 1656, 596, 492, 572, 520, 600, 488, 576, 516, 572, 516, 600, 492, 600, 1580, 524, 564, 600, 1580, 600, 1580, 600, 1580, 600, 1580, 600, 1580, 600, 1580, 600, 55856, 144};
//ligar ar (sinal do ar da marca eletrolux configura a 24 graus)

int movimento = LOW;

OneWire oneWireExt(temperatura_ext); 
// instancia o objeto oneWire para comunicacao com o sensor de temperatura externa
OneWire oneWireInt(temperatura_int); 
// instancia o objeto oneWire para comunicacao com o sensor de temperatura interna


DallasTemperature sensorExt(&oneWireExt); 
// instancia o objeto sensor de temperatura externa
DallasTemperature sensorInt(&oneWireInt); 
// instancia o objeto sensor de temperatura interna

static word homePage(int chuva, int valorGas, double tempExt, double tempInt, int alrm, int count){ // pagina WEB
bfill = ether.tcpOffset();
bfill.emit_p(PSTR(
"HTTP/1.0 200OK\r\n"
"Content-Type: text/html\r\n"
"Pragma: no-cache\r\n"
"\r\n"
"<html>"
 "<head><title>"
   "Smart Home 2560"
  "</title></head>"
  "<body>"
    "<h1>Bem vindo ao Smart Home</h1>"
    "<h3>Temperatura Interna: <em style='color:red'>"tempInt"</em></h3>"
    "<h3>Temperatura Externa: <em style='color:red'>"tempExt"</em></h3>"
    "<h3>Previsao de Chuva: <em style='color:red'>"chuva"</em></h3>"
    "<h3>Alerta de Gas: <em style='color:red'>"valorGas"</em></h3>"
    "<h3>Alarme: <em style='color:red'>"alrm"</em></h3>"
    "<h3>Quantidade de Disparos: <em style='color:red'>"count"</em></h3>"
    "<p><a href=/almr=1>Clique aqui para ativar o alarme!</a></p>"

  "</body>"
"</html>"));
return bfill.position();
}

int rainSensor() { // sensor de chuva
  int valorSensor = analogRead(rain);
  int valorSaida = map(valorSensor, 0, 1023, 255, 0);
  return valorSaida;
}

void ligarAr(){ // funcao para ligar o ar
    Serial.println("Ligando...");
    irsend.sendRaw(S_pwr, 349, 38);
    Serial.println("Executando a 38 Hz");
    digitalWrite(13, HIGH);
    delay(100);
    digitalWrite(13, LOW);
    tentativas++;
    delay(3000);
}

void alarme_sonoro(){ // funcao para disparar o alarme
      count++;
      for(int i = 0; i < 20; i++){
        digitalWrite(buzzer, HIGH);
        analogWrite(buzzer, 200);
        delay(100);
        analogWrite(buzzer, 25);
        delay(100);
      }
      digitalWrite(buzzer,LOW);
}

void atualizaTela1(double tempExt, double tempInt){

  sensorExt.requestTemperatures();
  sensorInt.requestTemperatures();
  double  tempExt = sensorExt.getTempCByIndex(0);
  double tempInt = sensorInt.getTempCByIndex(0);

  tempExt = sensorExt.getTempCByIndex(0);
  tempInt = sensorInt.getTempCByIndex(0);

  lcd.setCursor(0,0);
  lcd.print("Temp.Int: ");
  lcd.setCursor(9,0);
  lcd.print(tempInt);
  lcd.print(" C");

  lcd.setCursor(0,1);
  lcd.print("Temp.Ext: ");
  lcd.setCursor(9,1);
  lcd.print(tempExt);  
  lcd.print(" C");

  if(tempInt > 32.0 && tentativas <= 3){ 
    // se a temperatura interna for maior que 32 graus e o sinal nao tiver sido enviado mais de 3 vezes
    ligarAr();
  }

}

void atualizaTela2(int chuva, int valorGas){
   chuva = rainSensor();
   valorGas = analogRead(gas);

  if(valorGas > 200 && valorGas < 300) {
    alertaGas = " Alerta     ";
  }else if(valorGas > 300){
    alertaGas = " Perigo!    ";
    alarme_sonoro();
  }else{
    alertaGas = " Normal     ";
  }

  if(chuva >= 100){
    clima = " Chuva ";  
  }else if(chuva > 50 && chuva < 100){
    clima = " Garoa ";
  }else{
    clima = " Seco  ";
  }

  lcd.setCursor(0,0);
  lcd.print("Previsao:");
  lcd.setCursor(9,0);
  lcd.print(clima);

  lcd.setCursor(0,1);
  lcd.print("Gas:");
  lcd.setCursor(4,1);
  lcd.print(alertaGas);

}

void atualizaTela3(){
   lcd.setCursor(0,0);
   lcd.print("Alarme:");
   lcd.setCursor(7,0);
   if(alrm){
    lcd.print(" Ativo   ");
   }else{
    lcd.print(" Inativo ");
   }
   lcd.setCursor(0,1);
   lcd.print("Qt.Disparos:");
   lcd.setCursor(12,1);
   lcd.print(count);

}

void ativaAlarme(int alarme){
  movimento = digitalRead(pinPIR);
  if(alarme == HIGH && alrm == 0){
    alrm = 1;
  }else if(alarme == HIGH && alrm == 1){
    alrm = 0;
  }
  if(alrm == 1){
    if(movimento == HIGH){
      alarme_sonoro();
    }
  }
}


void setup() {
  lcd.init();                       
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Smart Home :)");

  pinMode(buzzer,OUTPUT);
  pinMode(pinPIR, INPUT);
  pinMode(button, INPUT);
  pinMode(13, OUTPUT);

  delay(5000); // calibrando sensores
  Serial.begin(115200);

  sensorInt.begin();
  sensorExt.begin();

  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) { // inicializa o modulo ethernet com o endereco MAC 
    Serial.println(F("Falha ao comunicar com o Ethernet shield")); 
    lcd.println("Falha Rede");
  }else{
    Serial.println(F("Ethernet OK"));
    lcd.println("Rede OK");
    ether.staticSetup(myip, gwip); // configura o endereco IP e Gateway
  }



  tempo_inicial = millis(); // inicializa o tempo inicial

  #if RECEPTOR
  attachInterrupt(digitalPinToInterrupt(pinIR), IR_Interrupt_Handler, CHANGE);
  #endif

}

void loop() {
  #if RECEPTOR // codigo para recepcao de sinal IR do controle remoto em formato RAW exibindo o sinal no monitor serial
    lcd.setCursor(0,0);
    lcd.print("Recebendo Sinal");

    Serial.println(F("Press the button on the remote now - once only"));
    delay(5000); 
    lcd.clear();
    if (x) { 
      digitalWrite(LEDPIN, HIGH);
      Serial.println();
      Serial.print(F("Raw: (")); 
      lcd.setCursor(0,1);
      lcd.print("Sinal Recebido! ");
      Serial.print((x - 1));
      Serial.print(F(") "));
      detachInterrupt(digitalPinToInterrupt(pinIR));
      for (int i = 1; i < x; i++) { 
        if (!(i & 0x1)) Serial.print(F("-"));
         Serial.print(irBuffer[i] - irBuffer[i - 1]);
         Serial.print(F(", "));
      }
      x = 0;
      Serial.println();
      Serial.println();
      
      digitalWrite(LEDPIN, LOW);
      attachInterrupt(digitalPinToInterrupt(pinIR), IR_Interrupt_Handler, CHANGE);
    }


  #else
    // variaveis para armazenar os valores dos sensores
    int chuva = rainSensor();
    int valorGas = analogRead(gas);
    sensorExt.requestTemperatures();
    sensorInt.requestTemperatures();
    double  tempExt = sensorExt.getTempCByIndex(0);
    double tempInt = sensorInt.getTempCByIndex(0);

    // se houver comunicacao com o modulo ethernet retorne a pagina WEB
    word pos = ether.packetLoop(ether.packetReceive());
    if(pos){
      for(int c=pos; Ethernet::buffer[c]; c++)
          Serial.print((char)Ethernet::buffer[c]);
        Serial.println();
        word n;
        n = homePage(chuva, valorGas, tempExt, tempInt, alrm, count);
        ether.httpServerReply(n);
      }
    
    
    tempo_atual = millis(); // atualiza o tempo atual

    ativaAlarme(digitalRead(button)); // verifica se o botao foi pressionado

    unsigned long tempoDecorrido = (tempo_atual - tempo_inicial); // calcula o tempo decorrido

    // atualiza a tela de acordo com o tempo decorrido
    if(tempoDecorrido < tempo_tela1){
      atualizaTela1();
    }else if(tempoDecorrido < (tempo_tela1 + tempo_tela2)){
      atualizaTela2(chuva, valorGas);
    }else if(tempoDecorrido < (tempo_tela1 + tempo_tela2 + tempo_tela3)){
      atualizaTela3();
    }
    else{
      tempo_inicial = millis();
    }

  #endif
  
}

