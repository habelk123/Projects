#include <Arduino.h>
#include <Ticker.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Bounce2.h>


#define LED_PIN 18 //external led output
#define BUTTON_PIN 19 //external user button input 
#define RESET_BUTTON_PIN 15 //external reset button input
//Screen specs:
#define SCREEN_W 128
#define SCREEN_H 64
#define OLED_RESET -1
#define SCREEN_ADD 0x3C

//Class Objects:
Adafruit_SSD1306 screen(SCREEN_W, SCREEN_H, &Wire, OLED_RESET);
Ticker ledTicker;
Ticker Time;
Bounce user_button = Bounce();
Bounce reset_button = Bounce();

//State Function Prototypes:
void idle(void);
void measure(void);
void wait(void);
void initializeSM(void);

//State Types:
typedef enum {IDLE = 0, WAIT, MEASURE} State_Type;

//Array of Function Pointers:
static void (*state_table[])(void) = {idle, wait, measure};


//Globals:
static State_Type curr_state;
volatile bool ledState = false;
int state = 0;
int rand_timer = 0;
int wait_count = 0;
int measure_count = 0;
int best = INT_MAX;

//Utilities/Callback Functions:
void toggleLED(){
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
}

void waitTimeIncrement(){
    wait_count++;
}

void measureTimeIncrement(){
    measure_count++;
}

//Button State Function:
void SetButtonState(){
    if(state != 1){
        state = 1;
    }
    else{
        state = 2;
    }
}

//Reset Button State Function:
void ResetButtonState(){
    curr_state = IDLE;
    screen.clearDisplay();
    screen.setCursor(5, 30);
    screen.println("Reaction Time Test");
    screen.display();
    //Detach all tickers
    Time.detach();
    ledTicker.detach();
    //reset user button state to initial as well as all time-tracking variables
    state = 0;
    rand_timer = 0;
    measure_count = 0;
    wait_count = 0;
    best = INT_MAX; //reset best score

    ledTicker.attach(0.5, toggleLED); //flash initial led sequence

}

//State Function Implementations:
void initializeSM(void){
    curr_state = IDLE;
    srand(time(0)); //seeding random number generator
}

void idle(void){ //idle state - flash led sequence. On transition toggle the led off for a random duration between 1-5 seconds
    if(state == 1){
        curr_state = WAIT;
        ledTicker.detach();
        digitalWrite(LED_PIN, LOW);
        rand_timer = (rand() % 5) + 1;
        Time.attach(1, waitTimeIncrement);
    }
}

void wait(void){ //wait state - led off until randomized wait duration is met. On transition, toggle led on and begin tracking user reaction time
    if(wait_count == rand_timer){
        curr_state = MEASURE;
        Time.detach();
        digitalWrite(LED_PIN, HIGH);
        Time.attach_ms(1, measureTimeIncrement);
        wait_count = 0;
        rand_timer = 0;
    }
    if(state == 2){ //if user tries to press the button before the led turns on, reset back to initial state (cheating condition)
        curr_state = IDLE;
        Time.detach();
        ledTicker.attach(0.5, toggleLED);
        rand_timer = 0;
        wait_count = 0;
    }
}

void measure(void){// measure state - display the user reaction time and their best reaction time from subsequent tries. On transition return to idle state
    if(state == 2){
        curr_state = IDLE;
        Time.detach();
        if(measure_count == 0){ //if measured time is 0, this isn't humanly possible and could just be up to intution, hence return to idle without recording (ensures validity of reaction time)
            ledTicker.attach(0.5, toggleLED);
        }

        else{ //display to oled screen
            char text1Buffer[50];
            char text2Buffer[50];
            if(measure_count < best){best = measure_count;}
            sprintf(text1Buffer, "Measured Time: %.3f", (float) measure_count / 1000);
            sprintf(text2Buffer, "Best Time: %.3f", (float) best / 1000);
            
            screen.clearDisplay();
            screen.setTextSize(0);
            screen.setCursor(5, 30);
            screen.println(text1Buffer);
            screen.setCursor(5, 50);
            screen.println(text2Buffer);
            screen.display();

            measure_count = 0; //reset the measured time for next attempt
            ledTicker.attach(0.5, toggleLED);
        }
    }
}

//Main:
void setup(){
    screen.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADD);
    screen.setTextColor(SSD1306_WHITE);
    //Startup message
    screen.clearDisplay();
    screen.setCursor(5, 30);
    screen.println("Reaction Time Test");
    screen.display();
    //configure pins for outputs and inputs (with debouncing)
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    user_button.attach(BUTTON_PIN);
    reset_button.attach(RESET_BUTTON_PIN);
    //specify debounce intervals as 50 ms
    reset_button.interval(50); 
    user_button.interval(50);

    ledTicker.attach(0.5, toggleLED); //idle led sequence
}

void loop(){
    user_button.update(); 
    reset_button.update();

    if(user_button.fell()){ //check on button press 
        SetButtonState();
    }
    if(reset_button.fell()){ //check on button press
        ResetButtonState();
    }
    state_table[curr_state](); //iterate through state table based on current state index
}