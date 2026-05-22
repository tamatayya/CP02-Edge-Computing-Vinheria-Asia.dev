// ============================================================
//  MONITOR DE VINHERIA — versão integrada
//  LDR + DHT22 + RTC + EEPROM + LCD I2C
//  Menu: idioma (PT/EN) + unidade de temperatura (C/F)
//  Liga/desliga: pressão longa (2s) no btn_anterior
//  Desenvolvido por @sia.dev
// ============================================================

#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <Wire.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>
#include "DHT.h"

// ---------- CONFIGURAÇÕES GERAIS ----------
#define SERIAL_OPTION 1
#define LOG_OPTION    0
#define UTC_OFFSET    -3

// ---------- DHT ----------
#define DHTPIN  2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ---------- LCD ----------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------- RTC ----------
RTC_DS1307 RTC;

// ---------- EEPROM ----------
const int maxRecords = 100;
const int recordSize = 10;
int currentAddress   = 0;
const int endAddress = maxRecords * recordSize;

#define EEPROM_IDIOMA  1020
#define EEPROM_UNIDADE 1021
#define EEPROM_MAGIC   1022
#define MAGIC_VALUE    0xAB

// ---------- PINOS ----------
const int ldr           = A0;
const int led_vermelho  = 7;
const int led_amarelo   = 8;
const int led_verde     = 9;
const int buzzer        = 6;
const int btn_anterior  = 3;
const int btn_proximo   = 4;
const int btn_confirmar = 5;

// ---------- LIMITES ----------
const int LIM_SEGURO = 20;
const int LIM_ALERTA = 60;
float trigger_t_min  = 10.0;
float trigger_t_max  = 27.0;
float trigger_u_min  = 50.0;
float trigger_u_max  = 80.0;

// ---------- CONFIGURAÇÕES DO USUÁRIO ----------
uint8_t cfgIdioma  = 0;
uint8_t cfgUnidade = 0;

// ---------- LIGA/DESLIGA ----------
bool sistemaLigado = true;
#define TEMPO_LONG_PRESS 2000UL

// ---------- STRINGS EM PROGMEM ----------
const char pt_bemvindo[]    PROGMEM = "Bem-vindo!";
const char pt_iniciando[]   PROGMEM = "Iniciando...";
const char pt_cuidado[]     PROGMEM = "CUIDADO!";
const char pt_temperatura[] PROGMEM = "Temp";
const char pt_umidade[]     PROGMEM = "Umidade";
const char pt_luz[]         PROGMEM = "Luz";
const char pt_fora[]        PROGMEM = " fora!";
const char pt_celsius[]     PROGMEM = "Celsius";
const char pt_fahrenheit[]  PROGMEM = "Fahrenheit";
const char pt_idioma[]      PROGMEM = "Idioma:";
const char pt_unidade[]     PROGMEM = "Unidade temp:";
const char pt_salvo[]       PROGMEM = "Config. salva!";
const char pt_carregado[]   PROGMEM = "Config. carregada";
const char pt_padrao[]      PROGMEM = "Padrao: PT + C";
const char pt_desligando[]  PROGMEM = "Desligando...";
const char pt_ligando[]     PROGMEM = "Ligando...";

const char en_bemvindo[]    PROGMEM = "Welcome!";
const char en_iniciando[]   PROGMEM = "Starting...";
const char en_cuidado[]     PROGMEM = "WARNING!";
const char en_temperatura[] PROGMEM = "Temp";
const char en_umidade[]     PROGMEM = "Humidity";
const char en_luz[]         PROGMEM = "Light";
const char en_fora[]        PROGMEM = " out!";
const char en_celsius[]     PROGMEM = "Celsius";
const char en_fahrenheit[]  PROGMEM = "Fahrenheit";
const char en_idioma[]      PROGMEM = "Language:";
const char en_unidade[]     PROGMEM = "Temp unit:";
const char en_salvo[]       PROGMEM = "Config. saved!";
const char en_carregado[]   PROGMEM = "Config. loaded";
const char en_padrao[]      PROGMEM = "Default: PT + C";
const char en_desligando[]  PROGMEM = "Turning off...";
const char en_ligando[]     PROGMEM = "Turning on...";

