#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <EEPROM.h>
#include <Stepper.h>

//ultra sonic.........
#define trig 3
#define echo 4
//flowrate sensor
int sensorPin = 2;
volatile long pulse;
float volume; 

//display & key pad
const int LCD_ADDRESS = 0x27;  
const int LCD_COLUMNS = 20;   
const int LCD_ROWS = 4;      

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

const byte ROWS = 4;           
const byte COLS = 4;          
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte pinRows[ROWS] = {13, 12, 11, 10};    
byte pinCols[COLS] = {9, 8, 7, 6};      

Keypad keypad = Keypad(makeKeymap(keys), pinRows, pinCols, ROWS, COLS);

// stepper moter
const int STEPS_PER_REV = 200;
Stepper mystepper(STEPS_PER_REV, 45, 47, 49, 51);

String userInput = " ";  // Variable to store user input
int k=0,area,current_volume,total_volume;

int nPrice;//new entered oil price
int oPrice;//entered old price

//temp_variable for stepper moter control.  
int temp1 =1;
int temp2 =1;

//password declare
int password2=1234;
int password3;

void setup() {
  Serial.begin(9600);

  //IR..................
  pinMode(5,INPUT); 
  pinMode(39,INPUT); // move
  pinMode(40,INPUT); // upper
  //ultra_sonic.........
  pinMode(trig,OUTPUT); 
  pinMode(echo,INPUT); 
  //flowrate senser
  pinMode(sensorPin, INPUT);
  
  attachInterrupt(digitalPinToInterrupt(sensorPin), increase, RISING);
   //Display
  lcd.begin(LCD_COLUMNS, LCD_ROWS);
  lcd.init();
  lcd.backlight();

  //temp output for oil pump moter and solinoid.
  pinMode(44,OUTPUT); //relay

  //stepper moter
  mystepper.setSpeed(300);

  // switch push button
  pinMode(41,INPUT_PULLUP); // start button
  pinMode(35,INPUT_PULLUP); // stop button
  // Buzzer
  pinMode(36,OUTPUT);

  lcd.setCursor(3,1);
  lcd.print("--WELCOME--");
  delay(1000);
  lcd.clear();

}

void loop() {
  menu();
}

//menu........................................
int menu(){
  lcd.setCursor(0,0);  
  lcd.print("1:Enter Oil in mL");
  lcd.setCursor(0,1);
  lcd.print("2:Money amount");  
  lcd.setCursor(0,2);
  lcd.print("3:Change oil price");
  lcd.setCursor(0,3);
  lcd.print("4:Price of oil 1L");


  while(1){

    char key = keypad.getKey();
    
    if(key){

      lcd.clear();
      
      if(key=='1'){
        getUserInputNumeric();
        k = userInput.toInt();
        filling_process();
        lcd.clear();
        return 0;
      }
      else if(key=='2'){
        getUserInputString();
        EEPROM.get(0, nPrice);
        k=(userInput.toInt()*1000)/nPrice;//change the price in to mililiters amount
        price_filling_process();//price filling display change function
        lcd.clear();
        return 0;
      }
      else if(key=='3'){
        price();
        return 0;
      }
      else if(key=='4'){
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("price for 1l->");
          lcd.setCursor(0,1);
          EEPROM.get(0, nPrice);
          lcd.print(nPrice);  
          delay(2000);
        return 0;
      }
      else{
        lcd.clear();
        lcd.setCursor(3,1);
        lcd.print("Invalid input");
        delay(1000);
        lcd.clear();
        return 0;
      }
  }
    // return 0; // meka ain karoth senser tika wada ..........while eke therumal thiyenawada???
  }
}

//keypad........................................................................
void getUserInputNumeric() {
  displayVolume();
  lcd.setCursor(0, 0);
  lcd.print("Oil in ml: ");
  lcd.setCursor(0, 2);
  lcd.print("Press #: Enter");
  lcd.setCursor(0, 3);
  lcd.print("Press D: Home");
  lcd.setCursor(userInput.length()+10, 0);
  userInput = "";

  while (true) {
    char key = keypad.getKey();

    if (key == '#') {
      break;
    }  else if (key == '*') {
      if (userInput.length() > 0) {
        userInput.remove(userInput.length() - 1);
        lcd.setCursor(userInput.length()+10, 0);
        lcd.print(" ");  // Clear the character
        lcd.setCursor(userInput.length()+10, 0);
      }
    }else if (key == 'D') {
      lcd.clear();
      //return by function to the main display
      while(1){
        menu();
      }
    } 
    else if (key) {
      userInput += key;
      lcd.print(key);
    }
  }

  // int numericValue = userInput.toInt();
  // lcd.clear();

}

