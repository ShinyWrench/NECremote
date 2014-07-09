//#include <avr/io.h>
//#include <avr/interrupt.h>
#include "Arduino.h"
#include "NECremote.h"

volatile ISRvars_t ISRvars;

NECremote::NECremote()
{  
    ISRvars.receiveState = 0;
    ISRvars.bufferPosition = 0;
	result = 0;	
}

//assigns IR pin and starts data collection with timer interrupt 1
void NECremote::enableReceiver(int pin)
{
	pinMode(pin, INPUT);
	ISRvars.IRpin = pin;
	
	//configure and begin interrupt
	cli();                    //close interrupts
 	TCCR1A = 0;               //clear timer register A
 	TCCR1B = 0;               //clear timer register B
	OCR1A = 1599;             //16 * 100 microseconds - 1
  	TCCR1B |= (1 << WGM12);   //set compare interrupt in register
  	TCCR1B |= (1 << CS10);    //no prescaler
  	TIMSK1 |= (1 << OCIE1A);  //set compare interrupt mask
	sei();                    //begin interrupts
}

//interrupt service routine to collect raw data
ISR(TIMER1_COMPA_vect)
{  	
	//read state of infrared sensor
    int IRdata = digitalRead(ISRvars.IRpin);

    //if pulse is detected, prepare to load buffer
	if (IRdata == 0 && ISRvars.receiveState != 2) 
	{
		ISRvars.receiveState = 1;
	}

	//load buffer if it still has room
    if (ISRvars.receiveState == 1)
    {
    	ISRvars.rawBuffer[ISRvars.bufferPosition] = IRdata;
    	ISRvars.bufferPosition = ISRvars.bufferPosition + 1;
    }    

  	//check if buffer is full 
	if (ISRvars.bufferPosition == 768) 
	{
		ISRvars.receiveState = 2;	
	}
}

//decodes buffer data and stores code in "result"
//returns 1 if meaningful data found, 0 if not
bool NECremote::decodeResults()
{
	Serial.print("checking receive state for decode... receive state: ");
	Serial.println(ISRvars.receiveState);
	Serial.print("buffer position: ");
	Serial.println(ISRvars.bufferPosition);
	
	//make sure buffer is full of new data
	if(ISRvars.receiveState == 2)	
	{
	
		Serial.println("decoding results");
	
		for(int i = 0; i < 768; i++)
		{
			Serial.print(ISRvars.rawBuffer[i]);
			if ((i + 1) % 32 == 0) Serial.println();
		}
	
		//index at start of buffer
		int bufferIndex = 0;
	
		//pulses are read as 0's, gaps are read as 1's
	
		//9.2 millisecond pulse at start of signal...
		while(ISRvars.rawBuffer[bufferIndex] == 0) {bufferIndex++;}
    
		//followed by 4.6 millisecond gap
		while(ISRvars.rawBuffer[bufferIndex] == 1) {bufferIndex++;}

 		//start of encoding begings with pulse
		int currentState = 0;

		//reset pulse width and gap width 
		int pulseWidth = 0;
 	    int gapWidth = 0;

 	   //process buffer to find pulses and gaps for encoding
 		for(int i = bufferIndex; i < 768; i++)
		{
   		 	if (ISRvars.rawBuffer[i] == 0 && currentState == 0) pulseWidth++;     	  //pulse continues
				else if (ISRvars.rawBuffer[i] == 1 && currentState == 1) gapWidth++;  //gap continues
  		  		else if (ISRvars.rawBuffer[i] == 1 && currentState == 0)              //gap begins
				{
 	    			currentState = 1;
  		   			gapWidth = 1;
				}
	    		else													     //pulse begins, previous gap is measured
	    		{
	       			currentState = 0;
	     			pulseWidth = 1;
	     			if(gapWidth < 10) result = (result << 1) + 0;  	         //short gap: bitshift 0 into result
	      				else if (gapWidth < 20) result = (result << 1) + 1;  //long gap: bitshift 1 into result
	  			}
 		}
  
		//return 1 if meaningful data acquired, otherwise return 0 
	   	if (result >= 0xFF0000 && result <= 0xFFFFFF) return 1;
	   		else return 0;
	}
	else return 0;
}

//resets result, entire buffer, index and receive state
void NECremote::clearAndResume()
{
	result = 0;
	for (int i = 0; i < 768; i++) ISRvars.rawBuffer[i] = 1;
    ISRvars.bufferPosition = 0;
	ISRvars.receiveState = 0;
    Serial.println("buffer is cleared");
}