const char* const str_bemvindo[]    PROGMEM = { pt_bemvindo,    en_bemvindo    };
const char* const str_iniciando[]   PROGMEM = { pt_iniciando,   en_iniciando   };
const char* const str_cuidado[]     PROGMEM = { pt_cuidado,     en_cuidado     };
const char* const str_temperatura[] PROGMEM = { pt_temperatura, en_temperatura };
const char* const str_umidade[]     PROGMEM = { pt_umidade,     en_umidade     };
const char* const str_luz[]         PROGMEM = { pt_luz,         en_luz         };
const char* const str_fora[]        PROGMEM = { pt_fora,        en_fora        };
const char* const str_celsius[]     PROGMEM = { pt_celsius,     en_celsius     };
const char* const str_fahrenheit[]  PROGMEM = { pt_fahrenheit,  en_fahrenheit  };
const char* const str_idioma[]      PROGMEM = { pt_idioma,      en_idioma      };
const char* const str_unidade[]     PROGMEM = { pt_unidade,     en_unidade     };
const char* const str_salvo[]       PROGMEM = { pt_salvo,       en_salvo       };
const char* const str_carregado[]   PROGMEM = { pt_carregado,   en_carregado   };
const char* const str_padrao[]      PROGMEM = { pt_padrao,      en_padrao      };
const char* const str_desligando[]  PROGMEM = { pt_desligando,  en_desligando  };
const char* const str_ligando[]     PROGMEM = { pt_ligando,     en_ligando     };

const char nome_pt[] PROGMEM = "Portugues";
const char nome_en[] PROGMEM = "English";
const char* const nomes_idioma[] PROGMEM = { nome_pt, nome_en };

char strBuf[17];
const char* getStr(const char* const* table) {
  strcpy_P(strBuf, (char*)pgm_read_word(&table[cfgIdioma]));
  return strBuf;
}

// ---------- ESTADO DO SISTEMA ----------
int estadoGeral          = -1;
int lastLoggedMinute     = -1;
int luminosidadeAnterior = -1;
int lumAnteriorLCD       = -1;

bool alertaTemperatura   = false;
bool alertaUmidade       = false;
float ultimaTemp         = 0;
float ultimaUmid         = 0;

unsigned long tempoPiscar    = 0;
bool estadoPiscar            = false;
unsigned long tempoAlternar  = 0;
bool mostrandoTemp           = true;
unsigned long tempoFimAlerta = 0;
bool aguardandoRepeticao     = false;

// ---------- CARACTERES CUSTOMIZADOS ----------
byte sorriso[] = { B00000, B00000, B00000, B11011, B00000, B10001, B01110, B00000 };
byte vazio[]   = { B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000 };
byte lampada[] = { B01110, B10001, B10101, B10101, B01110, B01110, B00100, B00000 };
byte grau[]    = { B01110, B01010, B01110, B00000, B00000, B00000, B00000, B00000 };
byte gota[]    = { B00100, B00100, B01010, B01010, B10001, B10001, B10001, B01110 };

// ============================================================
//  HELPERS
// ============================================================

bool btnPress(int pino) {
  if (digitalRead(pino) == LOW) {
    delay(50);
    if (digitalRead(pino) == LOW) {
      while (digitalRead(pino) == LOW);
      return true;
    }
  }
  return false;
}

// Detecta pressão longa no btn_anterior — não bloqueia o loop
// Retorna true uma única vez quando pressão >= TEMPO_LONG_PRESS
bool btnLongPress() {
  static unsigned long tempoInicio = 0;
  static bool esperando = false;

  if (digitalRead(btn_anterior) == LOW) {
    if (!esperando) {
      esperando    = true;
      tempoInicio  = millis();
    } else if (millis() - tempoInicio >= TEMPO_LONG_PRESS) {
      esperando = false;
      while (digitalRead(btn_anterior) == LOW); // aguarda soltar
      return true;
    }
  } else {
    esperando = false;
  }
  return false;
}

