

//  https://learn.adafruit.com/bluefruit-nrf52-feather-learning-guide/nrf52-adc
#define AIOS_UUID(val) ("555a0002-" val "-467a-9538-01f0652c74e8")
#define ANAS 1

/* AIOS  ​Automation IO Service */
#define UUID16_SVR_AUTOMATION_IO     "1815"

/* org.bluetooth.characteristic.digital  
| 11 11 11 11 11 11 11 01 | // all undefined; b7:tristate
| b0 b1 b2 b3 b4 b5 b6 b7 | // transmission order
| 0b1011111111111111      | // binary representation
*/
#define UUID16_CHR_DIGITAL           "2A56"

/*  org.bluetooth.characteristic.analog  */
#define UUID16_CHR_ANALOG            "2A58"

/* org.bluetooth.characteristic.aggregate */
#define UUID16_CHR_AGGREGATE         0x2A5A

/* https://www.bluetooth.com/specifications/gatt/descriptors/ */
#define DSC_number_of_digitals       "2909"
#define DSC_valid_range              "2906"

// time_trigger_setting :  The value of the descriptor has two parts. Part one is a condition field and occupies one octet, and part two is the comparison value (trigger point) that the characteristic value is checked against.  
//  <InformativeText>Available Conditions</InformativeText>
//   <Enumeration key="0" value="None" description="No time-based triggering used (valid for: Digital, Analog)" requires="C1"/>
//   <Enumeration key="1" value="Time Interval" description="Indicates or notifies unconditionally after a settable time.  This condition will cause server to periodically send notification or indication for the corresponding characteristic regardless of the Value Trigger state (valid for: Digital, Analog)" requires="C2"/>
//   <Enumeration key="2" value="Time Interval" description="Not indicated or notified more often than a settable time. After a successful indication or notification, the next indication or notification shall not be sent for the Time Interval time.  When the Time Interval expires, the characteristic is indicated or notified If the corresponding Value Trigger has a different state than at the time of the last successful indication or notification (valid for: Digital, Analog)" requires="C2" />
//   <Enumeration key="3" value="Count" description="Changed more often than.  This condition will cause server to count number of times the Value Trigger has changed its state and send the notification or indication for the corresponding characteristic on the “count” occurrence of the state change (valid for: Digital, Analog)" requires="C3"/>
//  <ReservedForFutureUse start="4" end="255" />
#define DSC_time_trigger_setting  "290E"


//  The value of the descriptor has two parts. Part one is a condition field and occupies one octet, and part two is the comparison value (trigger point) that the characteristic value is checked against. 
// <InformativeText>Available Conditions</InformativeText>
// <Enumeration key="0" value="None" description="The state is changed if the characteristic value is changed. (valid for: Digital, Analog)" />  <Enumeration key="1" value="Analog" description="Crossed a boundary. The state is changed if the value of the analog characteristic changes from less than to greater than a settable Analog value, or from greater than to less than a settable Analog value (valid for: Analog)" requires="C2"/>                <Enumeration key="2" value="Analog" description="On the boundary. The state is changed if the value of an analog characteristic changes from less than to equal to a settable Analog value, or from greater than to equal to a settable Analog value, or from equal to to less than or greater than a settable Analog value (valid for: Analog)" requires="C2" />                <Enumeration key="3" value="Analog" description="The state is changed if the value of the analog characteristic is changed more than a settable Analog value (valid for: Analog)" requires="C2"/>                <Enumeration key="4" value="Bit Mask" description="Mask then compare (logical-and of the Digital Input and the Bit Mask, condition is true if the result of this is different from the last stet) (valid for: Digital)" requires="C3" />                <Enumeration key="5" value="Analog Interval" description="Inside or outside the boundaries. The state is changed if the value of the analog characteristic changes from less than a settable Analog One value and greater than a settable Analog Two value to greater than a settable Analog One value or less than a settable Analog Two value (valid for: Analog)" requires="C4" />                <Enumeration key="6" value="Analog Interval" description="On the boundaries. The state is changed if the value of the analog characteristic changes from equal to a settable Analog One value or settable Analog Two value to any other value (valid for: Analog)" requires="C4" />                <Enumeration key="7" value="None" description="No value trigger.  This condition causes no state change regardless if the characteristic value changes.  It can be used for example when the value of one or more characteristic should not cause indication or notification of the Aggregate characteristic (valid for: Digital, Analog)" />                <ReservedForFutureUse start="8" end="255" />
#define DSC_value_trigger_setting  "290A"

