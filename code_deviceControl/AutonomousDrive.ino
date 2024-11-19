#include <SoftwareSerial.h>
#define BT_RXD A1                                 //A1 pin : 블루투스 통신 RX
#define BT_TXD A0                                 //A0 pin : 블루투스 통신 TX
#define AUTO 25
#define PASSIVE 26
//#define SAFE_DIST 20                              // 안전거리 상수 정의(단위 : cm)
#define TURN_LEFT 1
#define TURN_RIGHT 0


// "A1 pin : RX, A0 pin : TX" 인 시리얼 통신 설정
//// bluetooth 객체를 통해 bluetooth module 과 데이터 통신 수행
//// 스마트폰(master) - bluetooth module(slave)
SoftwareSerial bluetooth(BT_RXD,BT_TXD);

// ========== pin 관련 설정 ==========
// 초음파 센서 pin
//// *trig* : 초음파를 발사하는 신호를 보내는 pin (output)
//// *echo* : 외부 물체로부터 반사된 초음파 신호를 감지하는 pin (input)
const int LtrigPin = 13;                  // PB5 : 디지털 13번 핀에 연결
const int LechoPin = 12;                  // PB4 : 디지털 12번 핀에 연결

const int FtrigPin = 11;                  // PB3 : 디지털 11번 핀에 연결
const int FechoPin = 10;                  // PB2 : 디지털 10번 핀에 연결

const int RtrigPin = 9;                   // PB1 : 디지털 9번 핀에 연결
const int RechoPin = 8;                   // PB0 : 디지털 8번 핀에 연결

// Right 모터 제어 pin
const int RightMotor_forward_pin = 7;            // PD7 : 오른쪽 모터 제어선 IN1
const int RightMotor_backward_pin = 2;           // PD2 : 오른쪽 모터 제어선 IN2
const int RightMotor_E_pin = 5;                  // PD5 : 오른쪽 모터의 Enable ENA

// Left 모터 제어 pin
const int LeftMotor_forward_pin = 3;            // PD3 : 왼쪽 모터 제어선 IN3
const int LeftMotor_backward_pin = 4;           // PD4 : 왼쪽 모터 제어선 IN4
const int LeftMotor_E_pin = 6;                  // PD6 : 왼쪽 모터의 Enable ENB
// ========== pin 관련 설정 ==========

// ========== 차량 제어 관련 Var 정의 ==========
// bluetooth 송/수신 pin
int bluetoothTx = A1;                // 송신 pin                   
int bluetoothRx = A0;                // 수신 pin

//좌우 모터 속도 조절, 설정 가능 최대 속도 : 255
int L_MotorSpeed = 255;         // 왼쪽 모터 속도(default : 255(max))
int R_MotorSpeed = 255;         // 오른쪽 모터 속도(default : 255(max))
int mode = PASSIVE;             // 주행모드(default : PASSIVE)
// ========== 차량 제어 관련 Var 정의 ==========


// setup code to run once: 
void setup() 
{ 
    // echo pin : 'input'으로 설정
    pinMode(LechoPin, INPUT);                   // left
    pinMode(FechoPin, INPUT);                   // forward
    pinMode(RechoPin, INPUT);                   // right

    // trig pin : 'output' 으로 설정
    pinMode(LtrigPin, OUTPUT);                  // left
    pinMode(FtrigPin, OUTPUT);                  // forward
    pinMode(RtrigPin, OUTPUT);                  // right

    // Motor 제어 pin : 'output' 으로 설정
    pinMode(RightMotor_E_pin, OUTPUT);          // PWM signal 출력 -> motor 속도 제어        
    pinMode(RightMotor_forward_pin, OUTPUT);    
    pinMode(RightMotor_backward_pin, OUTPUT);
    pinMode(LeftMotor_E_pin, OUTPUT);           // PWM signal 출력 -> motor 속도 제어
    pinMode(LeftMotor_forward_pin, OUTPUT);
    pinMode(LeftMotor_backward_pin, OUTPUT);

    Serial.begin(115200);                       // 아두이노 - PC 간의 시리얼 통신 115200bps로 설정
    bluetooth.begin(9600);                      // 아두이노 - bluetooth module 간의 시리얼 통신 9600bps로 설정
    Serial.println("A+duino !!");
}

// main code to run repeatedly:
void loop() 
{
    // bluetooth 통신을 통해 (mobile app으로부터)수신받은 data 저장
    char command;   

    // bluetooth 통신을 통해, mobile app으로부터 명령 입력받기
    if(bluetooth.available())
    {
        command = bluetooth.read();
        if(!(command >= 32 && command <= 126))         // 유효한 ASCII 문자 여부 확인
        {
        Serial.println("Non-printable character received");
        stop_motors();
        }
    }
    else{;}

    // 주행 모드 설정
    if(mode == PASSIVE)
    {
        passive_mode(command); 
    }
    else if(mode==AUTO)
    {
        auto_mode(command); 
    }    
}

