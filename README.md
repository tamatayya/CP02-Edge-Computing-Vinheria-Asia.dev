# Monitor de Vinheria
**Desenvolvido por Asia.dev**

---

## Objetivo

O Monitor de Vinheria é um sistema embarcado desenvolvido com Arduino Uno para monitorar as condições ambientais de uma vinheria em tempo real. Vinhos são sensíveis a variações de temperatura, umidade e luminosidade — condições fora dos limites ideais podem comprometer a qualidade e a conservação dos produtos.

O sistema monitora continuamente esses três fatores e alerta o usuário visualmente (LEDs e LCD) e sonoramente (buzzer) sempre que alguma condição sair do intervalo seguro. Todos os eventos fora dos limites são registrados na EEPROM com timestamp para consulta posterior.

---

## Hardware utilizado

| Componente | Função |
|---|---|
| Arduino Uno | Microcontrolador principal |
| DHT11 | Sensor de temperatura e umidade |
| LDR (fotoresistor) | Sensor de luminosidade |
| DS1307 (RTC) | Relógio em tempo real |
| LCD 1602 I2C | Display de informações |
| LED verde | Indicador de estado seguro |
| LED amarelo | Indicador de estado de alerta |
| LED vermelho | Indicador de estado de perigo |
| Buzzer passivo | Alarme sonoro |
| 3x Push button | Navegação no menu |
| Resistor 220Ω (x3) | Limitador de corrente dos LEDs |
| Resistor 10kΩ | Divisor de tensão do LDR |

---

## Pinagem

| Pino Arduino | Componente |
|---|---|
| D2 | DHT11 (dados) |
| D3 | Botão Anterior |
| D4 | Botão Próximo |
| D5 | Botão Confirmar |
| D6 | Buzzer |
| D7 | LED vermelho |
| D8 | LED amarelo |
| D9 | LED verde |
| A0 | LDR (saída analógica) |
| A4 | SDA — LCD e RTC (I2C) |
| A5 | SCL — LCD e RTC (I2C) |

---

## Funcionalidades

### Monitoramento de luminosidade (LDR)
O sensor LDR mede continuamente a intensidade de luz no ambiente. O valor bruto é mapeado para uma escala de 0 a 100% e exibido no LCD em tempo real.

O sistema opera em três faixas:

| Faixa | Luminosidade | Estado |
|---|---|---|
| Seguro | abaixo de 20% | Ambiente escuro, ideal para conservação |
| Alerta | entre 20% e 60% | Luminosidade moderada, atenção necessária |
| Perigo | acima de 60% | Luminosidade alta, risco à conservação |

### Monitoramento de temperatura e umidade (DHT11)
A cada minuto o sistema lê temperatura e umidade. Se algum valor estiver fora dos limites definidos, um alerta é ativado e o evento é salvo na EEPROM.

| Parâmetro | Mínimo | Máximo |
|---|---|---|
| Temperatura | 10°C | 27°C |
| Umidade | 50% | 80% |

### Registro em EEPROM
Todos os eventos fora dos limites (luminosidade em alerta/perigo, temperatura ou umidade irregular) são gravados na EEPROM com timestamp, temperatura, umidade e valor do LDR. O sistema suporta até 100 registros em modo circular — quando cheio, sobrescreve os mais antigos.

### Relógio em tempo real (RTC DS1307)
O RTC mantém a data e hora do sistema, aplicando o fuso horário UTC-3. A data e hora são exibidas no LCD e usadas para marcar os registros salvos na EEPROM. Para funcionamento contínuo sem perda de hora em caso de queda de energia, recomenda-se o uso de uma bateria CR2032 no módulo RTC.

### Menu de configuração
Ao iniciar, o sistema exibe um menu para configurar idioma e unidade de temperatura. As configurações são salvas na EEPROM e carregadas automaticamente nas próximas inicializações.

**Idiomas disponíveis:** Português, English

**Unidades de temperatura:** Celsius, Fahrenheit

Se nenhum botão for pressionado em 10 segundos, o sistema usa o padrão (Português + Celsius).

---

## Funções dos componentes

### LEDs

