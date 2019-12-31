/*
MIT License

Copyright(c) 2019 Thomas Kim

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include <iostream>
#include <wiringPi.h>
#include <softPwm.h>

#include "Servo.h"
#include "../Util/FileLog.h"

#define DELAY_RELAY			100 
#define DELAY_PWM			10

namespace Hap {

	bool Servo::Init() {
		if (wiringPiSetupGpio() == -1) {
			return false;
		}
		pinMode(gpio_servo, PWM_OUTPUT);
		pinMode(gpio_relay, OUTPUT);

		return true;
	}

	uint8_t Servo::Attach(int value, int range) {
		digitalWrite(gpio_relay, HIGH);
		Log::Dbg("Servo::Attach, digitalWrite (relay=%d, HIGH)\n", gpio_relay);
		Wait(DELAY_RELAY);
		uint8_t ret = softPwmCreate(gpio_servo, value, range);
		Wait(DELAY_PWM);
		
		return ret;
	}

	void Servo::Detach() {
		softPwmStop(gpio_servo);
		Wait(DELAY_PWM);
		digitalWrite(gpio_relay, LOW);
		Log::Dbg("Servo::Detach, digitalWrite (relay=%d, LOW)\n", gpio_relay);
		Wait(DELAY_RELAY);
	}

	void Servo::Write(int value) {
		softPwmWrite(gpio_servo, value);
	}

	void Servo::Wait(int mili) {
		delay(mili);
		Log::Dbg("Servo::Wait(mili=%d)\n", mili);
	}
}
