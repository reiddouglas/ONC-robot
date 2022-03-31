#pragma config(I2C_Usage, I2C1, i2cSensors)
#pragma config(Sensor, in7,    infraSensor,    sensorReflection)
#pragma config(Sensor, dgtl2,  button,         sensorTouch)
#pragma config(Sensor, dgtl3,  RedLED,         sensorDigitalOut)
#pragma config(Sensor, dgtl7,  RedLED2,        sensorDigitalOut)
#pragma config(Sensor, dgtl10, sonarSensor,    sensorSONAR_cm)
#pragma config(Sensor, I2C_1,  encoder,        sensorQuadEncoderOnI2CPort,    , AutoAssign )
#pragma config(Motor,  port1,           motorLeft,     tmotorVex393_HBridge, openLoop, encoderPort, I2C_1)
#pragma config(Motor,  port2,           motorRight,    tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port3,           motorClaw,     tmotorVex269_MC29, openLoop)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

enum T_systemState{
	FIND_TARGET_CLOSE = 0,
	FIND_TARGET_SPIN,
	MOVE_NEW_AREA,
	PLACE_OBJECT
};

T_systemState systemState = FIND_TARGET_SPIN;

const int fullTurn = 1000;	//the encoder value for a whole 360 degree turn
const int infraSensorThreshhold = 3950;
const int wallThreshhold = 25;	//in cm
const int objectPlacementThreshhold = 7; //in cm
const int motorDefault = 50;
const int clawSpeed = 40;

const  int OFF = 0;
const  int ON  = 1;

void moveForwards(){
		motor[motorRight] = motorDefault;
		motor[motorLeft] = motorDefault*2;
}

void moveForwardsSlow(){
		motor[motorRight] = 35;
		motor[motorLeft] = 35;
}

void moveReverse(){
		motor[motorRight] = -motorDefault;
		motor[motorLeft] = -motorDefault*2;
}

void moveStop(){
	motor[motorRight] = 0;
	motor[motorLeft] = 0;
}

void turnLeft(){
		motor[motorRight] = motorDefault;
		motor[motorLeft] = -motorDefault;
}

bool wallCheck(){
		return !(SensorValue[sonarSensor] > wallThreshhold || SensorValue[sonarSensor] < 0);
}

bool wallCloseCheck(){
	return !(SensorValue[sonarSensor] > objectPlacementThreshhold || SensorValue[sonarSensor] < 0);
}

void turnRight(){
		motor[motorRight] = -motorDefault;
		motor[motorLeft] = motorDefault;
}

void turnSlightLeft(){
		motor[motorRight] = motorDefault;
		motor[motorLeft] = motorDefault-5;
}

void turnSlightRight(){
		motor[motorRight] = motorDefault-5;
		motor[motorLeft] = motorDefault;
}

void preChangeState(){
	moveStop();
	wait1Msec(500);
	resetMotorEncoder(motorLeft);
}


//Checks if target is within close range using infrared scanner
bool targetCheck(){
	clearTimer(T1);
	while(time1[T1]<150){
		if(wallCloseCheck() && SensorValue[infraSensor]<infraSensorThreshhold){
				return true;
		}
	}
	return false;
}

bool button_pushed;

void monitorInput(){
  if(SensorValue(button) && !button_pushed){
    button_pushed = true;
  }
}

/*
How spin works:

1. The robot begins turning left continuously

2. If the robot is too close to a wall, back up a bit and then begin spinning again

3. If the infra sensor is tripped, stop and begins approaching target

4. If the robot completes a 1.25 turn unprompted, robot will move to new area
*/

void spin(){
	while(!wallCheck()){
		turnLeft();


		if(SensorValue[infraSensor] < infraSensorThreshhold){
			preChangeState();
			systemState = FIND_TARGET_CLOSE;
			return;
		}


		if(getMotorEncoder(motorLeft) < -1500){
			preChangeState();
			systemState = MOVE_NEW_AREA;
			return;
	  }

	}

	moveReverse();
	resetMotorEncoder(motorLeft);
}

/*
How moveNewArea works:

1. The robot begins moving forward

2. If the robot is too close to a wall, back up a bit and then begin spin protocol

3. If the infra sensor is tripped, stop and begin approaching target

NOTE: The robot should always hit a wall in a square arena, hence there is no check on the motor encoder
*/

void moveNewArea(){
	while(!wallCheck()){
		moveForwards();
		if(SensorValue[infraSensor] < infraSensorThreshhold){
			preChangeState();
			systemState = FIND_TARGET_CLOSE;
			return;
		}
	}
	preChangeState();
	systemState = FIND_TARGET_SPIN;
	return;

	/*
	moveForwards();
	if(wallCheck()){
		moveReverse();
		wait1Msec(1000);
		preChangeState();
		systemState = FIND_TARGET_SPIN;
		return;
	}

	if(SensorValue[infraSensor] < infraSensorThreshhold){
		preChangeState();
		systemState = FIND_TARGET_CLOSE;
		return;
	}
	*/
}

/*
How findTarget works:

1. The robot continues to move forwards so long as the infra sensor is tripped within an interval (IR pulses at 10Hz or every 1/10 of a second)

2. If the IR sensor becomes untripped, spin

3. If the IR sensor is still tripped when the robot encounters a wall, does a check

4. The check will pass if the object is within a certain IR threshold

5. If the check passes, robot prepares to place object

6. If check fails, backup and spin
*/

void findTarget(){
	clearTimer(T1);
	while(time1[T1] < 300){
		moveForwardsSlow();

		if(SensorValue[infraSensor] < infraSensorThreshhold){
			clearTimer(T1);

		}
		if(targetCheck()){
			preChangeState();
			systemState = PLACE_OBJECT;
			return;
		}
	}
	preChangeState();
	systemState = FIND_TARGET_SPIN;
	return;
}

/*
How placeObject works:

1. SLOWLY inch forwards until within suitable range to place object

2. Activate placement mechanism

3. Back away from target slightly

4. Turn on LED to signal completion

*/


void placeObject(){

		int sonarSmall = 30;
		bool left = true;
		clearTimer(T1);
		while(time1[T1]<2000){
			if(left){
				turnLeft();
				if(SensorValue[sonarSensor] < sonarSmall){
					sonarSmall = SensorValue[sonarSensor];
				}else{
					left = false;
				}
			}else{
				turnRight();
				if(SensorValue[sonarSensor] < sonarSmall){
					sonarSmall = SensorValue[sonarSensor];
				}else{
					left = true;
				}
			}
		}


		moveStop();
		wait1Msec(1000);
		while(!wallCloseCheck()){
			moveForwardsSlow();
		}


		//Place object
		moveStop();
		motor[motorClaw] = clawSpeed;
		wait1Msec(250);
		motor[motorClaw] = 0;

		//back up
		moveReverse();
		wait1Msec(500);
		moveStop();

		//signal completion
		SensorValue(RedLED) = ON;
}

task main()
{
	button_pushed = false;
	SensorValue(RedLED) = OFF;
	resetMotorEncoder(motorLeft);
	resetMotorEncoder(motorRight);

	while (true){

		monitorInput();

		//tests only start when button is pushed

		if (button_pushed){

			SensorValue(RedLED) = OFF;

			switch(systemState){

				case(FIND_TARGET_SPIN):
					spin();
					break;

				case(MOVE_NEW_AREA):
					moveNewArea();
					break;

				case(FIND_TARGET_CLOSE):
					findTarget();
					break;

				case(PLACE_OBJECT):
					placeObject();
					return;
					break;

				default:
					break;
			}
		}
	}
}
