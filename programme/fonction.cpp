#include "fonction.h"

void effacePoids(Adafruit_PCD8544& lcd) {
  //poids
  char efface = 0x00;
  lcd.setCursor(2, 29);
  lcd.setTextSize(2);
  lcd.setTextColor(BLACK, WHITE);
  for (int i = 0; i < 5; i++) {
    lcd.write(efface);
  }
  lcd.setTextSize(1);
  lcd.setCursor(60,29);
  lcd.print(F(" Kg"));
  lcd.display();
}

void effaceMem(Adafruit_PCD8544& lcd){
  //poids
  char efface = 0x00;
  lcd.setCursor(35, 2);
  lcd.setTextSize(1);
  lcd.setTextColor(BLACK, WHITE);
  for (int i = 0; i < 5; i++) {
    lcd.write(efface);
  }
  lcd.setCursor(65,2);
  lcd.print("kg");
  lcd.display();
}
