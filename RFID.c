#define BOARD "DE10-Standard"
#define HEX3_HEX0_BASE 0xFF200020 
#define SW_BASE 0xFF200040 
#define KEY_BASE 0xFF200050
#define TIMER_BASE 0xFF202000

volatile int *feedback = (int *)SW_BASE;						//RFID signal and door angle feedback
volatile int *status = (int *)HEX3_HEX0_BASE;						//lock and alarm status display
volatile int *keypad = (int *)KEY_BASE;							
volatile int *timer = (int *)TIMER_BASE;

int lookuptable[] = {0x38, 0x3E, 0x77, 0x0};						//(L)ocked, (U)nlocked, (A)larm, and blank on 7-segment display respectively 
int mechanical =  0;									//solenoid lock
int electrical = 0; 									//LED indicator
int alarm = 3;
int press = 0;
int correct = 0;
int attempt = 0;


void verify_pin(){									//checks if the PIN has been entered correctly (PIN = 0123)
	if ((press == 0) && (*keypad & 0xF)){						//checks if any button has been pressed
		if (*keypad & 0x1){correct = 1;}					
		press++;		
		while(*keypad & 0xF);							//keep looking until button is released
	}
	if ((press == 1) && (*keypad & 0xF)){
		if (*keypad & 0xD){correct = 0;}	
		press++;
		while(*keypad & 0xF);
	}
	if ((press == 2) && (*keypad & 0xF)){
		if (*keypad & 0xB){correct = 0;}
		press++;
		while(*keypad & 0xF);
	}
	if ((press == 3) && (*keypad & 0xF)){
		if ((*keypad & 0x8) && (correct == 1)){					//correct pin entered, unlock door
			mechanical = 1;
			electrical = 1;
		}
		else {									//incorrect pin entered, increment attempts
			attempt++;
			if (attempt == 2){						//sound alarm after 2nd PIN attempt (only 2 attempts for demonstration)
				alarm = 2;
				*(timer+1) |= 0x4;					//start timer
				*(timer+1) &= 0x7;
			}					
		}
		press = 0;
		while(*keypad & 0xF);
	}
}


void auto_lock(){									//locks door after it has been closed for 5 seconds
	if (*(timer) & 0x1){
		mechanical = 0;
		electrical = 0;
		*timer &= 0x2;
		*(timer+1) |= 0x8;							//stop timer
		*(timer+1) &= 0xB;
	}
}


void lockdown(){									//allow new attempts after 5 seconds (short time for demonstration)
	if (*(timer) & 0x1){											
		alarm = 3;
		attempt = 0;
		*timer &= 0x2;	
		*(timer+1) |= 0x8;							
		*(timer+1) &= 0xB;
	}
}


int main(){										
	*(timer+1) |= 0x8;								//create a 5 second timer
	*(timer+2) = 0x6500;
	*(timer+3) = 0x1DCD;

	while(1){
		*status = lookuptable[alarm] * 0x10000 + lookuptable[electrical] * 0x100 + lookuptable[mechanical]; 	//display status on 7-segment display		

		if (alarm == 2){lockdown();}
		else {
			auto_lock();
			verify_pin();
			if ((*feedback & 0x8) && (*feedback & 0x2)){						//verify RFID signal 
				mechanical = 1;
				electrical = 1;
				press = 0;						//reset PIN entry in case RFID key was used mid-entry
			}
			
			if (mechanical == 1){						//start timer once door is unlocked
				*(timer+1) |= 0x4;
				*(timer+1) &= 0x7;
			}
			if ((*feedback & 0x16) && (mechanical == 1)){			//reset timer when door is open
				*(timer+2) = 0x6500;
				*(timer+3) = 0x1DCD;
			}
		}
	}
}