**LED verde (D9)**
Acende quando o ambiente está em estado seguro — luminosidade abaixo de 20% e temperatura e umidade dentro dos limites. Indica que as condições de armazenamento estão ideais.

**LED amarelo (D8)**
Acende em dois cenários de alerta:
- Luminosidade entre 20% e 60%: pisca 5 vezes com buzzer ao entrar no estado, depois repete a cada 3 segundos enquanto a condição persistir.
- Temperatura ou umidade fora dos limites: pisca continuamente a cada 300ms com bipes curtos.

**LED vermelho (D7)**
Acende quando a luminosidade ultrapassa 60% (perigo). Pisca rapidamente a cada 100ms com bipes agudos contínuos enquanto a condição persistir.

### LCD 1602 I2C

No estado normal (seguro), o display exibe:
- **Linha 1:** data e hora atual (`DD/MM HH:MM`)
- **Linha 2:** temperatura, símbolo de grau, unidade, ícone de gota, umidade e ícone de lâmpada com percentual de luminosidade

Nos estados de alerta, o display exibe `CUIDADO!` na linha 1 e a condição que causou o alerta na linha 2. Se temperatura e umidade estiverem simultaneamente fora dos limites, o display alterna entre os dois alertas a cada 3 segundos.

### Buzzer

Emite sons distintos conforme o estado:
- **Alerta de luz (amarelo):** dois tons alternados (1000Hz / 600Hz) em 5 pulsos
- **Perigo de luz (vermelho):** bipes agudos rápidos (3000Hz), a cada 100ms
- **Alerta de temperatura/umidade:** bipes curtos (1000Hz), a cada 300ms

### Botões

| Botão | Pino | Função no menu | Função geral |
|---|---|---|---|
| Anterior | D3 | — | Pressão longa (2s): liga/desliga o sistema |
| Próximo | D4 | Alterna entre opções | — |
| Confirmar | D5 | Confirma a seleção | — |

**Liga/desliga por pressão longa:**
Segurar o botão Anterior por 2 segundos desliga o sistema — o LCD apaga o backlight e todos os LEDs e o buzzer são desativados. Segurar novamente por 2 segundos religar o sistema normalmente.

---

## Estados do sistema

```
Luminosidade < 20%  →  Estado 0: SEGURO     → LED verde
Luminosidade 20–60% →  Estado 1: ALERTA LUZ → LED amarelo + buzzer intermitente
Luminosidade > 60%  →  Estado 2: PERIGO LUZ → LED vermelho + buzzer contínuo
Temp/Umid fora      →  Estado 3: ALERTA T/U → LED amarelo piscando + buzzer
```

A prioridade de estado segue a ordem: perigo de luz > alerta de luz > alerta de temperatura/umidade > seguro.

---

## Animação de inicialização

Ao ligar, o sistema exibe uma animação de onda no LCD varrendo as duas linhas do display, seguida da tela de boas-vindas com o nome da equipe (`Asia.dev`) e então o menu de configuração.

---

## Configurações no código

As constantes abaixo podem ser ajustadas diretamente no código:

```cpp
#define SERIAL_OPTION 1    // 1 = Serial Monitor ativo, 0 = desativado
#define LOG_OPTION    0    // 1 = imprime EEPROM no Serial ao iniciar
#define UTC_OFFSET    -3   // fuso horário (UTC-3 para horário de Brasília)

const int LIM_SEGURO = 20; // luminosidade máxima para estado seguro (%)
const int LIM_ALERTA = 60; // luminosidade máxima para estado de alerta (%)

float trigger_t_min = 10.0; // temperatura mínima (°C)
float trigger_t_max = 27.0; // temperatura máxima (°C)
float trigger_u_min = 50.0; // umidade mínima (%)
float trigger_u_max = 80.0; // umidade máxima (%)
```

---

## Bibliotecas necessárias

Instalar via **Sketch → Include Library → Manage Libraries** na Arduino IDE:

- `LiquidCrystal_I2C` — Frank de Brabander
- `RTClib` — Adafruit
- `DHT sensor library` — Adafruit (instalar com todas as dependências)

---

*Monitor de Vinheria — Asia.dev*
