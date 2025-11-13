/*
 * =============================================================================
 * پروژه: کنترل ربات مکانوم چهار چرخ با ESP32 و TB6600
 * راهنما: برای دانشجویان رباتیک و مکاترونیک
 *
 * (توضیح استاد): این کد برای کامپایل با هسته ESP32 آردوینو
 * نسخه 3.x.x و بالاتر به‌روزرسانی شده است.
 *
 * توضیحات:
 * این برنامه یک ربات چهار چرخ مکانوم را با استفاده از برد ESP32 و چهار
 * درایور استپر موتور TB6600 کنترل می‌کند.
 *
 * معماری کلیدی:
 * این کد از روش سنتی پالس‌دهی نرم‌افزاری (Software Pulsing) که منجر به لرزش
 * و قفل شدن CPU می‌شود، استفاده *نمی‌کند*.
 * به جای آن، ما از واحد سخت‌افزاری LEDC (LED Control) داخلی ESP32 برای
 * تولید سیگنال‌های پالس (PUL) با فرکانس دقیق و پایدار استفاده می‌کنیم.
 *
 * مزیت این روش:
 * 1. پایداری کامل: پالس‌ها توسط سخت‌افزار و بدون هیچ لرزشی (Jitter) تولید می‌شوند.
 * 2. آزادسازی CPU: پردازنده اصلی کاملاً آزاد است و درگیر تولید پالس نمی‌شود.
 * 3. کد ساده: ما می‌توانیم با خیال راحت از تابع delay() در حلقه اصلی (loop)
 *    استفاده کنیم، زیرا سخت‌افزار LEDC در پس‌زمینه به کار خود ادامه می‌دهد.
 *
 * (توضیح استاد): به تغییرات نسبت به نسخه 2.x دقت کنید:
 * 1. دیگر نیازی به تعریف دستی کانال‌های LEDC نیست.
 * 2. تابع 'ledcAttach' جایگزین 'ledcSetup' و 'ledcAttachPin' شده است.
 * 3. تابع 'ledcWrite' اکنون به جای 'کانال'، مستقیماً 'پین' را صدا می‌زند.
 * =============================================================================
 */

// --- تعریف پین‌های موتور جلو-راست (FR) ---
#define FR_ENABLE_PIN 32 // پین فعال‌ساز
#define FR_DIR_PIN    33 // پین جهت
#define FR_PULSE_PIN  25 // پین پالس

// --- تعریف پین‌های موتور جلو-چپ (FL) ---
#define FL_ENABLE_PIN 26 // پین فعال‌ساز
#define FL_DIR_PIN    27 // پین جهت
#define FL_PULSE_PIN  13 // پین پالس

// --- تعریف پین‌های موتور عقب-راست (RR) ---
#define RR_ENABLE_PIN 18 // پین فعال‌ساز
#define RR_DIR_PIN    19 // پین جهت
#define RR_PULSE_PIN  21 // پین پالس

// --- تعریف پین‌های موتور عقب-چپ (RL) ---
#define RL_ENABLE_PIN 4  // پین فعال‌ساز
#define RL_DIR_PIN    16 // پین جهت
#define RL_PULSE_PIN  17 // پین پالس

// =============================================================================
// (توضیح استاد): این بخش برای تنظیم سرعت ربات است.
// =============================================================================
// فرکانس پالس‌ها سرعت موتور را تعیین می‌کند (هرتز یا پالس بر ثانیه).
// برای افزایش سرعت ربات، این عدد را افزایش دهید (مثلاً 4000).
// برای کاهش سرعت ربات، این عدد را کاهش دهید (مثلاً 1000).
#define STEP_FREQUENCY 1500

// =============================================================================
// (توضیح استاد): این بخش پارامترهای فنی واحد LEDC را تنظیم می‌کند.
// =============================================================================
// رزولوشن تایمر LEDC. 8 بیت به ما بازه 0 تا 255 را برای Duty Cycle می‌دهد.
#define LEDC_TIMER_BITS 8
// ما به یک موج مربعی 50% نیاز داریم. برای رزولوشن 8 بیت، این مقدار 128 می‌شود.
// (1 << (8 - 1)) = (1 << 7) = 128
#define LEDC_DUTY_CYCLE (1 << (LEDC_TIMER_BITS - 1))