float toDisplay(float celsius) {
  return cfgUnidade == 1 ? (celsius * 9.0 / 5.0) + 32.0 : celsius;
}

String unidadeStr() {
  return cfgUnidade == 1 ? "F" : "C";
}

void todosLedsOff() {
  digitalWrite(led_verde,    LOW);
  digitalWrite(led_amarelo,  LOW);
  digitalWrite(led_vermelho, LOW);
}

// ============================================================
//  LIGA / DESLIGA
// ============================================================
void desligarSistema() {
  sistemaLigado = false;
  estadoGeral   = -1;
  todosLedsOff();
  noTone(buzzer);
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(getStr(str_desligando));
  delay(1000);
  lcd.clear();
  lcd.noBacklight();
  if (SERIAL_OPTION) Serial.println("[SISTEMA] Desligado.");
}

void ligarSistema() {
  sistemaLigado        = true;
  estadoGeral          = -1;
  lastLoggedMinute     = -1;
  luminosidadeAnterior = -1;
  lumAnteriorLCD       = -1;
  alertaTemperatura    = false;
  alertaUmidade        = false;
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(getStr(str_ligando));
  delay(1000);
  lcd.clear();
  if (SERIAL_OPTION) Serial.println("[SISTEMA] Ligado.");
}

// ============================================================
//  MENU DE CONFIGURAÇÃO
// ============================================================
void menuConfigurar() {
  const unsigned long TIMEOUT = 10000;

  bool jaConfigurado = (EEPROM.read(EEPROM_MAGIC) == MAGIC_VALUE);
  if (jaConfigurado) {
    cfgIdioma  = EEPROM.read(EEPROM_IDIOMA);
    cfgUnidade = EEPROM.read(EEPROM_UNIDADE);

    lcd.clear();
    lcd.setCursor(0, 0); lcd.print(getStr(str_carregado));
    lcd.setCursor(0, 1); lcd.print("OK=manter  >=cfg");

    unsigned long t = millis();
    bool reconfigurar = false;
    while (millis() - t < TIMEOUT) {
      if (btnPress(btn_confirmar)) { reconfigurar = false; break; }
      if (btnPress(btn_proximo))   { reconfigurar = true;  break; }
    }

    if (!reconfigurar) {
      lcd.clear();
      return;
    }
  }

  // ── PASSO 1: idioma ───────────────────────────────────────
  uint8_t idiomaTemp  = cfgIdioma;
  uint8_t unidadeTemp = cfgUnidade;
  unsigned long t = millis();

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Idioma / Language");

  while (true) {
    if (millis() - t >= TIMEOUT) {
      if (!jaConfigurado) usarPadrao();
      return;
    }
    lcd.setCursor(0, 1);
    char buf[17];
    strcpy_P(buf, (char*)pgm_read_word(&nomes_idioma[idiomaTemp]));
    lcd.print("> "); lcd.print(buf); lcd.print("         ");

    if (btnPress(btn_proximo))   { idiomaTemp = (idiomaTemp == 0) ? 1 : 0; t = millis(); }
    if (btnPress(btn_confirmar)) { cfgIdioma = idiomaTemp; break; }
  }

  // ── PASSO 2: unidade ──────────────────────────────────────
  t = millis();
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(getStr(str_unidade));

  while (true) {
    if (millis() - t >= TIMEOUT) {
      if (!jaConfigurado) usarPadrao();
      return;
    }
    lcd.setCursor(0, 1);
    lcd.print("> ");
    lcd.print(unidadeTemp == 0 ? getStr(str_celsius) : getStr(str_fahrenheit));
    lcd.print("          ");

    if (btnPress(btn_proximo))   { unidadeTemp = unidadeTemp == 0 ? 1 : 0; t = millis(); }
    if (btnPress(btn_confirmar)) { cfgUnidade = unidadeTemp; break; }
  }

  // ── SALVA NA EEPROM ───────────────────────────────────────
  EEPROM.write(EEPROM_IDIOMA,  cfgIdioma);
  EEPROM.write(EEPROM_UNIDADE, cfgUnidade);
  EEPROM.write(EEPROM_MAGIC,   MAGIC_VALUE);

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(getStr(str_salvo));
  delay(1500);
  lcd.clear();
}