int displayVolume(){
  lcd.clear();
  digitalWrite(trig,LOW);
  delayMicroseconds(2);
  digitalWrite(trig,HIGH);
  delayMicroseconds(10);
  digitalWrite(trig,LOW);
  long t = pulseIn(echo,HIGH);
  long cm = t/29/2;
  long current_volume = 16000 - (640*cm);
  lcd.setCursor(0, 0);
  lcd.print("Current Capacity:");
  lcd.setCursor(0, 3);
  lcd.print(current_volume);
  lcd.print(" ml");
  delay(2000);
  lcd.clear();
}

//filling_process............................
int filling_process(){
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("Entered Value: ");
  lcd.print(k);
  k=1.1*k;
  delay(900);
  start();
  check();

}
//start
int start(){
  lcd.clear();
  lcd.setCursor(1,1);
  lcd.print("Press Start Button");
  while(1){
    if(digitalRead(41)==LOW){
      break;
    }
      // Emg. stop
    if(digitalRead(35)==LOW){
      lcd.clear();
      while(1){
        menu();
      }
    }
  }
}

int check(){
  while(1){
    if(sonic_testing()){
      if(ir_testing()){
        if(motor_down()){
          if(flow_testing()){
            digitalWrite(44,HIGH);
            lcd.clear();
            lcd.setCursor(5,1);
            lcd.print("Filling....");
        }
        else{
            digitalWrite(44,LOW);
            lcd.clear();
            lcd.setCursor(3,1);
            lcd.print("Finished");
            for(int i=0;i<10;i++){  // Beep sound
              digitalWrite(36,HIGH);
              delay(200);
              digitalWrite(36,LOW);
              delay(200);
            }
            delay(4000);
            motor_up();
            pulse = 0;
            userInput = ""; 
            return 0;
        }
        }
      }        
    }
    else{
      return 0;
    }
  }

}

//ultra sonic senser........................................................
int sonic_testing(){
  digitalWrite(trig,LOW);
  delayMicroseconds(2);
  digitalWrite(trig,HIGH);
  delayMicroseconds(10);
  digitalWrite(trig,LOW);
  long t = pulseIn(echo,HIGH);
  long cm = t/29/2;
  long current_volume = total_volume - (640*cm); //calculate current_volume
    //print.............
    Serial.print(current_volume);
    Serial.println(" cm^3");
    delay(300);

  if(current_volume>k){// 25 wenukawata k daanna ooona calculate kerala
    return 1;
  }
  else{
    lcd.clear();
    lcd.setCursor(0,1);
    lcd.print("Insuffient Storage");
    delay(1000);
    return 0;
  }
}

//ir senser..................................................................
int ir_testing(){
  if(digitalRead(5)==LOW){
    return 1;
  }
  else{
    digitalWrite(44,LOW);
    lcd.clear();
    lcd.setCursor(0,1);
    lcd.print("Keep Bottle Properly");
    delay(900);
    return 0;
  }  
}

//flow rate senser.............................................................
int flow_testing(){
  volume = 2.663 * pulse;
  delay(10);
    
  if(volume > k){
    return 0;
    
   }
   else{
    return 1;
   }
}

void increase(){
pulse++;
}

// ============keypad option 2====================
void getUserInputString() {
  lcd.setCursor(0, 0);
  lcd.print("Amount money in Rs->");
  lcd.setCursor(0, 2);
  lcd.print("Press #: Enter");
  lcd.setCursor(0, 3);
  lcd.print("Press D: Home");
  lcd.setCursor(userInput.length(), 1);
  userInput = "";

  while (true) {
    char key = keypad.getKey();
     lcd.setCursor(userInput.length(), 1);
    if (key == '#') {
      break;      
    } else if (key == '*') {
      if (userInput.length() > 0) {
        userInput.remove(userInput.length() - 1);
        lcd.setCursor(userInput.length(), 1);
        lcd.print(" ");  // Clear the character
        lcd.setCursor(userInput.length(), 1);
      }
    }else if (key == 'D') {
      lcd.clear();
      //return by function to the main display
      while(1){
          menu();
      }
    } 
    else if (key) {
      userInput += key;
      lcd.print(key);
    }
  }

  lcd.clear();
  // lcd.print("Amount in Rs: ");
  // lcd.print(userInput);
}


//price to volume display
int price_filling_process(){
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("Price Value in ml ->");
  lcd.setCursor(0,2);
  lcd.print(k);
  delay(1200);
  
  start();
  check();
}