// (توضیح استاد): این تعاریف، کد ما را خواناتر می‌کنند.
// بسته به سیم‌کشی موتور شما، ممکن است لازم باشد این دو را جابجا کنید.
#define FORWARD  HIGH
#define BACKWARD LOW

// =============================================================================
// تابع پیکربندی اولیه (Setup)
// =============================================================================
void setup() {
  // راه‌اندازی ارتباط سریال برای مانیتورینگ و دیباگ
  Serial.begin(115200);
  Serial.println("در حال آماده‌سازی ربات مکانوم (هسته v3.x.x)...");

  // تنظیم تمام 12 پین کنترلی به عنوان خروجی (OUTPUT)
  pinMode(FR_ENABLE_PIN, OUTPUT);
  pinMode(FR_DIR_PIN, OUTPUT);
  pinMode(FR_PULSE_PIN, OUTPUT);
  
  pinMode(FL_ENABLE_PIN, OUTPUT);
  pinMode(FL_DIR_PIN, OUTPUT);
  pinMode(FL_PULSE_PIN, OUTPUT);
  
  pinMode(RR_ENABLE_PIN, OUTPUT);
  pinMode(RR_DIR_PIN, OUTPUT);
  pinMode(RR_PULSE_PIN, OUTPUT);
  
  pinMode(RL_ENABLE_PIN, OUTPUT);
  pinMode(RL_DIR_PIN, OUTPUT);
  pinMode(RL_PULSE_PIN, OUTPUT);

  // (توضیح استاد): درایورهای TB6600 معمولاً Active-LOW هستند.
  // یعنی با ارسال سیگنال LOW، درایور *فعال* می‌شود.
  digitalWrite(FR_ENABLE_PIN, LOW);
  digitalWrite(FL_ENABLE_PIN, LOW);
  digitalWrite(RR_ENABLE_PIN, LOW);
  digitalWrite(RL_ENABLE_PIN, LOW);

  // --- پیکربندی هسته اصلی: واحد LEDC (API v3.x.x) ---
  
  // (توضیح استاد): در این 4 خط، ما تایمرهای سخت‌افزاری را پیکربندی و
  // به پین‌ها متصل می‌کنیم. تابع 'ledcAttach' هر سه کار (انتخاب کانال،
  // تنظیم فرکانس و رزولوشن، و اتصال به پین) را در یک مرحله انجام می‌دهد.
  ledcAttach(FR_PULSE_PIN, STEP_FREQUENCY, LEDC_TIMER_BITS);
  ledcAttach(FL_PULSE_PIN, STEP_FREQUENCY, LEDC_TIMER_BITS);
  ledcAttach(RR_PULSE_PIN, STEP_FREQUENCY, LEDC_TIMER_BITS);
  ledcAttach(RL_PULSE_PIN, STEP_FREQUENCY, LEDC_TIMER_BITS);
  
  Serial.println("ربات آماده است. شروع سکانس دمو...");
}

// =============================================================================
// توابع کنترلی سطح پایین (Low-Level Control)
// =============================================================================

// (توضیح استاد): این تابع جهت چرخش هر چهار موتور را تنظیم می‌کند.
void setMotorDirections(int frDir, int flDir, int rrDir, int rlDir) {
  digitalWrite(FR_DIR_PIN, frDir);
  digitalWrite(FL_DIR_PIN, flDir);
  digitalWrite(RR_DIR_PIN, rrDir);
  digitalWrite(RL_DIR_PIN, rlDir);
}

// (توضیح استاد): این تابع به سخت‌افزار LEDC دستور می‌دهد که تولید پالس را
// با Duty Cycle تنظیم شده (128 یا 50%) آغاز کند.
// در API v3.x، ما دستور را مستقیماً به پین ارسال می‌کنیم.
void activateMotors() {
  ledcWrite(FR_PULSE_PIN, LEDC_DUTY_CYCLE);
  ledcWrite(FL_PULSE_PIN, LEDC_DUTY_CYCLE);
  ledcWrite(RR_PULSE_PIN, LEDC_DUTY_CYCLE);
  ledcWrite(RL_PULSE_PIN, LEDC_DUTY_CYCLE);
}