void usarPadrao() {
  cfgIdioma  = 0;
  cfgUnidade = 0;
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(pt_padrao);
  delay(1500);
  lcd.clear();
}

// ============================================================
void setup() {
  pinMode(led_vermelho,  OUTPUT);
  pinMode(led_amarelo,   OUTPUT);
  pinMode(led_verde,     OUTPUT);
  pinMode(buzzer,        OUTPUT);
  pinMode(btn_anterior,  INPUT_PULLUP);
  pinMode(btn_proximo,   INPUT_PULLUP);
  pinMode(btn_confirmar, INPUT_PULLUP);

  Serial.begin(9600);
  dht.begin();

  lcd.init();
  lcd.backlight();
  lcd.createChar(0, sorriso);
  lcd.createChar(1, vazio);
  lcd.createChar(2, lampada);
  lcd.createChar(3, grau);
  lcd.createChar(4, gota);

  RTC.begin();
  RTC.adjust(DateTime(2026, 5, 22, 11, 48, 20)); 

  if (LOG_OPTION) get_log();
  EEPROM.begin();

  // ── CHARS DA ANIMAÇÃO DE ONDA ─────────────────────────────
  byte waveChar0[] = { 0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F };
  byte waveChar1[] = { 0x00,0x00,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F };
  byte waveChar2[] = { 0x00,0x00,0x00,0x00,0x1F,0x1F,0x1F,0x1F };
  byte waveChar3[] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x1F,0x1F };
  byte waveChar4[] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
  lcd.createChar(0, waveChar0);
  lcd.createChar(1, waveChar1);
  lcd.createChar(2, waveChar2);
  lcd.createChar(3, waveChar3);
  lcd.createChar(4, waveChar4);
  lcd.createChar(5, sorriso);

  lcd.setCursor(3, 0); lcd.print("Asia.dev");
  lcd.setCursor(15, 0); lcd.write(5);
  delay(2000);

  for (int pos = -7; pos <= 21; pos++) {
    for (int col = 0; col < 16; col++) {
      int dist = col - pos;
      if      (dist == 0)               { lcd.setCursor(col,0); lcd.write(0); lcd.setCursor(col,1); lcd.write(0); }
      else if (dist == 1 || dist == -1) { lcd.setCursor(col,0); lcd.write(1); lcd.setCursor(col,1); lcd.write(0); }
      else if (dist == 2 || dist == -2) { lcd.setCursor(col,0); lcd.write(2); lcd.setCursor(col,1); lcd.write(0); }
      else if (dist == 3 || dist == -3) { lcd.setCursor(col,0); lcd.write(3); lcd.setCursor(col,1); lcd.write(0); }
      else if (dist == 4 || dist == -4) { lcd.setCursor(col,0); lcd.write(4); lcd.setCursor(col,1); lcd.write(0); }
      else if (dist == 5 || dist == -5) {                                      lcd.setCursor(col,1); lcd.write(1); }
      else if (dist == 6 || dist == -6) {                                      lcd.setCursor(col,1); lcd.write(2); }
      else if (dist == 7 || dist == -7) {                                      lcd.setCursor(col,1); lcd.write(3); }
      else { lcd.setCursor(col,0); lcd.write(4); lcd.setCursor(col,1); lcd.write(4); }
    }
    delay(20);
  }
  lcd.clear();

  lcd.createChar(0, sorriso);
  lcd.createChar(1, vazio);
  lcd.createChar(2, lampada);
  lcd.createChar(3, grau);
  lcd.createChar(4, gota);

  menuConfigurar();

  lcd.setCursor(0, 0); lcd.print(getStr(str_bemvindo));
  lcd.setCursor(0, 1); lcd.print(getStr(str_iniciando));
  delay(1500);
  lcd.clear();

  todosLedsOff();
  digitalWrite(led_verde, HIGH);
}