//volume price function
int price(){
  userInput = "";
  lcd.print("Enter the password->");//get the password
  lcd.setCursor(0, 2);
  lcd.print("Press A: Change pin");
  lcd.setCursor(0, 3);
  lcd.print("Press D: Home");
//   int t=EEPROM.get(8,password3);
// lcd.print(t);
  
  lcd.setCursor(userInput.length(), 1);
  
  
  while (true) {
    char key = keypad.getKey();

    if (key == '#') {
      break;
    } else if (key == '*') {
      if (userInput.length() > 0) {
        userInput.remove(userInput.length() - 1);
        lcd.setCursor(userInput.length(), 1);
        lcd.print(" ");  // Clear the character
        lcd.setCursor(userInput.length(), 1);
      }
    }
    else if (key == 'A') {
      lcd.clear();
      password();
    
    } 
    else if (key == 'D') {
      lcd.clear();
      while(1){
          menu();
      }
    } 
     else if (key) {
      userInput += key;
      lcd.print('*');
    }
  }
  EEPROM.get(8, password3);
 //check the password
if(password3==userInput.toInt()){
  lcd.clear();
  //change the oil price
  getUserInput();
  k=userInput.toInt();
  oPrice = nPrice;//get temp varible
  nPrice = k;//price change to new user input
  EEPROM.put(0,nPrice);//store the new price
  EEPROM.put(4,oPrice);//store the old price

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("New price for 1l->");
  lcd.setCursor(0,1);
  lcd.print(nPrice);    
  lcd.setCursor(0,2);
  lcd.print("Old price for 1l->");
  lcd.setCursor(0,3);
  lcd.print(oPrice);
  delay(1200);
  lcd.clear();
  return 1;
}
  
 else{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Wrong Password..!!");
  delay(1200);
  return 0;
 }
}

int password(){

  userInput = "";
  lcd.setCursor(0, 0);
  lcd.print("Enter the password->");//get the password
  
  lcd.setCursor(0, 2);
  lcd.print("Press D: Home");
  
  lcd.setCursor(userInput.length(), 1);
 while (true) {
    char key = keypad.getKey();

    if (key == '#') {
      break;
    } else if (key == '*') {
      if (userInput.length() > 0) {
        userInput.remove(userInput.length() - 1);
        lcd.setCursor(userInput.length(), 1);
        lcd.print(" ");  // Clear the character
        lcd.setCursor(userInput.length(), 1);
      }
    }
    else if (key == 'D') {
      lcd.clear();
      while(1){
      menu();
      }
    } 
     else if (key) {
      userInput += key;
      lcd.print('*');
    }
  }
  EEPROM.get(8, password3);
  if(password3==userInput.toInt() || password2==userInput.toInt() ){
  lcd.clear();
  userInput = "";
  lcd.setCursor(0, 0);
  lcd.print("new password->");//get the password
  
  lcd.setCursor(0, 2);
  lcd.print("Press D: Home");
  
  lcd.setCursor(userInput.length(), 1);
  while (true) {
    char key = keypad.getKey();

    if (key == '#') {
      break;
    } else if (key == '*') {
      if (userInput.length() > 0) {
        userInput.remove(userInput.length() - 1);
        lcd.setCursor(userInput.length(), 1);
        lcd.print(" ");  // Clear the character
        lcd.setCursor(userInput.length(), 1);
      }
    }
    else if (key == 'D') {
      lcd.clear();
      while(1){
      menu();
      }
    } 
     else if (key) {
      userInput += key;
      lcd.print('*');
    }
  }
  k=userInput.toInt();
  password3=k;
  EEPROM.put(8,password3);
  while(1){
    menu();
  }
  
  }
}


void getUserInput() {
  userInput = "";
  lcd.setCursor(0, 0);
  lcd.print("Change price Rs(1l)");
  lcd.setCursor(0, 2);
  lcd.print("Press D: Home");
  lcd.setCursor(userInput.length(), 1);
  
  
  while (true) {
    char key = keypad.getKey();

    if (key == '#') {
      break;
    } else if (key == '*') {
      if (userInput.length() > 0) {
        userInput.remove(userInput.length() - 1);
        lcd.setCursor(userInput.length(), 1);
        lcd.print(" ");  // Clear the character
        lcd.setCursor(userInput.length(), 1);
      }
    }else if (key == 'D') {
      lcd.clear();
      //return by function to the main display
      while(1){
          menu();
      }
    }
     else if (key) {
      userInput += key;
      lcd.print(key);
    }
  }

  lcd.clear();
  // lcd.print("new price: ");
  // lcd.print(userInput);
  
}

//motor_down
int motor_down(){
  while(1){
  if(digitalRead(39)==HIGH){
    mystepper.step(STEPS_PER_REV);
  }
  else{
      digitalWrite(45,LOW);
      digitalWrite(47,LOW);
      digitalWrite(49,LOW);
      digitalWrite(51,LOW);
      return 1;
  }
}
}

//motor_up
int motor_up(){
  while(1){
    if(digitalRead(40)==HIGH){
      mystepper.step(-STEPS_PER_REV);
    }
    else{
      digitalWrite(45,LOW);
      digitalWrite(47,LOW);
      digitalWrite(49,LOW);
      digitalWrite(51,LOW);

      break;
    }
  }
}