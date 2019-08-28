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

const uint8_t ANALOG_TO_PIN[8] = {PIN_A0,PIN_A1,PIN_A2,PIN_A3,PIN_A4,PIN_A5,PIN_A6,PIN_A7};
bool isReserved(byte pin);

// binary InputOutput bits
class pinIO {
  public:
    bool bitv; // state value of the pin
    byte mode;
    static byte nPins;
    static void createDigBits(byte nBits);
    static bool isDigital(byte pin);
    static void setMode(byte bitIdx, byte mode){ 
      if (bitIdx<nPins) pins[bitIdx].setMode(mode); 
      else Serial.println("bad bitIdx"); };
    static void setState(byte bitIdx, bool bitv){ 
      if (bitIdx<nPins) pins[bitIdx].setState(bitv); };
    static bool getState(byte bitIdx) { return pins[bitIdx].bitv; };
    static bool acceptBLEdig(uint8_t * bledat, uint8_t len);
    static word produceBLEdig(uint8_t * digdat, uint8_t len);
    pinIO() {pin=0; mode=0; bitv=0; }; //nPins=0;};
    pinIO(byte pin) : pinIO() { this->pin=pin; };
  protected:
    static pinIO * pins;
    void setMode(byte mode);
    byte pin;
  private:
    void setState(bool bitv);
};

// analog channels 
class anaIO : public pinIO {
	public:
		word anaval;
		anaIO() : pinIO() { anaval=0; };
    static byte nChans;
		static void createAnaPins(byte nPins);
    static bool isAnalog(byte pin);
		static void setMode(byte chIdx, byte mode) {if (chIdx<nChans) anachan[chIdx].setMode(mode); };
      //static void setState(byte bitId, bool bit) {if (chIdx<nChans) anachan[bitId].setState(bit); };;
		static word getAnaState(byte chIdx) {if (chIdx<nChans) return anachan[chIdx].getAnaState(); };
		static word produceBLEana(byte chIdx);
	private:  
	  static anaIO * anachan;
     void setMode(byte mode);
	  word getAnaState();
	  void setAnaState(word value);
};
