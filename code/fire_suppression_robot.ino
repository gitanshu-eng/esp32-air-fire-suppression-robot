// ---------------- MOTOR DRIVER PINS ----------------
#define ENA 13
#define ENB 25
#define IN1 12
#define IN2 14
#define IN3 27
#define IN4 26

// ---------------- IR SENSORS ----------------
#define IR1 5
#define IR2 19
#define IR3 18
#define IR4 21

// ---------------- FLAME ANALOG PINS ----------------
#define FL   33
#define FLM  32
#define FC   35
#define FRM  34
#define FR   36

// ---------------- RELAY ----------------
#define RELAY 22
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// ---------------- SETTINGS ----------------
#define THRESHOLD 1500
#define SPEED_FORWARD 95
#define SPEED_TURN 100

// PWM setup (ESP32)
#define PWM_FREQ 1000
#define PWM_RES 8
#define CH_A 0
#define CH_B 1

// ---------------- STATES ----------------
enum RobotState { ROAMING, FIRE_TRACKING };
RobotState state = ROAMING;

// ---------------- MOTOR FUNCTIONS ----------------
void setSpeed(int a, int b){
  ledcWrite(CH_A, a);
  ledcWrite(CH_B, b);
}

void forward(){
  Serial.println("→ Forward");
  digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);
  digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);
}

void backward(){
  Serial.println("→ Backward");
  digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH);
  digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH);
}

void turnLeft(){
  Serial.println("→ Turning Left");
  digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH);
  digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);
}

void turnRight(){
  Serial.println("→ Turning Right");
  digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH);
}

void stopMotors(){
  Serial.println("→ Motors Stopped");
  digitalWrite(IN1,LOW);
  digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW);
  digitalWrite(IN4,LOW);
}

// ---------------- SETUP ----------------
void setup(){
  Serial.begin(115200);

  pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT); pinMode(IN4,OUTPUT);

  pinMode(IR1,INPUT_PULLUP);
  pinMode(IR2,INPUT_PULLUP);
  pinMode(IR3,INPUT_PULLUP);
  pinMode(IR4,INPUT_PULLUP);

  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, RELAY_OFF);

  ledcSetup(CH_A,PWM_FREQ,PWM_RES);
  ledcSetup(CH_B,PWM_FREQ,PWM_RES);
  ledcAttachPin(ENA,CH_A);
  ledcAttachPin(ENB,CH_B);

  Serial.println("🤖 Robot Ready");
}

// ---------------- LOOP ----------------
void loop(){

  // READ SENSORS
  int ir1 = digitalRead(IR1);
  int ir2 = digitalRead(IR2);

  int left   = analogRead(FL);
  int leftM  = analogRead(FLM);
  int center = analogRead(FC);
  int rightM = analogRead(FRM);
  int right  = analogRead(FR);

  bool flameDetected = (left>THRESHOLD || leftM>THRESHOLD ||
                        center>THRESHOLD || rightM>THRESHOLD ||
                        right>THRESHOLD);

  // PRINT SENSORS
  Serial.println("------------------------------------------------");
  Serial.print("IR1="); Serial.print(ir1);
  Serial.print("  IR2="); Serial.println(ir2);

  Serial.print("FL=");  Serial.print(left);
  Serial.print("  FLM="); Serial.print(leftM);
  Serial.print("  FC="); Serial.print(center);
  Serial.print("  FRM="); Serial.print(rightM);
  Serial.print("  FR="); Serial.println(right);

  Serial.print("FlameDetected: ");
  Serial.println(flameDetected ? "YES" : "NO");

  // ---------- SWITCH TO FIRE MODE ----------
  if(state == ROAMING && flameDetected){
    Serial.println("🔥 Flame detected → Switching to FIRE_TRACKING mode");
    state = FIRE_TRACKING;
  }

  // ---------- ROAMING MODE ----------
  if(state == ROAMING){

    Serial.println("STATE: ROAMING");

    if(ir1 == HIGH || ir2 == HIGH){
      Serial.println("⚠️ Obstacle detected!");
      setSpeed(SPEED_FORWARD,SPEED_FORWARD);
      backward();
      delay(700);

      setSpeed(SPEED_TURN,SPEED_TURN);
      turnRight();
      delay(600);
    }
    else{
      setSpeed(SPEED_FORWARD,SPEED_FORWARD);
      forward();
    }
  }

  // ---------- FIRE TRACKING MODE ----------
  else if(state == FIRE_TRACKING){

    Serial.println("STATE: FIRE_TRACKING");

    // 🔥 FIRE REACHED
    if(center > THRESHOLD &&
       center >= leftM && center >= rightM &&
       (ir1 == HIGH || ir2 == HIGH)){

        Serial.println("🔥 FIRE REACHED → START EXTINGUISH LOOP");
        stopMotors();

        // 🔁 EXTINGUISH LOOP
        while(true){

          bool stillFlame =
            analogRead(FL)  > THRESHOLD ||
            analogRead(FLM) > THRESHOLD ||
            analogRead(FC)  > THRESHOLD ||
            analogRead(FRM) > THRESHOLD ||
            analogRead(FR)  > THRESHOLD;

          if(!stillFlame){
            Serial.println("✅ FIRE OUT");
            digitalWrite(RELAY, RELAY_OFF);

            Serial.println("Escaping fire area...");
            backward();
            delay(700);
            turnRight();
            delay(700);

            state = ROAMING;
            return;
          }

          Serial.println("🌀 FAN ON (5s)");
          digitalWrite(RELAY, RELAY_ON);
          delay(5000);

          Serial.println("⏸ FAN PAUSE (2s)");
          digitalWrite(RELAY, RELAY_OFF);
          delay(2000);
        }
    }

    // 🔥 TRACK FIRE DIRECTION
    if(center >= leftM && center >= rightM){
      Serial.println("CENTER strongest → Forward");
      setSpeed(SPEED_FORWARD,SPEED_FORWARD);
      forward();
    }
    else if(left > center || leftM > center){
      Serial.println("LEFT stronger → Turning Left");
      setSpeed(SPEED_TURN,SPEED_TURN);
      turnLeft();
    }
    else if(right > center || rightM > center){
      Serial.println("RIGHT stronger → Turning Right");
      setSpeed(SPEED_TURN,SPEED_TURN);
      turnRight();
    }
  }

  delay(150);
}