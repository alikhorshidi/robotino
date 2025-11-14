#include <Servo.h>

// تعریف پین‌های سروو موتورها
#define BASE_ROTATION_PIN 8//2
#define BASE_SHOULDER1_PIN 7//3
#define BASE_SHOULDER2_PIN 6//4
#define ELBOW_PIN 5
#define WRIST1_PIN 4//6
#define WRIST2_PIN 3//7
#define GRIPPER_PIN 2//8

// ایجاد اشیاء سروو
Servo baseRotation;
Servo baseShoulder1;
Servo baseShoulder2;
Servo elbow;
Servo wrist1;
Servo wrist2;
Servo gripper;

// متغیرهای کنترل موقعیت
int currentBaseRotation = 90;
int currentBaseShoulder1 = 90;
int currentBaseShoulder2 = 90;
int currentElbow = 90;
int currentWrist1 = 90;
int currentWrist2 = 90;
int currentGripper = 90;

void setup() {
  // اتصال سروو موتورها به پین‌ها
  baseRotation.attach(BASE_ROTATION_PIN);
  baseShoulder1.attach(BASE_SHOULDER1_PIN);
  baseShoulder2.attach(BASE_SHOULDER2_PIN);
  elbow.attach(ELBOW_PIN);
  wrist1.attach(WRIST1_PIN);
  wrist2.attach(WRIST2_PIN);
  gripper.attach(GRIPPER_PIN);

  // تنظیم موقعیت اولیه
  setInitialPosition();

  // راه‌اندازی ارتباط سریال
  Serial.begin(9600);
}

void setInitialPosition() {
  // تنظیم موقعیت اولیه همه سروو موتورها در حالت میانی
  baseRotation.write(90);
  baseShoulder1.write(90);
  baseShoulder2.write(90);
  elbow.write(90);
  wrist1.write(90);
  wrist2.write(90);
  gripper.write(90);
}

void moveServo(Servo &servo, int &currentPosition, int targetPosition, int speed) {
  // حرکت نرم سروو با سرعت کنترل شده
  if (currentPosition < targetPosition) {
    for (int pos = currentPosition; pos <= targetPosition; pos++) {
      servo.write(pos);
      delay(speed);
    }
  } else {
    for (int pos = currentPosition; pos >= targetPosition; pos--) {
      servo.write(pos);
      delay(speed);
    }
  }
  currentPosition = targetPosition;
}

void moveShoulders(int targetPosition, int speed) {
  // حرکت همزمان شانه‌ها در جهت معکوس
  moveServo(baseShoulder1, currentBaseShoulder1, 90 - (targetPosition - 90), speed);
  moveServo(baseShoulder2, currentBaseShoulder2, 90 + (targetPosition - 90), speed);
}

void controlRobotArm() {
  // دریافت دستورات از طریق سریال
  static String inputString = "";
  static boolean stringComplete = false;

  while (Serial.available()) {
    char inChar = (char)Serial.read();
    
    // اضافه کردن کاراکتر به رشته ورودی
    inputString += inChar;
    
    // بررسی پایان دستور
    if (inChar == '\n') {
      stringComplete = true;
    }
  }

  // پردازش دستور کامل
  if (stringComplete) {
    // حذف کاراکترهای اضافی
    inputString.trim();
    
    // بررسی طول دستور
    if (inputString.length() >= 2) {
      char command = inputString.charAt(0);
      int value = inputString.substring(1).toInt();

      // اجرای دستور
      switch(command) {
        case 'A': // چرخش پایه
          moveServo(baseRotation, currentBaseRotation, value, 15);
          break;
        case 'B': // شانه‌ها
          moveShoulders(value, 15);
          break;
        case 'D': // آرنج
          moveServo(elbow, currentElbow, value, 15);
          break;
        case 'E': // مچ اول
          moveServo(wrist1, currentWrist1, value, 15);
          break;
        case 'F': // مچ دوم
          moveServo(wrist2, currentWrist2, value, 15);
          break;
        case 'G': // گریپر
          moveServo(gripper, currentGripper, value, 15);
          break;
        case 'H': // موقعیت اولیه
          setInitialPosition();
          break;
      }
    }

    // پاک کردن رشته ورودی
    inputString = "";
    stringComplete = false;
  }
}

void loop() {
  controlRobotArm();
}
