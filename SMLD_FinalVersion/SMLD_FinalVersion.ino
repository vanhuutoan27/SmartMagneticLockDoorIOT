//Thư viện RFID
#include <SPI.h>
#include <MFRC522.h>
//Thư viện Servo
#include <Servo.h>
//Thư viện I2C
#include <Wire.h>
//Thư viện Keypad
#include <Keypad.h>
//Thư viện LCD I2C
#include <LiquidCrystal_I2C.h>

//Định nghĩa hằng số các chân Arduino (9, 10) và MFRC522 (RST_PIN, SS_PIN)
#define SS_PIN 10
#define RST_PIN 9
//Arduino (8) kết nối Còi Chíp - Buzzer (buzzerPin) (Đầu Dương)
const int buzzerPin = 8;
//Arduino (7) kết nối với Servo (doorLock)
#define doorLock 7

//Định nghĩa số hàng (numRows) và số cột (numCols) cho Keypad 2x3
//Đồng thời, định nghĩa các chân (rowPins và colPins) được sử dụng để kết nối Keypad
const byte numRows = 2;
const byte numCols = 3;
byte rowPins[numRows] = { 2, 3 };
byte colPins[numCols] = { 4, 5, 6 };

//Khởi tạo đối tượng "mfrc522" (RFID RC522)
MFRC522 mfrc522(SS_PIN, RST_PIN);
//Khởi tạo đối tượng "lcd" (LCD)
LiquidCrystal_I2C lcd(0x27, 16, 2);
//Khởi tạo đối tượng "myservo" (Servo)
Servo myservo;

//Định nghĩa mảng "keys" để lưu các phím trên keypad(2 hàng 3 cột), và các giá trị tương ứng được gán cho mỗi nút
char keys[numRows][numCols] = {
  { '1', '2', '3' },
  { '4', '5', '6' }
};

//Khởi tạo đối tượng keypad sử dụng thông số như ma trận phím (makeKeymap(keys)), các chân hàng và cột, số hàng và số cột
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, numRows, numCols);

//Khai báo biến "pos" để lưu giá trị góc của servo motor.
int pos = 0;

//Khai báo "maxAttempts" để xác định số lần nhập sai tối đa được phép
//"wrongAttempts" lưu số lần nhập sai hiện tại
//enteredPassword lưu mật khẩu đã nhập
//"passwordEntered" xác định xem mật khẩu đã được nhập hay chưa (Mặc định là chưa)
const int maxAttempts = 3;
int wrongAttempts = 0;
String enteredPassword = "";
bool passwordEntered = false;

//============================================================================================//
//========================================[VOID SETUP]========================================//
//============================================================================================//

void setup() {
  //Khởi động giao tiếp Serial với tốc độ 9600 baud (Baud là một đơn vị đo tốc độ truyền dữ liệu,
  //Tốc độ 9600 baud là một thang đo được sử dụng trong việc đo lường tốc độ truyền dữ liệu thông qua giao tiếp serial)
  //Cho phép chương trình gửi và nhận dữ liệu qua cổng Serial (USB) để theo dõi và gỡ lỗi
  Serial.begin(9600);
  //Khởi động SPI và Sử dụng để giao tiếp với RFID (MFRC522)
  SPI.begin();
  //Khởi động module MFRC522. Hàm này khởi tạo và thiết lập module để sẵn sàng đọc các thẻ RFID
  mfrc522.PCD_Init();
  //Khởi động màn hình LCD. Hàm này khởi tạo và cấu hình màn hình LCD để sẵn sàng hoạt động
  lcd.init();
  //Kết nối servo với (doorLock)
  //Hàm này khởi tạo kết nối giữa servo motor và chân điều khiển để sẵn sàng điều khiển servo
  myservo.attach(doorLock);
  //Thiết lập buzzerPin là OUTPUT (đầu ra)
  //Điều này cho phép chương trình điều khiển chân đó để phát ra âm thanh thông qua buzzer
  pinMode(buzzerPin, OUTPUT);
  //Bật đèn nền của màn hình LCD. Kích hoạt đèn nền để hiển thị nội dung
  lcd.backlight();
  //Đặt vị trí con trỏ trên màn hình LCD (0,0)
  lcd.setCursor(0, 0);
  lcd.print("Enter password:");
  delay(500);
}

//============================================================================================//
//========================================[VOID LOOP]=========================================//
//============================================================================================//

