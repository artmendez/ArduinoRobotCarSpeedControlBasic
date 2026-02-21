/*
  Carro Robot con Arduino UNO + L298N (2 motores DC)
  ---------------------------------------------------
  Este código proporciona:
  - Control de velocidad por PWM (0–100%)
  - Control de dirección (avanzar, retroceder, girar)
  - Alto con freno eléctrico o rueda libre
  - Ejemplo de trayectoria repetida con ciclo 'for'

Si vas a utilizar este código como se muestra, es necesario asegurar las conexiones como sigue:
  Cableado usado en este código:
    ENA -> D5 (PWM)
    IN1 -> D8
    IN2 -> D9
    IN3 -> D10
    IN4 -> D11
    ENB -> D6 (PWM)

  IMPORTANTE:
  - Retira los jumpers de ENA y ENB en el L298N para usar PWM.
  - Conecta la batería de motores al +12V del L298N (o según tus motores).
  - Une el GND del L298N con el GND del Arduino (tierra común).
  - Opcional: usa el regulador de 5V del L298N SOLO si está instalado el jumper.

  Autor: A. Méndez

  IMPORTANTE: Puede utilizarse este código como base. Solamente es necesario reemplazar el código en:
      void setup(){
        No modificar los comandos existentes de configuración de pines.
        AÑADIR LO QUE SE DESEA QUE SE EJECUTE UNA SOLA VEZ AL ENCENDER EL ARDUINO.
      }

      y en

      void loop(){
        ...
        AÑADIR LO QUE SE DESEA QUE SE EJECUTE DE MANERA REPETIDA. SE EJECUTA DESPUÉS DE SETUP()
      }
*/

// CONEXIONES ARDUINO AL L298N usados en este código.
// Opcionalmente MODIFICAR LAS ASIGNACIONES DE PINES de acuerdo a tu robot.
// O bien MODIFICAR LAS CONEXIONES FÍSICAS entre tu Arduino y el L298N si se desea usar estas.
// Asegurarse que ENA y ENB estén conectados a PINES PWM, señalados con un "~" en el Arduino UNO.
const uint8_t ENA = 5;   // Enable control de velocidad Motor Izquierdo (PWM)
const uint8_t IN1 = 8;   // Dirección Motor Izquierdo
const uint8_t IN2 = 9;   // Dirección Motor Izquierdo

const uint8_t IN3 = 10;  // Dirección Motor Derecho
const uint8_t IN4 = 11;  // Dirección Motor Derecho
const uint8_t ENB = 6;   // Enable control de velocidad Motor Derecho (PWM)

// AJUSTES DE OPERACIÓN:
// Si un motor gira al revés, cambia a true para invertir su sentido.
bool INVERTIR_MOTOR_IZQ = false;
bool INVERTIR_MOTOR_DER = false;

// Si 'true', el alto usa freno eléctrico (IN1=IN2=HIGH). Si 'false', usa rueda libre (IN1=IN2=LOW).
bool FRENO_ACTIVO = true;

// rUTINAS UTILITARIAS: NO MODIFICARLAS.
/*
Las siguientes líneas ajustan y facilitan el uso de parámetros para los motores:
  Convierte un porcentaje (0–100) a valor PWM (0–255).
  - Si es negativo, conserva el signo y aplica el mapeo al valor absoluto.
  - Se limita a -100..100 para proteger el driver.
*/
int porcentajeA_pwm(int velPct) {
  if (velPct > 100) velPct = 100;
  if (velPct < -100) velPct = -100;

  int signo = (velPct < 0) ? -1 : 1;
  int absPct = abs(velPct);
  int pwm = map(absPct, 0, 100, 0, 255); // 0%->0, 100%->255 para pensar en porcentaje de velocidad del motor.
  return signo * pwm;
}

/*La rutina aplicarMotor:
  Aplica velocidad y sentido a un motor dado sus pines.
  - velPctSigned: -100..100 (negativo = reversa)
  - invertido: invierte el sentido (útil si el cableado invierte giro)

  NO MODIFICARLA
*/
void aplicarMotor(uint8_t pinIN1, uint8_t pinIN2, uint8_t pinEN, bool invertido, int velPctSigned) {
  int pwmSigned = porcentajeA_pwm(velPctSigned);
  int pwm = abs(pwmSigned);

  if (pwm == 0) {
    // ALTO: freno o rueda libre
    if (FRENO_ACTIVO) {
      digitalWrite(pinIN1, HIGH);
      digitalWrite(pinIN2, HIGH);
    } else {
      digitalWrite(pinIN1, LOW);
      digitalWrite(pinIN2, LOW);
    }
    analogWrite(pinEN, 0);
    return;
  }

  // Determinar sentido. NO MODIFICARLA.
  bool adelante = (pwmSigned > 0); // true=adelante, false=atrás
  if (invertido) adelante = !adelante;

  if (adelante) {
    digitalWrite(pinIN1, HIGH);
    digitalWrite(pinIN2, LOW);
  } else {
    digitalWrite(pinIN1, LOW);
    digitalWrite(pinIN2, HIGH);
  }

  analogWrite(pinEN, pwm);
}