// 자율주행 mode 
//// 우회전 : 후진 -> 오른쪽 모터 정지 -> 왼쪽 모터만 전진
//// 좌회전 : 후진 -> 왼쪽 모터 정지 -> 오른쪽 모터만 전진
void auto_mode(char command)
{
    // foward 초음파 센서로부터 측정된 외부 장애물과의 거리
    float Ultra_df = FUltrasonic();
    //회전방향(0 : 우회전, 1 : 좌회전) 
    int turn_direction = 0;                   

    // case : 전진 
    if(Ultra_df >= 250) 
    {
        forward();        
    }	
    
    // case : 외부 장애물과의 거리가 25.0cm 미만 
    else 
    {
    
    //// case1 : 외부 장애물과의 거리 < 15.0cm  
    if (Ultra_df < 150) 
    {
        Serial.println("Ultra_df < 150cm ");
        
        //// case1-1. 후진
        backward();               
        delay(1000);
        
        //// case1-2. 좌/우 DC 모터 정지
        stop_motors();
        delay(200);
    }

    //// case2 : 15.0cm <= 외부 장애물과의 거리 < 25.0cm
    else 
    {
        //// case2-1. 좌/우 DC 모터 정지
        stop_motors();
        delay(200);
        
        //// case2-2. 우회전/좌회전 결정
        Serial.println("150cm <= Ultra_df < 250cm ");
        turn_direction =  Servo_con();
        
        //// 우회전
        if (turn_direction == 0) 
        {
            Serial.println("우회전.");
            turn(TURN_RIGHT);
        }
        
        //// 좌회전
        else if (turn_direction == 1) 
        {
            Serial.println("좌회전.");
            turn(TURN_LEFT);
        }
    }
    }

    //수동 전환
    if (command == 'P')
    { 
        Serial.println("PASSIVE SWITCH");
        mode = PASSIVE;

        stop_motors();
        delay(500);
    }
}

// 수동 주행 모드
void passive_mode(char command)
{
    switch(command)
    {
        case 'F' :
            forward();
            break;
            
        case 'B' :
            backward();
            break;
            
        case 'L' :
            turn(TURN_LEFT);
            break;
            
        case 'R' :
            turn(TURN_RIGHT );
            break;
            
        case 'S' :
            stop_motors();
            break;
            
        case 'A' :
            Serial.println("AUTO SWITCH");
            mode=AUTO;   
            break;
            
        default : 
            Serial.println("Invalid command received");
            stop_motors();
            break;   
    }
}
    

// DC motor 제어 -> 좌/우회전, 전진/후진 동작 구현
void motor_role(int R_motor, int L_motor)
{
    // RightMotor 방향 제어(전진 || 후진)
    digitalWrite(RightMotor_forward_pin, R_motor);     
    digitalWrite(RightMotor_backward_pin, !R_motor);   

    // LeftMotor 방향 제어(전진 || 후진)
    digitalWrite(LeftMotor_forward_pin, L_motor);      
    digitalWrite(LeftMotor_backward_pin, !L_motor); 

    // RightMotor, LeftMotor 속도 제어 
    analogWrite(RightMotor_E_pin, R_MotorSpeed);  
    analogWrite(LeftMotor_E_pin, L_MotorSpeed);   
}

// Servo 모터 제어 
//// => 회전방향 설정
int Servo_con() 
{
    float Ultra_dl = LUltrasonic();
    float Ultra_dr = RUltrasonic();
    return (Ultra_dl > Ultra_dr ) ? 1 : 0;  // 1: 좌회전, 0: 우회전
}

//앞쪽 센서 길이 측정
//// 거리단위 : mm
int FUltrasonic()   
{ 
    return measure_distance(FtrigPin, FechoPin);
}

//왼쪽 센서 길이 측정
int LUltrasonic()   
{ 
    return measure_distance(LtrigPin, LechoPin);
}

//오른쪽 센서 길이 측정
int RUltrasonic()   
{
    return measure_distance(RtrigPin, RechoPin);
}


// 거리 측정 함수 
float measure_distance(int trigPin, int echoPin) 
{  
    // trigPin에서 초음파 발생(echoPin도 HIGH)
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);

    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH);
    return (duration * 0.034 / 2);  // cm 단위로 반환
}


// 모터 정지 함수
void stop_motors() 
{
    analogWrite(RightMotor_E_pin, 0);
    analogWrite(LeftMotor_E_pin, 0);
}

// 후진 함수
void forward() 
{
    motor_role(HIGH, HIGH);
}

// 후진 함수
void backward() 
{
    motor_role(LOW, LOW);
}

// 회전 함수
//// 우회전(0) : 정지 -> 후진 -> 오른쪽 모터 전진, 왼쪽 모터 후진
//// 좌회전(1) : 정지 -> 후진 -> 오른쪽 모터 후진, 왼쪽 모터 전진
void turn(const int direction) 
{
    stop_motors();
    delay(500);

    backward();
    delay(500);

    motor_role(direction == 0 ? HIGH : LOW, direction == 0 ? LOW : HIGH);
    delay(800);
}