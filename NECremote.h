//NECremote.h
//started 7/6/14
//
//collects IR signal from NEC remote controls 
//and processes it into HEX codes
#ifndef NECremote_h
#define NECremote_h

class NECremote
{
	public:
		NECremote();
		unsigned long result;
		void enableReceiver(int pin);
		bool decodeResults();
		void clearAndResume();
};

// information for the interrupt service routine
typedef struct 
{
	int IRpin;
	bool receiveState;
	unsigned int rawBuffer[768];
	int bufferPosition;
} 
ISRvars_t;

extern volatile ISRvars_t ISRvars;

#endif
