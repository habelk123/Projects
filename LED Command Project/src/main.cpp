#include <Arduino.h>
#include<Ticker.h>
#include <stdio.h>

//IDENTIFIERS FOR LEDS AND MODES
enum ledId {RED = 0, BLUE, YELLOW, GREEN, ALL, LED_COUNT};
enum ledMode{ON = 0, OFF, PULSE, NONE};
//LED STATE VARIABLES AND GPIO PINS
ledMode led_modes[LED_COUNT] = {OFF, OFF, OFF, OFF, NONE};
const uint8_t led_pins[LED_COUNT] = {18, 19, 21, 22};
//TICKER OBJECT FOR PULSING + ARRAY
Ticker led_Tickers[LED_COUNT];
bool led_output_states[LED_COUNT] = {false, false, false, false};

//PARSING VARIABLES AND BUFFER FOR SERIAL COMMANDS
char cmd_buffer[64];
int buffer_iter = 0;
volatile bool cmd_ready = false;
volatile bool cmd_deliver = false;

//FUNCTION TO PARSE LED IDENTIFIERS
bool map_led(const char* token, ledId *out_led){
    if(strcmp(token, "RED") == 0)  *out_led = RED;
    else if(strcmp(token, "BLUE") == 0)  *out_led = BLUE;
    else if(strcmp(token, "YELLOW") == 0)  *out_led = YELLOW;
    else if(strcmp(token, "GREEN") == 0)  *out_led = GREEN;
    else if(strcmp(token, "ALL") == 0)  *out_led = ALL;
    else return false;

    return true;
}

//FUNCTION TO PARSE LED MODES
bool map_mode(const char* token, ledMode* out_mode){
    if(strcmp(token, "ON") == 0)  *out_mode = ON;
    else if(strcmp(token, "OFF") == 0)  *out_mode = OFF;
    else if(strcmp(token, "PULSE") == 0)  *out_mode = PULSE;
    else return false;

    return true;
}
//FUNCTION TO TOGGLE LED STATE
void toggleLED(int led){
    led_output_states[led] = !led_output_states[led];
    digitalWrite(led_pins[led], led_output_states[led]);
}
//FUNCTION TO PARSE SERIAL COMMANDS + RESPOND VIA SERIAL MONITOR
void parseCommand(char* command){
    char id_token[8];
    char mode_token[8];
    int i = 0;
    while(command[i] != '\0'){
        command[i] = (char)toupper((unsigned char)command[i]);
        i++;
    }
    int parsed_token = sscanf(command, "%7s %7s", id_token, mode_token);
    if(parsed_token == 2){
        ledId led;
        ledMode mode;
        if(map_led(id_token, &led) && map_mode(mode_token, &mode)){
            led_modes[led] = mode;
            Serial.printf("OK: %s %s\r\n", id_token, mode_token);
        }
        else{
            Serial.printf("ERR: Invalid LED or Mode\r\n");
        }
    }
    else{
        Serial.printf("ERR: Invalid Command Format\r\n");
    }
}


void setup(){
    Serial.begin(115200);
    for (int i = 0; i < LED_COUNT; i++) { //INITIALIZE LED PINS
        pinMode(led_pins[i], OUTPUT);
        digitalWrite(led_pins[i], LOW);
    }
    Serial.println("READY");
}

void loop(){
    //CHECK FOR SERIAL INPUT
    if(Serial.available() && buffer_iter < 63){
        char c = (char)Serial.read();
        if(c == '\r'){
            return;
        }
        else if(c == '\n'){
            cmd_buffer[buffer_iter] = '\0';
            buffer_iter = 0;
            cmd_ready = true; //COMMAND RECIEVED AND READY TO PARSE
        }
        else{
            cmd_buffer[buffer_iter++] = c;
        }
    }
    if(cmd_ready){
        parseCommand(cmd_buffer); //PARSE COMMAND FOR HARDWARE DELIVERY
        cmd_ready = false;
        cmd_deliver = true;
    }
    if(cmd_deliver){
        // Update LEDs according to led_modes array
        if(led_modes[ALL] != NONE){ //IF "ALL" WAS SET, SET TO ALL LEDS
            for(int j = 0; j < LED_COUNT; j++){
                led_modes[j] = led_modes[ALL];
            }
            led_modes[ALL] = NONE; //RESET "ALL" MODE
        }
        for(int i = 0; i < LED_COUNT; i++){//APPLY MODES TO EACH LED
            switch(led_modes[i]){
                case ON:
                    led_Tickers[i].detach();
                    digitalWrite(led_pins[i], HIGH);
                    led_output_states[i] = true; //SET SPECIFC LED HIGH FOR STATE TRACKING
                    break;
                case OFF:
                    led_Tickers[i].detach();
                    digitalWrite(led_pins[i], LOW);
                    led_output_states[i] = false;//SET SPECIFC LED LOW FOR STATE TRACKING
                    break;
                case PULSE:
                    if(!led_Tickers[i].active()){ //IF NOT PULSING, START PULSING
                        led_Tickers[i].attach(0.25, toggleLED, i); //TICKER CALLBACK TO TOGGLE LED
                    }
                    break;
            }
        }

        cmd_deliver = false;//RESET CONTROL FLAG FOR NEXT COMMAND
    }
}