void loop() {
  //Nhận giá trị phím được nhập
  //Nếu có key thì xét có phải key 6 không, nếu key 6 thì thực hiện hàm checkPassword()
  //Ngược lại chương trình sẽ tiếp tục nhận key được nhập và hiển thị ra màn hình
  char key = keypad.getKey();
  if (key != NO_KEY) {
    if (key == '6') {
      checkPassword();
      delay(500);
    } else {
      enteredPassword += key;
      lcd.setCursor(0, 1);
      lcd.print(enteredPassword);
    }
  }

  //Kiểm tra có phải thẻ mới không
  //Nếu không thì return
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  //Đọc thông tin từ thẻ từ
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  //Thực hiện hàm checkCard()
  checkCard();
}

//============================================================================================//
//===================================[CHECK CARD FUNCTION]====================================//
//============================================================================================//

void checkCard() {
  //Khai báo biến content dùng để lưu trữ nội dung thẻ RFID được đọc
  String content = "";
  byte letter;
  //
  //Trong for, content được cập nhật bằng cách nối các giá trị byte của địa chỉ thẻ
  //Dòng 132 dùng để kiểm tra xem giá trị byte có nhỏ hơn 0x10 hay không. Nếu nhỏ hơn, nối thêm " 0" vào content để đảm bảo định dạng chuẩn
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i]));
  }

  //Kiểm tra xem chuỗi được nối từ việc đọc thẻ có khớp với ID thẻ không
  if (content.substring(1) == "53 18 00 01") {
    //Reset số lần nhập sai thành 0
    wrongAttempts = 0;
    //Xóa màn hình, đặt con trỏ ở (hàng 3 dòng 0)
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Valid Card!");

    //Còi chíp kêu và dừng lại 1s
    buzz(100, 100);
    delay(1000);

    //Thực hiện hàm mở cửa
    openDoor();
  } else {
    //Số lần nhập sai tăng lên 1
    wrongAttempts++;
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Invalid Card!");
    lcd.setCursor(0, 1);
    lcd.print("Please try again!");

    //Còi chíp kêu lên nhưng lâu hơn
    buzz(500, 200);

    //Nếu nhập sai quá 3 lần (maxAttempts = 3) --> Khóa 5s
    if (wrongAttempts >= maxAttempts) {
      buzz(1000, 100);
      buzz(1000, 100);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Unlock after...");
      //Thực hiện hàm countdown 5s
      countdown(5);
    } else {
      delay(3000);
    }

    buzz(100, 100);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enter password:");
  }
}
//============================================================================================//
//=================================[CHECK PASSWORD FUNCTION]==================================//
//============================================================================================//

void checkPassword() {
  //Set mật khẩu cố định là 1234, có thể sửa
  String password = "1234";
  //Check nếu mật khẩu nhập vào bằng passowrd = "1234"
  if (enteredPassword == password) {
    //Mật khẩu đã nhập là có
    passwordEntered = true;
    wrongAttempts = 0;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Correct password!");

    buzz(100, 100);

    openDoor();
  } else {
    wrongAttempts++;
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Incorrect pass!");
    lcd.setCursor(0, 1);
    lcd.print("Please try again!");

    buzz(500, 200);

    if (wrongAttempts >= maxAttempts) {
      buzz(1000, 100);
      buzz(1000, 100);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Unlock after...");
      countdown(5);
    } else {
      delay(3000);
    }
  }
  buzz(100, 100);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter password:");
  enteredPassword = "";
}

//============================================================================================//
//====================================[OPEN DOOR FUNCTION]====================================//
//============================================================================================//

void openDoor() {
  //Pos là vị trí hiện tại của động cơ, nếu vị trí hiện tại >= 0 độ, động cơ sẽ -1 độ cho tới khi = 20 độ sẽ dừng
  //Mở cửa
  for (pos = 90; pos >= 0; pos -= 1) {
    myservo.write(pos);
    delay(5);
  }
  delay(2000);
  //Nếu vị trí hiện tại <= 90 độ, động cơ sẽ +1 độ cho tới khi = 90 độ sẽ dừng
  //Đóng cửa
  for (pos = 0; pos <= 90; pos += 1) {
    //Động cơ hoạt động
    myservo.write(pos);
    delay(5);
  }
}

//============================================================================================//
//======================================[BUZZ FUNCTION]=======================================//
//============================================================================================//

void buzz(int duration, int delayTime) {
  //Còi chíp hoạt động
  digitalWrite(buzzerPin, HIGH);
  //Trong khoảng thời gian duration đã lập trình
  delay(duration);
  //Còi chíp dừng hoạt động
  digitalWrite(buzzerPin, LOW);
  //Còi chíp dừng lại trong bao lâu rồi tiếp tục thực thi chương trình khác
  delay(delayTime);
}

//============================================================================================//
//===================================[COUNTDOWN FUNCTION]=====================================//
//============================================================================================//

void countdown(int seconds) {
  for (int i = seconds; i >= 0; i--) {
    lcd.setCursor(13, 1);
    lcd.print(i);
    delay(1000);
  }
}