// ============================================================
void loop() {

  // ── VERIFICA LONG PRESS PARA LIGA/DESLIGA ─────────────────
  if (btnLongPress()) {
    if (sistemaLigado) desligarSistema();
    else               ligarSistema();
  }

  // ── SE DESLIGADO, NÃO FAZ MAIS NADA ──────────────────────
  if (!sistemaLigado) return;

  DateTime now      = RTC.now();
  DateTime ajustado = DateTime(now.unixtime() + UTC_OFFSET * 3600L);

  int ldrStatus    = analogRead(ldr);
  int luminosidade = map(ldrStatus, 0, 1013, 0, 100);

  // ATIVA/DESATIVA O Loop do Serial Print, para debug dos valores ldr da funcao map
  // if (SERIAL_OPTION && luminosidade != luminosidadeAnterior) {
  //   Serial.print("LDR: "); Serial.print(ldrStatus);
  //   Serial.print(" | Lum: "); Serial.print(luminosidade);
  //   Serial.println("%");
  // }
  luminosidadeAnterior = luminosidade;

  // ── ATUALIZAÇÃO A CADA MINUTO ─────────────────────────────
  if (ajustado.minute() != lastLoggedMinute) {
    lastLoggedMinute = ajustado.minute();

    float temperatura = dht.readTemperature();
    float umidade     = dht.readHumidity();

    alertaTemperatura = temperatura < trigger_t_min || temperatura > trigger_t_max;
    alertaUmidade     = umidade     < trigger_u_min || umidade     > trigger_u_max;
    ultimaTemp        = temperatura;
    ultimaUmid        = umidade;

    if (alertaTemperatura || alertaUmidade) salvarEEPROM(ajustado, ldrStatus);

    if (SERIAL_OPTION) {
      Serial.print("Temp: "); Serial.print(toDisplay(temperatura));
      Serial.print(unidadeStr()); Serial.print(" | Umid: ");
      Serial.print(umidade); Serial.print("% | ");
      Serial.print(ajustado.day());    Serial.print("/");
      Serial.print(ajustado.month()); Serial.print("/");
      Serial.print(ajustado.year());  Serial.print(" ");
      Serial.print(ajustado.hour());  Serial.print(":");
      Serial.print(ajustado.minute()); Serial.println();
    }

    if ((estadoGeral == 0 || estadoGeral == -1) && !alertaTemperatura && !alertaUmidade) {
      atualizarLCD(ajustado, temperatura, umidade);
    }
  }

  // ── DETERMINA ESTADO GERAL ────────────────────────────────
  int novoEstado;
  if      (luminosidade >= LIM_ALERTA)         novoEstado = 2;
  else if (luminosidade >= LIM_SEGURO)         novoEstado = 1;
  else if (alertaTemperatura || alertaUmidade) novoEstado = 3;
  else                                         novoEstado = 0;

  // ── TRANSIÇÃO DE ESTADO ───────────────────────────────────
  if (novoEstado != estadoGeral) {
    estadoGeral = novoEstado;
    todosLedsOff();
    noTone(buzzer);
    aguardandoRepeticao = false;
    lumAnteriorLCD = -1;
    lcd.clear();

    if (estadoGeral == 0) {
      digitalWrite(led_verde, HIGH);
      DateTime aj2 = DateTime(RTC.now().unixtime() + UTC_OFFSET * 3600L);
      atualizarLCD(aj2, dht.readTemperature(), dht.readHumidity());

    } else if (estadoGeral == 1) {
      salvarEEPROM(ajustado, ldrStatus);
      exibirAlertaLCD(getStr(str_luz), String(luminosidade) + "%" + getStr(str_fora));
      for (int i = 0; i < 5; i++) {
        digitalWrite(led_amarelo, HIGH); tone(buzzer, 1000, 300); delay(300);
        digitalWrite(led_amarelo, LOW);  tone(buzzer, 600,  300); delay(300);
      }
      digitalWrite(led_amarelo, HIGH);
      tempoFimAlerta      = millis();
      aguardandoRepeticao = true;

    } else if (estadoGeral == 2) {
      salvarEEPROM(ajustado, ldrStatus);
      exibirAlertaLCD(getStr(str_luz), String(luminosidade) + "% PERIGO!");
      digitalWrite(led_vermelho, HIGH);

    } else if (estadoGeral == 3) {
      mostrandoTemp = true;
      tempoAlternar = millis();
      if (alertaTemperatura) {
        exibirAlertaLCD(getStr(str_temperatura),
          String(toDisplay(ultimaTemp), 1) + unidadeStr() + getStr(str_fora));
      } else {
        exibirAlertaLCD(getStr(str_umidade),
          String((int)ultimaUmid) + "%" + getStr(str_fora));
      }
      digitalWrite(led_amarelo, HIGH);
    }
  }

  // ── AÇÕES CONTÍNUAS ───────────────────────────────────────
  if (estadoGeral == 0) atualizarLuzLCD(luminosidade);

  if (estadoGeral == 1) {
    if (aguardandoRepeticao && millis() - tempoFimAlerta >= 3000) {
      aguardandoRepeticao = false;
      for (int i = 0; i < 5; i++) {
        digitalWrite(led_amarelo, HIGH); tone(buzzer, 1000, 300); delay(300);
        digitalWrite(led_amarelo, LOW);  tone(buzzer, 600,  300); delay(300);
      }
      digitalWrite(led_amarelo, HIGH);
      tempoFimAlerta      = millis();
      aguardandoRepeticao = true;
    }
  }

  if (estadoGeral == 2) {
    if (millis() - tempoPiscar >= 100) {
      tempoPiscar  = millis();
      estadoPiscar = !estadoPiscar;
      digitalWrite(led_vermelho, estadoPiscar ? HIGH : LOW);
      if (estadoPiscar) tone(buzzer, 3000, 80);
    }
  }

  if (estadoGeral == 3) {
    if (millis() - tempoPiscar >= 300) {
      tempoPiscar  = millis();
      estadoPiscar = !estadoPiscar;
      digitalWrite(led_amarelo, estadoPiscar ? HIGH : LOW);
      if (estadoPiscar) tone(buzzer, 1000, 150);
    }
    if (alertaTemperatura && alertaUmidade && millis() - tempoAlternar >= 3000) {
      tempoAlternar = millis();
      mostrandoTemp = !mostrandoTemp;
      if (mostrandoTemp) {
        exibirAlertaLCD(getStr(str_temperatura),
          String(toDisplay(ultimaTemp), 1) + unidadeStr() + getStr(str_fora));
      } else {
        exibirAlertaLCD(getStr(str_umidade),
          String((int)ultimaUmid) + "%" + getStr(str_fora));
      }
    }
  }
}