/*
  Rutinas por motor: aceptan -100..100
  (Valores positivos: adelante; negativos: reversa)
*/
void motorIzquierdo(int velPct) {
  aplicarMotor(IN1, IN2, ENA, INVERTIR_MOTOR_IZQ, velPct);
}

void motorDerecho(int velPct) {
  aplicarMotor(IN3, IN4, ENB, INVERTIR_MOTOR_DER, velPct);
}

/*
  Mueve ambos motores a la vez. velIzq y velDer en -100..100
  (permite giros por diferencial: e.g., +60 y -60)
  Esta rutina puede llamarse para mover los motores especificando la velocidad de cada uno por separado.
  Rango: -100 ... 0 ... 100
  Valores negativos, mueve el motor correspondiente en reversa, valores positivos hacia adelante.
*/
void mover(int velIzq, int velDer) {
  motorIzquierdo(velIzq);
  motorDerecho(velDer);
}

/*
  Atajos de movimiento (velocidad en 0..100)
  Se pueden usar estas rutinas para:
   - avanzar: moverse hacia adelante con ambos motores a la misma velocidad.
   - retroceder: moverse hacia atrás con ambos motores a la misma velocidad.
   - girarDerecha y girarIzquierda: gira a la derecha al mover ambos motores con las mismas velocidades pero invertidas.
*/
void avanzar(int velocidadPct)     { mover(+velocidadPct, +velocidadPct); }
void retroceder(int velocidadPct)  { mover(-velocidadPct, -velocidadPct); }
void girarDerecha(int velocidadPct){ mover(+velocidadPct, -velocidadPct); }
void girarIzquierda(int velocidadPct){ mover(-velocidadPct, +velocidadPct); }

/*
  Alto inmediato. Usa el modo configurado (freno o rueda libre).
*/
void alto() {
  mover(0, 0);
}

/*
  Opcional: cambia el modo de frenado en tiempo real.
*/
void setFrenoActivo(bool activo) {
  FRENO_ACTIVO = activo;
}

// Inicialización del programa. Se ejecuta una sola vez al inciar el controlador.
void setup() {
  // Configurar pines como salida
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Estado inicial: alto
  alto();

  // (Opcional) Puede hacerse un arranque con un pequeño par para "despegar" motores duros:
  // mover(25, 25);
  // delay(150);
  // alto();
}

// EJEMPLO DE UNA TRAYECTORIA:

/*
  Recorre una trayectoria sencilla y la repite 'repeticiones' veces.
  Trayectoria:
    1) Avanza 1.5 s al 70%
    2) Gira a la derecha 0.6 s al 70%
    3) Avanza 1.0 s al 60%
    4) Gira a la izquierda 0.6 s al 70%
    5) Retrocede 0.7 s al 50%
    6) Alto 0.5 s
*/
void trayectoriaEjemplo(int repeticiones) {
  for (int i = 0; i < repeticiones; i++) {
    avanzar(70);         delay(1500);
    girarDerecha(70);    delay(600);
    avanzar(60);         delay(1000);
    girarIzquierda(70);  delay(600);
    retroceder(50);      delay(700);
    alto();              delay(500);
  }
}

// Esta rutina se ejecuta repite automáticamente después de ejecutar SETUP.
void loop() {
  // Repite la trayectoria 3 veces y espera 1 s entre bloques.
  trayectoriaEjemplo(3);
  alto();
  delay(1000); // espera 1000 milisegundos (1 segundo)

  // el siguiente ejemplo avanza en curva hacia la izquierda con diferencia 
  // de velocidad en ambos motores y luego alterna a la derecha.
  mover(60,100); // motor izquierdo +60, derecho +100
  delay(1000); // espera 1 segundo, los motores seguirán funcionando.
  mover(100, 60); // motor izquierdo +100, derecho +60
  delay(1000); // espera 1 segundo, los motores seguirán funconando.

  // también puede usarse con valores negativos
  mover(-75, 40); // mueve la rueda izquierda a -75, mientras mueve la derecha a +100
  // resultará en un movimiento de reversa con curva a la derecha.
  delay(1000); // espera 1 segundo, los motores seguirán funcionando.

  alto(); // se detienen los motores. equivalente a llamar mover(0,0);
  delay(3000); // espera 3 segundos. los motores estarán detenidos por la llamada alto()

}