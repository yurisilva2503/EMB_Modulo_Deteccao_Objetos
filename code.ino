volatile long sure = 0;
volatile float mesafe = 0;

bool circuitoAtivo = false;
bool botaoPressionadoAnterior = false;

unsigned long ultimoTrigger = 0;
const unsigned long intervaloLeitura = 500;

unsigned long tempoDebounce = 0;
const unsigned long delayDebounce = 200;

enum EstadoUltrassom { PRONTO, AGUARDANDO_ECHO_HIGH, AGUARDANDO_ECHO_LOW };
EstadoUltrassom estadoUltrassom = PRONTO;

unsigned long tempoEchoStart = 0;
unsigned long tempoEchoEnd = 0;

void setup() {
  // UART
  UBRR0H = 0;
  UBRR0L = 103;
  UCSR0B = (1 << TXEN0);
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

  // Pinos
  DDRB |= (1 << DDB0);     // Buzzer (pino 8)
  DDRD |= (1 << DDD7);     // Trig (pino 7)
  DDRD &= ~(1 << DDD6);    // Echo (pino 6)
  PORTD &= ~(1 << PORTD6);

  DDRD &= ~(1 << DDD2);    // Botão (pino 2)
  PORTD |= (1 << PORTD2);  // Pull-up botão

  DDRD |= (1 << DDD3);     // LED (pino 3)
  PORTD &= ~(1 << PORTD3); // LED off
}

void serialWrite(char data) {
  while (!(UCSR0A & (1 << UDRE0)));
  UDR0 = data;
}

void serialPrint(const char* str) {
  while (*str) serialWrite(*str++);
}

void serialPrintFloat(float value) {
  char buffer[10];
  dtostrf(value, 4, 1, buffer); // 4 = largura mínima, 1 = 1 casa decimal
  serialPrint(buffer);
}

void loop() {
  unsigned long agora = millis();
  unsigned long agoraMicro = micros();

  // Debounce botão
  bool botaoPressionado = !(PIND & (1 << PIND2));
  if (botaoPressionado && !botaoPressionadoAnterior && (agora - tempoDebounce > delayDebounce)) {
    tempoDebounce = agora;
    circuitoAtivo = !circuitoAtivo;

    if (circuitoAtivo) {
      PORTD |= (1 << PORTD3); // Liga LED
    } else {
      PORTD &= ~(1 << PORTD3); // Desliga LED
      PORTB &= ~(1 << PORTB0); // Desliga buzzer
    }
  }
  botaoPressionadoAnterior = botaoPressionado;

  if (!circuitoAtivo) return;

  switch (estadoUltrassom) {
    case PRONTO:
      if (millis() - ultimoTrigger >= intervaloLeitura) {
        // Envia trigger
        PORTD &= ~(1 << PORTD7);
        delayMicroseconds(2);
        PORTD |= (1 << PORTD7);
        delayMicroseconds(10);
        PORTD &= ~(1 << PORTD7);

        estadoUltrassom = AGUARDANDO_ECHO_HIGH;
      }
      break;

    case AGUARDANDO_ECHO_HIGH:
      if (PIND & (1 << PIND6)) {
        tempoEchoStart = micros();
        estadoUltrassom = AGUARDANDO_ECHO_LOW;
      }
      // opcional: timeout
      break;

    case AGUARDANDO_ECHO_LOW:
    if (!(PIND & (1 << PIND6))) {
        tempoEchoEnd = micros();
        sure = tempoEchoEnd - tempoEchoStart;
        mesafe = (sure / 2) / 29.0;

        serialPrint("D: ");
        serialPrintFloat(mesafe);
        serialPrint("cm\r\n");

        // Controle do buzzer com base na distância
        if (mesafe <= 50) {
          PORTB |= (1 << PORTB0);  // Liga buzzer continuamente
        } 
        else if (mesafe >= 50 && mesafe <= 100) {
            // Buzzer intermitente (liga/desliga a cada 500ms)
            if ((millis() / 500) % 2 == 0) {
                PORTB |= (1 << PORTB0);  // Liga buzzer
            } else {
                PORTB &= ~(1 << PORTB0);  // Desliga buzzer
            }
        } 
        else {
            PORTB &= ~(1 << PORTB0);  // Desliga buzzer
        }

        ultimoTrigger = millis();
        estadoUltrassom = PRONTO;
    }
    break;
  }
}
