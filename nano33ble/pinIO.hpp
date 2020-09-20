// pin modes from Arduino.h
#ifndef _WIRING_CONSTANTS_
#define INPUT           (0x0)
#define OUTPUT          (0x1)
#define INPUT_PULLUP    (0x2)
#define INPUT_PULLDOWN  (0x3)
#endif
// some extra pin modes
#define INPUT_ANALOG    (0x4)
#define OUTPUT_ANALOG   (0x5)
#define IO_NONE        (0xff)

#ifndef BLUEFRUIT_H_
#define LED_RED LED_BUILTIN
#endif

const uint8_t ANALOG_TO_PIN[8] = {PIN_A0,PIN_A1,PIN_A2,PIN_A3,PIN_A4,PIN_A5,PIN_A6,PIN_A7};
bool isReserved(byte pin);

#define NREFS 6		//analog reference voltages
enum class refADC {
    v12,
    v18,
    v24,
    v30,
    v36,
    vVDD
};

uint16_t volt2anaDat(float volt , uint16_t scale);

// binary InputOutput bits
class pinIO {
  public:
    static byte nPins;
    // constructs all bits
    static void createDigBits(byte nBits);
    static bool isDigital(byte pin);
    // assert mode to one of arduino(+) pin modes; returns success
    static bool setMode(byte iBit, byte mode){ 
      if (iBit<nPins) return pins[iBit].setMode(mode); 
      else Serial.println("bad bitIdx"); return false; };
    // set value of bit
    static void setState(byte iBit, bool bitv){ 
      if (iBit<nPins) pins[iBit].setState(bitv); };
    // get value of bit
    static bool getState(byte iBit) { return pins[iBit].bitv; };
    static bool acceptBLEdig(uint8_t * bledat, uint8_t len);
    static word produceBLEdig(uint8_t * digdat, uint8_t len);
    pinIO() {pin=0; mode=0; bitv=0; }; //nPins=0;};
    pinIO(byte pin) : pinIO() { this->pin=pin; };
  protected:
    bool bitv; // state value of the pin
    byte mode;
    static pinIO * pins;
    bool setMode(byte mode);
    byte pin;
  private:
    void setState(bool bitv);
};

// analog channels 
class anaIO : public pinIO {
	public:
		anaIO() : pinIO() { anaval=0; };
		static byte nChans;
		// constructs all analog channels inactive
		static void createAnaPins(byte nPins);
		static bool isAnalog(byte pin);
		// assert channel to pinMode defined above ; return success
		static bool setMode(byte chIdx, byte mode) {
			if (chIdx<nChans) return anachans[chIdx].setMode(mode); return false; };
		static bool setReference(byte chIdx, word mVref ) {
			if (chIdx<nChans) return anachans[chIdx].setReference( mVref );
			return false;
		}
		static byte getMode(byte chIdx) { return anachans[chIdx].mode; };
		static uint16_t getAnaState(byte chIdx) {if (chIdx<nChans) return anachans[chIdx].getAnaState(); };
		static uint16_t produceBLEana(byte chIdx, uint16_t scale);
		static float maxVoltRange(byte chIdx) { return anachans[chIdx].maxVoltRange(); };
    
		// get reference voltage which is also max
		float maxVoltRange();
		// set reference voltage
		bool setReference(word mVref );
		// set ADC resolution 
		void setResolution(byte nbits);
		// get reading in volt 
		float getVoltage();
	protected:
		uint16_t anaval;
		refADC reference;
		byte resolution;
	private:  
		static anaIO * anachans;
		bool setMode(byte mode);
		uint16_t getAnaState();
		void setAnaState(uint16_t value);
};