// ============================================================
//  FUNÇÕES
// ============================================================

void exibirAlertaLCD(String condicao, String valor) {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(getStr(str_cuidado));
  lcd.setCursor(0, 1);
  String linha = condicao + ": " + valor;
  if (linha.length() > 16) linha = linha.substring(0, 16);
  lcd.print(linha);
}

void atualizarLCD(DateTime dt, float temp, float umid) {
  lcd.setCursor(0, 0);
  lcd.print(dt.day()    < 10 ? "0" : ""); lcd.print(dt.day());    lcd.print("/");
  lcd.print(dt.month()  < 10 ? "0" : ""); lcd.print(dt.month());  lcd.print(" ");
  lcd.print(dt.hour()   < 10 ? "0" : ""); lcd.print(dt.hour());   lcd.print(":");
  lcd.print(dt.minute() < 10 ? "0" : ""); lcd.print(dt.minute());
  lcd.setCursor(0, 1);
  lcd.print(toDisplay(temp), 1); lcd.write(byte(3)); lcd.print(unidadeStr()); lcd.print(" ");
  lcd.write(byte(4)); lcd.print((int)umid); lcd.print("% ");
  lcd.setCursor(11, 1); lcd.write(byte(2));
}

void atualizarLuzLCD(int lum) {
  if (lum == lumAnteriorLCD) return;
  lumAnteriorLCD = lum;
  lcd.setCursor(12, 1);
  lcd.print(lum);
  lcd.print("%  ");
}