/* <Summary>
        One or more Characteristic Presentation Format descriptors may be present. If multiple of these descriptors are present, then a Aggregate Formate descriptor is present.   This descriptor is read only and does not require authentication or authorization to read.
        This descriptor is composed of five parts: format, exponent, unit, name space and description.
        The Format field determines how a single value contained in the Characteristic Value is formatted.
        The Exponent field is used with integer data types to determine how the Characteristic Value is further formatted.
        The actual value = Characteristic Value * 10^Exponent.            
   </Summary> 
   If a device has more than one instance of the Analog characteristic, each characteristic shall include a Characteristic Presentation Format descriptor that has a namespace / description value that is unique for that instance of the Analog characteristic. The Namespace «Bluetooth SIG» as defined in [2] shall be used. Description values from 0x0001 and upwards shall be used to uniquely identify each Analog characteristic.
   */
#define DSC_characteristic_presentation_format  "2904"  // for ch=%d
#define DSC_data_type_uint16                  06
#define DSC_data_type_sint16                  14

#define UN_length_meter                         0x2701
#define UN_time_second                          0x2703
#define UN_current_ampere                       0x2704
#define UN_voltage_volt                         0x2728
#define UN_power_watt                           0x2726
#define UN_energy_joule                         0x2725


#define VAL_TRIG_COND(chan) (value_trigs[chan].condition)
#define VAL_TRIG_VAL1(chan) (value_trigs[chan].value1)
#define VAL_TRIG_VAL2(chan) (value_trigs[chan].value2)

// time trigger condition
#define ttINTERVAL 1
#define ttMININTERVAL 2
#define ttNRVALTRIGS 3

// value trigger condition
#define vtBOUNDPASS 1
#define vtEQUALS 2
#define vtDEVIATES 3
#define vtDIFFINMASK 4
#define vtINOUTAREA 5
#define vtONBOUNDARY 6
#define vtNONE 7

#ifndef LenDigBits
const uint8_t nAnaChan = 4;  // NUM_ANALOG_INPUTS-1; //TOTAL_ANALOG_PINS;  leave out Battery channel
const uint8_t nDigBits = 24 ; //NUM_DIGITAL_PINS; 

// nr of bytes required to store dig io bits i.e. 4 per byte
#define LenDigBits ((nDigBits >> 2) + ((nDigBits &3) ? 1 : 0))

#endif

// https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Descriptors/org.bluetooth.descriptor.gatt.characteristic_presentation_format.xml
// gatttool -b C9:04:5E:8D:26:97 --char-read --handle 0x001e
// Characteristic value/descriptor: 14 fb 00 20 00 00 00 00
typedef struct __attribute__ ((packed)) pres_format_t {
	uint8_t  format;      // enum "0" .. "27"
	int8_t   exponent;    // 
	uint16_t unit;        // uuid
	uint8_t  namespc;     // used to identify the organization
	int16_t  description; // enumerated value from the organization (also for analog channel nr)
	} prs_frm_t;

// structures for trigger conditions
// these struct result in condition beeing first / Big Endian
typedef struct time_trig_t {
	uint8_t  condition;  // 0:no ,1:time interv ,2:time interv cond, 3:count
	ulong tookms;
	union {
		uint32_t interv;		// uint24 [s]
		uint16_t cnt;
		uint8_t  dum;
	} tm;
} time_trig_t;
typedef struct dig_val_trig_t {
	uint8_t condition;	// 0:anyChg ,4:chg in dig bit mask, 7:not triggering
	//uint8_t mask[nDigBits/4+1];	//
	} dig_val_trig_t;
typedef struct ana_val_trig_t {
	uint8_t 	condition;	// 0:anyChg ,1:crossingVal1 ,2:arrive or leave Val1 ,3:step exceeds Val1 , 4:chg in dig bit mask ,5:in or out bounds ,6:from val1-2 to other ,7:not triggering
	union {
		uint16_t val;
		uint16_t vals[2];
	} lev;
	} ana_val_trig_t;
typedef struct __attribute__ ((packed)) ana_valid_range_t {
  uint16_t Lower_inclusive_value;
  uint16_t Upper_inclusive_value;
} ana_valid_range_t;

class digCharacteristic : public BLECharacteristic
{
	public:
		digCharacteristic(char* bleuuid);
		bool notifyEnabled(void);
		word changed(void);
		void write(void);
	protected:
		byte digdata[LenDigBits];
};


class anaCharacteristic : public BLEShortCharacteristic
{
	public:
		byte anachan;
		anaCharacteristic();  //dummy
		#ifdef ANAS
		anaCharacteristic(char* bleuuid, uint8_t chan);
		//void addDescriptors(void);
		#endif
		void updateVoltRange(void);
		bool notifyEnabled(void);
	protected:
		ana_valid_range_t ana_valid_range;
    //uint16_t valrngHandle;
};

//extern BLEService   aios ;  // automation io service
extern void setupDIGC(BLEService& aios);
extern word pollDIGC(ulong tick);

void setupANAC(BLEService& aios);
word pollANAC(ulong tick);
extern ulong tdiff(ulong tick1, ulong tick2);  // see pinIO