// (توضیح استاد): این تابع با ارسال Duty Cycle صفر به پین، تولید پالس را متوقف می‌کند.
// موتورها می‌ایستند اما "گشتاور نگهدارنده" (Holding Torque) را حفظ می‌کنند.
void robotStop() {
  Serial.println("توقف.");
  ledcWrite(FR_PULSE_PIN, 0);
  ledcWrite(FL_PULSE_PIN, 0);
  ledcWrite(RR_PULSE_PIN, 0);
  ledcWrite(RL_PULSE_PIN, 0);
}

// =============================================================================
// توابع حرکتی سطح بالا (High-Level Abstraction)
// (توضیح استاد): اینها توابعی هستند که شما برای نوشتن مانورهای خود استفاده خواهید کرد.
// این توابع، منطق جدول کینماتیک (جدول ۱ در راهنما) را پیاده‌سازی می‌کنند.
// =============================================================================

// مانور: حرکت مستقیم به جلو
void moveForward() {
  Serial.println("حرکت به جلو...");
  setMotorDirections(FORWARD, FORWARD, FORWARD, FORWARD);
  activateMotors();
}

// مانور: حرکت مستقیم به عقب
void moveBackward() {
  Serial.println("حرکت به عقب...");
  setMotorDirections(BACKWARD, BACKWARD, BACKWARD, BACKWARD);
  activateMotors();
}

// مانور: حرکت جانبی (خرچنگی) به چپ
void strafeLeft() {
  Serial.println("حرکت جانبی به چپ (Strafe Left)...");
  // منطبق بر جدول ۱
  setMotorDirections(BACKWARD, FORWARD, FORWARD, BACKWARD);
  activateMotors();
}

// مانور: حرکت جانبی (خرچنگی) به راست
void strafeRight() {
  Serial.println("حرکت جانبی به راست (Strafe Right)...");
  // منطبق بر جدول ۱
  setMotorDirections(FORWARD, BACKWARD, BACKWARD, FORWARD);
  activateMotors();
}

// مانور: چرخش درجا به صورت ساعتگرد (Clockwise)
void rotateClockwise() {
  Serial.println("چرخش ساعتگرد (CW)...");
  // منطبق بر جدول ۱
  setMotorDirections(FORWARD, BACKWARD, FORWARD, BACKWARD);
  activateMotors();
}

// مانور: چرخش درجا به صورت پادساعتگرد (Counter-Clockwise)
void rotateCounterClockwise() {
  Serial.println("چرخش پادساعتگرد (CCW)...");
  // منطبق بر جدول ۱
  setMotorDirections(BACKWARD, FORWARD, BACKWARD, FORWARD);
  activateMotors();
}

// =============================================================================
// حلقه اصلی برنامه (Loop)
// (توضیح استاد): در اینجا ما سکانس نمایشی را اجرا می‌کنیم.
// =============================================================================
void loop() {
  
  Serial.println("--- شروع دور جدید دمو ---");

  // 1. حرکت به جلو برای 5 ثانیه
  moveForward();
  delay(5000);

  // توقف 1 ثانیه‌ای بین مانورها
  robotStop();
  delay(1000);

  // 2. حرکت به عقب برای 5 ثانیه
  moveBackward();
  delay(5000);

  robotStop();
  delay(1000);

  // 3. حرکت جانبی به چپ برای 5 ثانیه
  strafeLeft();
  delay(5000);

  robotStop();
  delay(1000);

  // 4. حرکت جانبی به راست برای 5 ثانیه
  strafeRight();
  delay(5000);

  robotStop();
  delay(1000);

  // --- نمایش مانورهای اضافی ---

  // 5. چرخش ساعتگرد برای 3 ثانیه
  rotateClockwise();
  delay(3000);

  robotStop();
  delay(1000);

  // 6. چرخش پادساعتگرد برای 3 ثانیه
  rotateCounterClockwise();
  delay(3000);

  // توقف طولانی‌تر (5 ثانیه) قبل از تکرار کل سکانس
  robotStop();
  delay(5000);
}