void salvarEEPROM(DateTime timestamp, int ldrVal) {
  float temperatura = dht.readTemperature();
  float umidade     = dht.readHumidity();
  int tempInt = (int)(temperatura * 100);
  int humiInt = (int)(umidade * 100);
  EEPROM.put(currentAddress,     (long)timestamp.unixtime());
  EEPROM.put(currentAddress + 4, tempInt);
  EEPROM.put(currentAddress + 6, humiInt);
  EEPROM.put(currentAddress + 8, ldrVal);
  getNextAddress();
  if (SERIAL_OPTION) {
    Serial.print("[EEPROM] Salvo — ");
    Serial.print(timestamp.day()    < 10 ? "0" : ""); Serial.print(timestamp.day());    Serial.print("/");
    Serial.print(timestamp.month()  < 10 ? "0" : ""); Serial.print(timestamp.month());  Serial.print("/");
    Serial.print(timestamp.year());                                                       Serial.print(" ");
    Serial.print(timestamp.hour()   < 10 ? "0" : ""); Serial.print(timestamp.hour());   Serial.print(":");
    Serial.print(timestamp.minute() < 10 ? "0" : ""); Serial.print(timestamp.minute()); Serial.print(" | ");
    Serial.print("Temp: "); Serial.print(toDisplay(temperatura)); Serial.print(unidadeStr()); Serial.print(" | ");
    Serial.print("Umid: "); Serial.print(umidade); Serial.print("% | ");
    Serial.print("Lum: ");  Serial.print(map(ldrVal, 0, 1013, 0, 100)); Serial.println("%");
  }
}

void getNextAddress() {
  currentAddress += recordSize;
  if (currentAddress >= endAddress) currentAddress = 0;
}

void get_log() {
  Serial.println("======================================");
  Serial.println("        LOG DA EEPROM — VINHERIA      ");
  Serial.println("======================================");
  Serial.println("Timestamp            Temp    Umid    Lum");
  Serial.println("--------------------------------------");
  bool semDados = true;
  for (int address = 0; address < endAddress; address += recordSize) {
    long timeStamp;
    int  tempInt, humiInt, ldrInt;
    EEPROM.get(address,     timeStamp);
    EEPROM.get(address + 4, tempInt);
    EEPROM.get(address + 6, humiInt);
    EEPROM.get(address + 8, ldrInt);
    if (timeStamp == (long)0xFFFFFFFF || timeStamp == 0) continue;
    semDados = false;
    float temperatura  = tempInt / 100.0;
    float umidade      = humiInt / 100.0;
    int   luminosidade = map(ldrInt, 0, 1013, 0, 100);
    DateTime dt = DateTime(timeStamp);
    Serial.print(dt.day()    < 10 ? "0" : ""); Serial.print(dt.day());    Serial.print("/");
    Serial.print(dt.month()  < 10 ? "0" : ""); Serial.print(dt.month());  Serial.print("/");
    Serial.print(dt.year());                                                Serial.print(" ");
    Serial.print(dt.hour()   < 10 ? "0" : ""); Serial.print(dt.hour());   Serial.print(":");
    Serial.print(dt.minute() < 10 ? "0" : ""); Serial.print(dt.minute()); Serial.print("  ");
    Serial.print(toDisplay(temperatura)); Serial.print(unidadeStr()); Serial.print("   ");
    Serial.print(umidade);      Serial.print("%   ");
    Serial.print(luminosidade); Serial.println("%");
  }
  if (semDados) Serial.println("Nenhum registro encontrado.");
  Serial.println("======================================");
}