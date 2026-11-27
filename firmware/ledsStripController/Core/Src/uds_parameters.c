#include "uds_parameters.h"

	float dashboardParamCouple[3];
	float dashboardParamMaxHold[3]; // stores max value for each displayed param (used by Max Hold feature)


	uint8_t shownParamsArray[240];
	uint8_t params_setup_dashboardPageIndex=0;
	uint8_t total_pages_in_params_setup_dashboard_menu=0;

	uint8_t total_pages_in_dashboard_menu_gasoline=74; //47 upstream pages, 20 two or one value pages, 7 three value pages
	uint8_t total_pages_in_dashboard_menu_diesel=57; //brake in show params 10/10/2026 - was 56
	uint8_t currentParamElementSelection=0;
	

	    // Costruzione della stringa
	    //buildLineWithFormat(template, power, torque, result);


	//format string $x.yf for float params where y is decimal part and x is integer part
	//format string $enum for enumerator values derived from specific enum arrays
	const	uds_params_couple_element uds_params_array[2][UDS_PAGES_MAX]={
				{ 	//Gasoline
					{.name="PWR $3.0fCV $3.0fNm",				.udsParamId={1,		2		}}, //param couple: PWR and Torque
					{.name="OIL $1.1fbar W.$3.0f" "\xB0" "C",	.udsParamId={0,		42		}}, //param couple: OIL pressure and Water Temp.
					{.name="OIL $1.1fbar O.$3.0f" "\xB0" "C",	.udsParamId={0,		5		}}, //param couple: OIL pressure and Oil Temp.
					{.name="OIL $3.0f" "\xB0" "C W.$3.0f" "\xB0" "C",	.udsParamId={5,		42		}}, //param couple: OIL temp. and Water Temp.
					{.name="OIL $1.1fL Qual.$3.0f%",			.udsParamId={28,	31		}}, //param couple: OIL level and Oil Quality
					{.name="BAT $3.0f% $3.1fA",					.udsParamId={21,	4		}}, //param couple: BAT State Of Charge and current
					{.name="BAT $2.2fV $3.1fA",					.udsParamId={35,	4		}}, //param couple: BAT voltage and current
					{.name="PWR: $3.2fCV   ",					.udsParamId={1,		1		}}, //Power
					{.name="TORQUE: $3.2fNm",					.udsParamId={2,		2		}}, //Torque
					{.name="IC AirOut: $3.1f" "\xB0" "C",		.udsParamId={22,	22		}}, //Intercooler output air temperature
					{.name="IC AirIn:  $3.1f" "\xB0" "C",		.udsParamId={23,	23		}}, //Intercooler input  air temperature
					{.name="BOOST ABS: $2.1fbar",				.udsParamId={24,	24		}}, //Boost Absolute Pressure
					{.name="BOOST: $3.1fbar",					.udsParamId={25,	25		}}, //Boost Pressure calculated from Absolute pressure
					{.name="TURBO: $2.3fV  ",					.udsParamId={26,	26		}}, //Turbo Sensor Voltage
					{.name="ODOM.LAST:$6.0fkm",					.udsParamId={27,	27		}}, //distance since last time odometer was zeroized
					{.name="OIL: $1.3fL    ",					.udsParamId={28,	28		}}, //Oil Quantity
					{.name="OIL: $1.3fbar  ",					.udsParamId={29,	29		}}, //Oil Pressure
					{.name="OIL: $3.0f" "\xB0" "C   ",			.udsParamId={30,	30		}}, //multiair Oil Temperature
					{.name="OIL QUALY: $3.0f%",					.udsParamId={31,	31		}}, //Oil Quality
					{.name="OIL UnAir: $2.2f" "\xB0" "C",		.udsParamId={32,	32		}}, //Multiair Module Oil Temperature
					{.name="GEARBOX: $2.2f" "\xB0" "C",			.udsParamId={33,	33		}}, //Gearbox Temperature
					{.name="BATT.: $3.0f%  ",					.udsParamId={34,	34		}}, //Battery State Of Charge
					{.name="BATT.: $3.3fA  ",					.udsParamId={4,		4		}}, //Battery Current
					{.name="BATT.: $3.3fV  ",					.udsParamId={35,	35		}}, //Battery Voltage
					{.name="AIR COND.:$2.2fbar",				.udsParamId={36,	36		}}, //Air Conditioner Pressure (same format as diesel, max 18 chars - V.3.9.2)
					{.name="CUR. GEAR: $enum",					.udsParamId={6,		6		}}, //Current Gear
					{.name="Time ON: $6.0fmin",					.udsParamId={37,	37		}}, //Time Since engine on
					{.name="OVER RPM: $3.0fsec",				.udsParamId={38,	38		}}, //elapsed time in engine overspeed condition
					{.name="OVER RPM: $6.0f",					.udsParamId={39,	39		}}, //number of times of engine overspeed condition
					{.name="EXHAUST GAS:$4.0f" "\xB0" "C",		.udsParamId={40,	40		}}, //Exaust gas temperature
					{.name="CATAL.: $4.1f" "\xB0" "C",			.udsParamId={41,	41		}}, //catalytic converter temperature sensor
					{.name="WATER: $3.2f" "\xB0" "C ",			.udsParamId={42,	42		}}, //water temperature
					{.name="KNOCK: $4.3fmV ",					.udsParamId={43,	43		}}, //head knock sensor voltage
					{.name="KEY ID: $4.0f  ",					.udsParamId={44,	44		}}, //inserted Key ID
					{.name="SparkCYL1: $1.2fdeg",				.udsParamId={45,	45		}}, //Cylinder correction
					{.name="SparkCYL2: $1.2fdeg",				.udsParamId={46,	46		}}, //Cylinder correction
					{.name="SparkCYL3: $1.2fdeg",				.udsParamId={47,	47		}}, //Cylinder correction
					{.name="SparkCYL4: $1.2fdeg",				.udsParamId={48,	48		}}, //Cylinder correction
					{.name="DRIVE STYLE: $enum",				.udsParamId={15,	15		}}, //Drive Style
					{.name="SPEED: $3.3fkm/h",					.udsParamId={7,		7		}}, //Speed
					{.name="Seatbelt Alarm:$enum",				.udsParamId={13,	13		}}, //SeatBelt Alarm
					{.name="  0-100Km/h $1.3fs",				.udsParamId={9,		9		}}, //0-100km/h time statistic
					{.name="100-200Km/h $1.3fs",				.udsParamId={10,	10		}}, //100-200km/h time statistic
					{.name="Best  0-100:$2.2fs",				.udsParamId={11,	11		}}, //0-100km/h Best time statistic (same format as diesel: MISSED fits in 18 chars - V.3.9.2)
					{.name="Best100-200:$2.2fs",				.udsParamId={12,	12		}}, //100-200km/h Best time statistic (same format as diesel: MISSED fits in 18 chars - V.3.9.2)
					{.name="Pedal Map: $enum",					.udsParamId={17,	17		}}, //selected Pedal Map
					{.name="BrakePedal: $3.0f%",				.udsParamId={18,	18		}}, //Brake pedal press percentage (0x107) //brake in show params 10/10/2026
					{.name="GBX $3.0f" "\xB0" "C W.$3.0f" "\xB0" "C",	.udsParamId={33,	42		}}, //page 47: gearbox oil and water temp.
					{.name="CAC $3.0f" "\xB0" "C IC $3.0f" "\xB0" "C",	.udsParamId={89,	22		}}, //page 48: DID 1C16 (hypothesis: CAC coolant) and intercooler air out
					{.name="MAP $1.2f BST $1.2f",				.udsParamId={90,	24		}}, //page 49: manifold pressure and boost before throttle, bar abs
					{.name="AIR $3.0fkg $2.1fL/h",				.udsParamId={91,	92		}}, //page 50: air mass kg/h and fuel flow L/h
					{.name="KNK1 $3.0f 2 $3.0fmV",				.udsParamId={93,	94		}}, //page 51: knock sensor cyl 1 and 2
					{.name="KNK3 $3.0f 4 $3.0fmV",				.udsParamId={95,	96		}}, //page 52: knock sensor cyl 3 and 4
					{.name="GEAR $enum TQ $3.0fNm",				.udsParamId={6,		2		}}, //page 53: gear and torque, both native (no UDS request)
					[54]={.name="SPK1 $2.1f 2 $2.1f" "\xB0",	.udsParamId={97,	98	}}, //spark correction cyl 1 and 2, signed
					[55]={.name="SPK3 $2.1f 4 $2.1f" "\xB0",	.udsParamId={99,	100	}}, //spark correction cyl 3 and 4, signed
					[56]={.name="KNOCK: $4.0fmV",	.udsParamId={101,	101	}}, //knock sensor, 2 bytes (page 32 reads 1 byte)
					[57]={.name="RAIL: $3.0fbar",	.udsParamId={102,	102	}}, //fuel rail pressure, DID 1946
					[58]={.name="OIL $3.0f" "\xB0" "C MA $3.0f" "\xB0" "C",	.udsParamId={5,	32	}}, //oil (native) and MultiAir oil
					[59]={.name="EGT$4.0f CAT$4.0f" "\xB0" "C",	.udsParamId={40,	41	}}, //exhaust gas and catalyst
					[60]={.name="IDLE $4.0f/$4.0frpm",	.udsParamId={104,	105	}}, //idle target / engine speed
					[61]={.name="ST $3.1f LT $3.1f%",	.udsParamId={106,	107	}}, //OBD 06 / 07 fuel trims
					[62]={.name="OUT $3.0f" "\xB0" "C W.$3.0f" "\xB0" "C",	.udsParamId={108,	42	}}, //OBD 46 ambient air and water
					[63]={.name="ADV $3.1f" "\xB0" " LD $3.0f%",	.udsParamId={109,	110	}}, //OBD 0E timing advance and OBD 04 load
					[64]={.name="LAMBDA: $1.3f",	.udsParamId={111,	111	}}, //OBD 44 commanded lambda
					[65]={.name="W.UP $2.1fmin $3.0f" "\xB0" "C",	.udsParamId={14,	5	}}, //warm-up timer (native 14) and oil
					[66]={.name="$2.1fL/h $2.1fL/100",				.udsParamId={92,	16		}}, //page 66: fuel flow and instant consumption
					[67]={.name="U$3.0f O$3.0f W$3.0f" "\xB0" "",	.udsParamId={32,	5,	42	},	.values=3}, //UniAir, oil and water
					[68]={.name="O$3.0f W$3.0f G$3.0f" "\xB0" "",	.udsParamId={5,	42,	33	},	.values=3}, //oil, water and gearbox
					[69]={.name="M$1.2f B$1.2f T$3.0f",	.udsParamId={90,	24,	22	},	.values=3}, //manifold, boost and intercooler air
					[70]={.name="P$1.1f O$3.0f W$3.0f" "\xB0" "",	.udsParamId={0,	5,	42	},	.values=3}, //oil pressure, oil and water
					[71]={.name="$2.1fV $4.0fA $3.0f%",	.udsParamId={35,	4,	21	},	.values=3}, //battery voltage, current and charge
					[72]={.name="G$enum T$3.0f B$1.2f",	.udsParamId={6,	2,	24	},	.values=3}, //gear, torque and boost
					[73]={.name="E$4.0f C$4.0f T$3.0f",	.udsParamId={40,	41,	22	},	.values=3}, //exhaust gas, catalyst and intercooler air
					//
//					{.name="RAM: $5.0fB",						.udsParamId={16,	16		}}, //Free RAM
//									{.name={'T', 'Y', 'R', 'E', ' ', 'R', 'F', ':', ' ',},              .reqId=0x18DAC7F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x022240B3),   .replyId=0x18DAF1C7,    .replyLen=1,    .replyOffset=4, .replyValOffset=-50,    .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={0xB0,'C'}                        }, // LIMITE BYTE
//									{.name={'T', 'Y', 'R', 'E', ' ', 'R', 'R', ':', ' ',},              .reqId=0x18DAC7F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x022230B4),   .replyId=0x18DAF1C7,    .replyLen=1,    .replyOffset=4, .replyValOffset=-50,    .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={0xB0,'C'}                        }, // LIMITE BYTE
//									{.name={'T', 'Y', 'R', 'E', ' ', 'L', 'F', ':', ' ',},              .reqId=0x18DAC7F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x022240B2),   .replyId=0x18DAF1C7,    .replyLen=1,    .replyOffset=4, .replyValOffset=-50,    .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={0xB0,'C'}                        }, // LIMITE BYTE
//									{.name={'T', 'Y', 'R', 'E', ' ', 'L', 'F', ':', ' ',},              .reqId=0x18DAC7F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x022240B1),   .replyId=0x18DAF1C7,    .replyLen=1,    .replyOffset=4, .replyValOffset=-50,    .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={0xB0,'C'}                        }, // LIMITE BYTE
// unit grams/km (wrong?)			{.name={'P','A','R','T','I','C','U','L','.', ':',' ',},				.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032218AA),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=0,001,			.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'g','/','k','m'}					}, // GRAMMI PER KM
// just to print stuff for debug:	{.name={'D',},														.reqId=0x1F,		.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),	.replyId=0x00000000,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=2,	.replyMeasurementUnit={'s', }							}, //debug string

				},{ //diesel
						{.name="PWR $3.0fCV $3.0fNm",				.udsParamId={1,		2		}}, //param couple: PWR and Torque
						{.name="OIL $1.1fbar W.$3.0f" "\xB0" "C",	.udsParamId={0,		68		}}, //param couple: OIL pressure and Water Temp.
						{.name="OIL $1.1fbar O.$3.0f" "\xB0" "C",	.udsParamId={0,		5		}}, //param couple: OIL pressure and Oil Temp.
						{.name="OIL $3.0f" "\xB0" "C W.$3.0f" "\xB0" "C",	.udsParamId={5,		68		}}, //param couple: OIL temp. and Water Temp.
						{.name="OIL $2.1fmm Qu.$3.0f%",				.udsParamId={64,	63		}}, //param couple: OIL level and Oil Quality
						{.name="BAT $3.0f% $3.1fA",					.udsParamId={21,	4		}}, //param couple: BAT State Of Charge and current
						{.name="BAT $2.2fV $3.1fA",					.udsParamId={62,	4		}}, //param couple: BAT voltage and current
						{.name="DPF $2.2f% $2.2f" "\xB0" "C",		.udsParamId={55,	56		}}, //param couple: DPF clogging percentage and temperature
						{.name="REGEN $2.1f% $3.0f" "\xB0" "C",		.udsParamId={57,	56		}}, //param couple: DPF regeneration progress percentage and temperature
						{.name="EGR C.$3.0f% ST.$3.0f%",			.udsParamId={73,	74		}}, //param couple: EGR cmd and sts
						{.name="PWR: $3.2fCV   ",					.udsParamId={1,		1		}}, //Power
						{.name="TORQUE: $3.2fNm",					.udsParamId={2,		2		}}, //Torque
						{.name="DPF: $3.2f%    ",					.udsParamId={55,	55		}}, //DPF clogging percentage
						{.name="DPF: $3.2f" "\xB0" "C   ",			.udsParamId={56,	56		}}, //DPF Temperature
						{.name="DPF REGEN: $3.2f%",					.udsParamId={57,	57		}}, //DPF regeneration progress percentage
						{.name="REGEN: $enum   ",					.udsParamId={8,		8		}}, //DPF regeneration type
						{.name="LAST REGEN:$5.0fkm",				.udsParamId={58,	58		}}, //DPF last regeneration distance in km
						{.name="TOT REGEN: $5.0f",					.udsParamId={59,	59		}}, //DPF total number of regenerations
						{.name="MEAN REGEN:$5.0fkm",				.udsParamId={60,	60		}}, //DPF mean regeneration distance in km
						{.name="MEAN REGEN:$3.0fmin",				.udsParamId={61,	61		}}, //DPF mean regeneration duration in minutes
						{.name="BAT $2.2fV     ",					.udsParamId={62,	62		}}, //Battery Voltage
						{.name="BAT $3.0f%     ",					.udsParamId={21,	21		}}, //Battery State Of Charge percentage
						{.name="BAT $3.1fA     ",					.udsParamId={4,		4		}}, //Battery current
						{.name="OIL QUALY: $3.0f%",					.udsParamId={63,	63		}}, //Oil Quality
						{.name="OIL: $3.0f" "\xB0" "C   ",			.udsParamId={5,		5		}}, //Oil temperature
						{.name="OIL: $2.2fbar  ",					.udsParamId={0,		0		}}, //Oil pressure
						{.name="OIL: $3.2fmm   ",					.udsParamId={64,	64		}}, //Oil quantity in mm
						{.name="ADBLUE: $3.2fL ",					.udsParamId={65,	65		}}, //Adblue quantity in Liters
						{.name="ADBLUE: $3.2f% ",					.udsParamId={66,	66		}}, //Adblue quantity in percentage
						{.name="GEARBOX: $3.2f" "\xB0" "C",			.udsParamId={33,	33		}}, //gearbox temperature
						{.name="EXHAUST GAS:$4.0f" "\xB0" "C",		.udsParamId={67,	67		}}, //exhaust gas temperature (turbo input)
						{.name="CUR. GEAR: $enum",					.udsParamId={6,		6		}}, //current gear
						{.name="WATER: $3.0f" "\xB0" "C ",			.udsParamId={68,	68		}}, //water temperature
						{.name="EGR CMD:$2.2f% ",					.udsParamId={73,	73		}}, //EGR command
						{.name="EGR:    $2.2f% ",					.udsParamId={74,	74		}}, //EGR status
						{.name="TURBO REQ: $2.1fbar",				.udsParamId={76,	76		}}, //Turbo Request pressure
						{.name="TURBO REQ: $2.2f%",					.udsParamId={77,	77		}}, //Turbo Request percentage
						{.name="TURBO: $2.2f" "\xB0" "C ",			.udsParamId={78,	78		}}, //Turbo temperature
						{.name="TURBO: $2.2fbar",					.udsParamId={79,	79		}}, //Turbo pressure
						{.name="TURBO: $2.2f%  ",					.udsParamId={80,	80		}}, //Turbo percentage
						{.name="BOOST REQ.:$2.1fbar",				.udsParamId={81,	81		}}, //Boost Request pressure
						{.name="BOOST: $1.2fV  ",					.udsParamId={82,	82		}}, //Boost sensor voltage
						{.name="RAIL: $5.2fbar ",					.udsParamId={83,	83		}}, //Rail pressure
						{.name="DIESEL: $2.2f" "\xB0" "C",			.udsParamId={84,	84		}}, //Diesel temperature
						{.name="ODOM.LAST: $5.0fkm",				.udsParamId={85,	85		}}, //Distance in km since last odometer reset
						{.name="AIR COND.:$2.2fbar",				.udsParamId={86,	86		}}, //Air conditioner pressure
						{.name="FUEL CONS.:$1.2fL/h",				.udsParamId={87,	87		}}, //Fuel Consume
						{.name="DEBIMETER:$3.2f" "\xB0" "C",		.udsParamId={88,	88		}}, //Debimeter temperature
						{.name="SPEED:$3.2fkm/h",					.udsParamId={7,		7		}}, //speed
						{.name="Seatbelt Alarm:$enum",				.udsParamId={13,	13		}}, //Seatbelt Alarm Status
						{.name="0-100Km/h:  $2.2fs",				.udsParamId={9,		9		}}, //0-100km/h Statistic
						{.name="100-200Km/h:$2.2fs",				.udsParamId={10,	10		}}, //100-200km/h Statistic
						{.name="Best  0-100:$2.2fs",				.udsParamId={11,	11		}}, //0-100km/h Best Statistic
						{.name="Best100-200:$2.2fs",				.udsParamId={12,	12		}}, //0-100km/h Best Statistic
						{.name="DRIVE STYLE: $enum",				.udsParamId={15,	15		}}, //Drive Style
						{.name="Pedal Map: $enum",					.udsParamId={17,	17		}}, //selected Pedal Map
						{.name="BrakePedal: $3.0f%",				.udsParamId={18,	18		}}, //Brake pedal press percentage (0x107) //brake in show params 10/10/2026
//						{.name="RAM: $5.0fB",						.udsParamId={16,	16		}}, //Free RAM

//								{.name={'F','-','L',' ','T','I','R','E',':',' ',},					.reqId=0x18DAC7F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032240B1),	.replyId=0x18DAF1C7,	.replyLen=1,	.replyOffset=4, .replyValOffset=-50,	.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,'C',}						},
//								{.name={'F','-','R',' ','T','I','R','E',':',' ',},					.reqId=0x18DAC7F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032240B2),	.replyId=0x18DAF1C7,	.replyLen=1,	.replyOffset=4, .replyValOffset=-50,	.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,'C',}						},
//								{.name={'R','-','L',' ','T','I','R','E',':',' ',},					.reqId=0x18DAC7F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032240B3),	.replyId=0x18DAF1C7,	.replyLen=1,	.replyOffset=4, .replyValOffset=-50,	.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,'C',}						},
//								{.name={'R','-','R',' ','T','I','R','E',':',' ',},					.reqId=0x18DAC7F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032240B4),	.replyId=0x18DAF1C7,	.replyLen=1,	.replyOffset=4, .replyValOffset=-50,	.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,'C',}						},
//may be not received			{.name={'E','G','R',' ','C','M','D','1',':',},						.reqId=0x18DB33F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322012C),	.replyId=0x18DBF133,	.replyLen=1,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.3921568627,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							},
//sometimes wrong value			{.name={'E','G','R',' ','M','E','A','S','.', ':',' ',},				.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322189C),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=-32767,	.replyScale=0.00305185095,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							},
//wrong value 43520g			{.name={'P','A','R','T','I','C','U','L','.', ':',' ',},				.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032218AA),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'g',}							},
//may be not received			{.name={'T','U','R','B','O','1',':',},								.reqId=0x18DB33F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03220175),	.replyId=0x18DBF133,	.replyLen=2,	.replyOffset=5,	.replyValOffset=0,		.replyScale=0.1,			.replyScaleOffset=-40,	.replyDecimalDigits=1,	.replyMeasurementUnit={0xB0,'C',}						},
//may be not received			{.name={'T','U','R','B','O','4',':',},								.reqId=0x18DB33F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322010B),	.replyId=0x18DBF133,	.replyLen=1,	.replyOffset=0,	.replyValOffset=-100,	.replyScale=0.01,			.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'b','a','r',}					},
//stuck to 3,18V				{.name={'T','U','R','B','O','5',':',},								.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221936),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.0001,			.replyScaleOffset=0,	.replyDecimalDigits=2,	.replyMeasurementUnit={'V',}							},
//may be not received			{.name={'F','U','E','L',':',' ',},									.reqId=0x18DB33F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03220123),	.replyId=0x18DBF133,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=10,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'k','P','a',}					},
//just to print stuff for debug:{.name={'D',},														.reqId=0x1F,		.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),	.replyId=0x00000000,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=2,	.replyMeasurementUnit={'s', }							}, //debug string

				}
	};

	
	const	uds_param_single_element single_uds_params_array[UDS_SINGLE_PARAMS_MAX]={
		{.reqId=0x10,		.reqLen=4,  .reqData=SWAP_UINT32(0x00000000),	.replyId=0x000004B2,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=0.1,			.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'b','a','r',}					}, //0		oil pressure
		{.reqId=0x11,	    .reqLen=4,	.reqData=SWAP_UINT32(0x00000000),   .replyId=0x000000FB,	.replyLen=2,	.replyOffset=0,	.replyValOffset=-500,	.replyScale=0.000142378,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'C','V',}						}, //1		power
		{.reqId=0x12,	    .reqLen=4,	.reqData=SWAP_UINT32(0x00000000),   .replyId=0x000000FB,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=1,				.replyScaleOffset=-500,	.replyDecimalDigits=0,	.replyMeasurementUnit={'N','m',}						}, //2		torque
		{.reqId=0x13,		.reqLen=4,  .reqData=SWAP_UINT32(0x00000000),	.replyId=0x0000041A,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							}, //3		Battery percentage
		{.reqId=0x14,		.reqLen=4,  .reqData=SWAP_UINT32(0x00000000),	.replyId=0x0000041A,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.1,			.replyScaleOffset=-250,	.replyDecimalDigits=2,	.replyMeasurementUnit={'A',}							}, //4		battery current
		{.reqId=0x15,		.reqLen=4,  .reqData=SWAP_UINT32(0x00000000),	.replyId=0x000004B2,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=1,				.replyScaleOffset=-40,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,'C',}						}, //5		oil temperature

		{.reqId=0x17,		.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),	.replyId=0x000002EF,	.replyLen=1,	.replyOffset=0, .replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={' ',}							}, //6		current gear
		{.reqId=0x18,		.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),	.replyId=0x00000101,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.0625,			.replyScaleOffset=0,	.replyDecimalDigits=2,	.replyMeasurementUnit={'k','m','/','h', }				}, //7		speed
		{.reqId=0x19,		.reqLen=4,  .reqData=SWAP_UINT32(0x00000000),	.replyId=0x000005AE,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={' ',}							}, //8		DPF Regeneration type
		{.reqId=0x1A,		.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),	.replyId=0x00000000,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=2,	.replyMeasurementUnit={'s', }							}, //9		statistic 0/100
		{.reqId=0x1B,		.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),	.replyId=0x00000000,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=2,	.replyMeasurementUnit={'s', }							}, //10		statistic 100/200
		{.reqId=0x1C,		.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),	.replyId=0x00000000,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=2,	.replyMeasurementUnit={'s', }							}, //11		best statistic 0/100
		{.reqId=0x1D,		.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),	.replyId=0x00000000,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=2,	.replyMeasurementUnit={'s', }							}, //12		best statistic 100/200
		{.reqId=0x1E,		.reqLen=4,	.reqData=SWAP_UINT32(0x032255A0),	.replyId=0x18DAF160,	.replyLen=1,	.replyOffset=0, .replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={' ',}							}, //13		seat belt alarm
		{.reqId=0x1F,		.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),	.replyId=0x00000000,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=2,	.replyMeasurementUnit={'s', }							}, //14		warm-up timer, minutes (was debug string)
		{.reqId=0x20,		.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),	.replyId=0x00000000,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=2,	.replyMeasurementUnit={' ', }							}, //15		Drive Style
		{.reqId=0x21,		.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),	.replyId=0x00000000,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'B', }							}, //16		Free RAM
		{.reqId=0x22,		.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),	.replyId=0x00000000,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={' ', }							}, //17		Selected Pedal Map
		{.reqId=0x23,		.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),	.replyId=0x00000000,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'%', }							}, //18		Brake Pedal Press (do not use because it resides on C2, and not available on C1. if required reuse this record for something else)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x03223A16),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.0625,         .replyScaleOffset=0,    .replyDecimalDigits=3,  .replyMeasurementUnit={'d','e','g',}					}, //19		cylinder5 correction (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x03223A17),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.0625,         .replyScaleOffset=0,    .replyDecimalDigits=3,  .replyMeasurementUnit={'d','e','g',}					}, //20		cylinder6 correction (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x032219BD),	.replyId=0x18DAF110,	.replyLen=1,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							}, //21		Alternative battery percentage (gasoline and diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221935),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=-40,    .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={0xB0,'C'}						}, //22		intercooler air out (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03223A58),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=-40,    .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={0xB0,'C'}						}, //23		intercooler air in (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x0322195A),   .replyId=0x18DAF110,    .replyLen=2,    .replyOffset=0, .replyValOffset=-1,     .replyScale=0.001,          .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={'B','A','R'}						}, //24		boost absolute pressure (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x0322195A),   .replyId=0x18DAF110,    .replyLen=2,    .replyOffset=0, .replyValOffset=-1,     .replyScale=0.001,          .replyScaleOffset=-1,   .replyDecimalDigits=1,  .replyMeasurementUnit={'B','A','R'}						}, //25		boost pressure extracted from absolute pressure (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221936),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.0001,			.replyScaleOffset=0,	.replyDecimalDigits=2,	.replyMeasurementUnit={'V',}							}, //26		turbo (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03222002),	.replyId=0x18DAF110,	.replyLen=3,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.1,			.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'k','m',}						}, //27		odometer last (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x03223A41),   .replyId=0x18DAF110,    .replyLen=2,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.001,          .replyScaleOffset=0,    .replyDecimalDigits=3,	.replyMeasurementUnit={'L',},							}, //28		oil quantity (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x0322130A),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.039215686,    .replyScaleOffset=0,    .replyDecimalDigits=2,  .replyMeasurementUnit={'B','a','r',}					}, //29		oil pressure (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x03221302),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=1, .replyValOffset=0,      .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=0,  .replyMeasurementUnit={0xB0,'C',}						}, //30		oil temperature (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x03223813),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=0.0015259022,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							}, //31		oil quality (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x0322198E),   .replyId=0x18DAF110,    .replyLen=2,    .replyOffset=0, .replyValOffset=0,    	.replyScale=0.0625,         .replyScaleOffset=-40,  .replyDecimalDigits=2,  .replyMeasurementUnit={0xB0,'C',}						}, //32		multiair module oil temperature (gasoline)
		{.reqId=0x18DA18F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x032204FE),   .replyId=0x18DAF118,    .replyLen=1,    .replyOffset=0, .replyValOffset=-40,    .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={0xB0,'C'}						}, //33		gearbox temperature
		{.reqId=0x18DA40F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x03221005),	.replyId=0x18DAF140,	.replyLen=2,	.replyOffset=1,	.replyValOffset=0,  	.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							}, //34		battery percentage (gasoline)
		{.reqId=0x18DA40F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x03221004),   .replyId=0x18DAF140,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.1,            .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={'V',}							}, //35		battery voltage (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322192F),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.01,			.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'b','a','r',}					}, //36		air conditioner pressure (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x03221009),   .replyId=0x18DAF110,    .replyLen=2,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.25,           .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={'m',}                            }, //37		time since engine ON (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x03222006),   .replyId=0x18DAF110,    .replyLen=2,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.01,           .replyScaleOffset=0,    .replyDecimalDigits=2,  .replyMeasurementUnit={'s',}							}, //38		time spent in engine overspeed (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03222004),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=0,  .replyMeasurementUnit={' ',}							}, //39		engine overspeed number of times (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x032218BA),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=5,           	.replyScaleOffset=-50,  .replyDecimalDigits=0,  .replyMeasurementUnit={0xB0,'C',}                       }, //40		exhaust gas temperature (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x03221837),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,    	.replyScale=5,              .replyScaleOffset=-50,  .replyDecimalDigits=1,  .replyMeasurementUnit={0xB0,'C',}                       }, //41		Catalytic sensor temperature (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221003),	.replyId=0x18DAF110,	.replyLen=1,	.replyOffset=0, .replyValOffset=0,		.replyScale=1,			    .replyScaleOffset=-40,	.replyDecimalDigits=1,	.replyMeasurementUnit={0xB0,'C',}						}, //42		Water temperature (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x03221841),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.125,          .replyScaleOffset=0,    .replyDecimalDigits=3,  .replyMeasurementUnit={'m','V',}						}, //43		head knock (gasoline)
		{.reqId=0x18DA40F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x03220131),   .replyId=0x18DAF140,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=0,  .replyMeasurementUnit={' ',}							}, //44		Key ID (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322186C),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.0625,         .replyScaleOffset=0,    .replyDecimalDigits=3,  .replyMeasurementUnit={'d','e','g',}                    }, //45		cylinder1 correction (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x0322186D),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.0625,         .replyScaleOffset=0,    .replyDecimalDigits=3,  .replyMeasurementUnit={'d','e','g',}					}, //46		cylinder2 correction (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x0322186E),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.0625,         .replyScaleOffset=0,    .replyDecimalDigits=3,  .replyMeasurementUnit={'d','e','g',}					}, //47		cylinder3 correction (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x0322186F),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.0625,         .replyScaleOffset=0,    .replyDecimalDigits=3,  .replyMeasurementUnit={'d','e','g',}					}, //48		cylinder4 correction (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x032218F0),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=0,  .replyMeasurementUnit={' ',}							}, //49		drive style selector position (gasoline)
		{.reqId=0x18DAC7F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x022240B3),   .replyId=0x18DAF1C7,    .replyLen=1,    .replyOffset=4, .replyValOffset=-50,    .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={0xB0,'C'}						}, //50		front right tyre pressure (not implemented now)
		{.reqId=0x18DAC7F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x022230B4),   .replyId=0x18DAF1C7,    .replyLen=1,    .replyOffset=4, .replyValOffset=-50,    .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={0xB0,'C'}						}, //51		rear right tyre pressure (not implemented now)
		{.reqId=0x18DAC7F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x022240B2),   .replyId=0x18DAF1C7,    .replyLen=1,    .replyOffset=4, .replyValOffset=-50,    .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={0xB0,'C'}						}, //52		front left tyre pressure (not implemented now)
		{.reqId=0x18DAC7F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x022240B1),   .replyId=0x18DAF1C7,    .replyLen=1,    .replyOffset=4, .replyValOffset=-50,    .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={0xB0,'C'}						}, //53		rear left tyre pressure (not implemented now)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032218AA),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.001,			.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'g','/','k','m'}					}, //54		particulate (gasoline)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032218E4),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.015259022,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							}, //55		DPF clogging percentage (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x032218DE),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.02,			.replyScaleOffset=-40,	.replyDecimalDigits=1,	.replyMeasurementUnit={0xB0,'C',}						}, //56		DPF temperature (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x0322380B),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.001525902,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							}, //57		DPF regeneration progress percentage (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x03223807),	.replyId=0x18DAF110,	.replyLen=3,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.1,			.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'k','m',}						}, //58		last regeneration (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x032218A4),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=1,  			.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={' ',}							}, //59		total regenerations number (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x03223809),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=1,  			.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'k','m',}						}, //60		mean regeneration (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x0322380A),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.01666666666,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'m','i','n',}					}, //61		mean regeneration duration (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x03221955),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=0.0005,			.replyScaleOffset=0,	.replyDecimalDigits=3,	.replyMeasurementUnit={'V',}							}, //62		battery voltage (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x03223813),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=0.0015259022,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							}, //63		oil quality (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x0322194E),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=0.1,			.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'m','m',}						}, //64		oil level [mm in oil pan (50-70mmm)] (diesel)
		{.reqId=0x18DA01F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x0322D930),	.replyId=0x18DAF101,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=0.00097676774,	.replyScaleOffset=0,	.replyDecimalDigits=2,	.replyMeasurementUnit={'L',}							}, //65		adblue level in liters (diesel)
		{.reqId=0x18DA01F1,	.reqLen=4,  .reqData=SWAP_UINT32(0x0322D97C),	.replyId=0x18DAF101,	.replyLen=1,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=0.390625,		.replyScaleOffset=0,	.replyDecimalDigits=2,	.replyMeasurementUnit={'%',}							}, //66		adblue level in % (diesel)
		{.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03223836),   .replyId=0x18DAF110,    .replyLen=2,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.02,			.replyScaleOffset=-40,	.replyDecimalDigits=1,  .replyMeasurementUnit={0xB0,'C',}                       }, //67		exhaust gas temperature (turbo input) (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221003),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.02,			.replyScaleOffset=-40,	.replyDecimalDigits=1,	.replyMeasurementUnit={0xB0,'C',}						}, //68		water temperature (diesel)
		{.reqId=0x18DAC7F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032240B1),	.replyId=0x18DAF1C7,	.replyLen=1,	.replyOffset=4, .replyValOffset=-50,	.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,'C',}						}, //69		from left tire temperature (not implemented) (diesel)
		{.reqId=0x18DAC7F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032240B2),	.replyId=0x18DAF1C7,	.replyLen=1,	.replyOffset=4, .replyValOffset=-50,	.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,'C',}						}, //70		front right tire temperature (diesel)
		{.reqId=0x18DAC7F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032240B3),	.replyId=0x18DAF1C7,	.replyLen=1,	.replyOffset=4, .replyValOffset=-50,	.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,'C',}						}, //71		rear left tire temperature (diesel)
		{.reqId=0x18DAC7F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032240B4),	.replyId=0x18DAF1C7,	.replyLen=1,	.replyOffset=4, .replyValOffset=-50,	.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,'C',}						}, //72		rear right tire temperature (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322189B),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=-32767,	.replyScale=0.00305185095,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							}, //73		EGR command (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322189A),	.replyId=0x18DAF110,	.replyLen=1,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.1953125,		.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							}, //74		EGR status (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322189C),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=-32767,	.replyScale=0.00305185095,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							}, //75		EGR measured - sometimes shows wrong value - not used (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221942),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.000030517578,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'b','a','r',}					}, //76		Turbo request pressure (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322189F),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.00152590219,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							}, //77		Turbo request percentage (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221935),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.02,			.replyScaleOffset=-40,	.replyDecimalDigits=1,	.replyMeasurementUnit={0xB0,'C',}						}, //78		Turbo temperature (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322195A),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=-32768, .replyScale=0.001,			.replyScaleOffset=-1,	.replyDecimalDigits=2,	.replyMeasurementUnit={'b','a','r',}					}, //79		Turbo Pressure (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032218A0),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.00152590219,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							}, //80		Turbo percentage (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221959),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=-32768,	.replyScale=0.001,			.replyScaleOffset=-1,	.replyDecimalDigits=1,	.replyMeasurementUnit={'b','a','r',}					}, //81		Boost pressure Request  (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322195B),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.0001,			.replyScaleOffset=0,	.replyDecimalDigits=2,	.replyMeasurementUnit={'V',}							}, //82		Boost sensor voltage (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221947),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.05,			.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'b','a','r',}					}, //83		Rail pressure (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221900),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.02,			.replyScaleOffset=-40,	.replyDecimalDigits=1,	.replyMeasurementUnit={0xB0,'C',}						}, //84		Diesel temperature (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03222002),	.replyId=0x18DAF110,	.replyLen=3,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.1,			.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'k','m',}						}, //85		Odometer Last (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322192F),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.01,			.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'b','a','r',}					}, //86		Air Conditioner pressure (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221942),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.0004,			.replyScaleOffset=0,	.replyDecimalDigits=3,	.replyMeasurementUnit={'L','/','h',}					}, //87		Fuel consume (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322193F),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.02,			.replyScaleOffset=-40,	.replyDecimalDigits=1,	.replyMeasurementUnit={0xB0,'C',}						}, //88		Debimeter temperature (diesel)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221C16),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyFlags=UDS_REPLY_SIGNED,	.replyValOffset=0,	.replyScale=1,		.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,'C',}		}, //89		DID 1C16, hypothesis: charge air cooler coolant temperature (gasoline IAW10JA)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221937),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,	.replyScale=0.001,	.replyScaleOffset=0,	.replyDecimalDigits=2,	.replyMeasurementUnit={'b','a','r',}	}, //90		intake manifold pressure, 1 mbar/count (gasoline IAW10JA)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03223A60),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,	.replyScale=0.125,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'k','g','/','h',}	}, //91		measured air mass, 0.125 kg/h/count (gasoline IAW10JA)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221942),	.replyId=0x18DAF110,	.replyLen=1,	.replyOffset=0,	.replyValOffset=0,	.replyScale=0.1,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'L','/','h',}	}, //92		fuel flow, 0.1 L/h/count, not verified (gasoline IAW10JA)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221891),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,	.replyScale=0.125,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'m','V',}		}, //93		knock sensor cyl 1 (gasoline IAW10JA)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221892),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,	.replyScale=0.125,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'m','V',}		}, //94		knock sensor cyl 2 (gasoline IAW10JA)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221893),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,	.replyScale=0.125,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'m','V',}		}, //95		knock sensor cyl 3 (gasoline IAW10JA)
		{.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221894),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,	.replyScale=0.125,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'m','V',}		}, //96		knock sensor cyl 4 (gasoline IAW10JA)
		[97]={.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322186C),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyFlags=UDS_REPLY_SIGNED,	.replyValOffset=0,	.replyScale=0.0625,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'d','e','g',}	}, //97		spark correction cyl 1, 16 bit signed
		[98]={.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322186D),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyFlags=UDS_REPLY_SIGNED,	.replyValOffset=0,	.replyScale=0.0625,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'d','e','g',}	}, //98		spark correction cyl 2
		[99]={.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322186E),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyFlags=UDS_REPLY_SIGNED,	.replyValOffset=0,	.replyScale=0.0625,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'d','e','g',}	}, //99		spark correction cyl 3
		[100]={.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322186F),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyFlags=UDS_REPLY_SIGNED,	.replyValOffset=0,	.replyScale=0.0625,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'d','e','g',}	}, //100		spark correction cyl 4
		[101]={.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221841),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyFlags=0,	.replyValOffset=0,	.replyScale=0.125,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'m','V',}	}, //101		knock sensor, 16 bit
		[102]={.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221946),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyFlags=0,	.replyValOffset=0,	.replyScale=0.00599,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'b','a','r',}	}, //102		fuel rail pressure, DID 1946
		[104]={.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322186B),	.replyId=0x18DAF110,	.replyLen=1,	.replyOffset=0,	.replyFlags=0,	.replyValOffset=0,	.replyScale=10,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'r','p','m',}	}, //104		idle speed target
		[105]={.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221000),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyFlags=0,	.replyValOffset=0,	.replyScale=0.25,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'r','p','m',}	}, //105		engine speed
		[106]={.reqId=0x18DB33F1,	.reqLen=3,	.reqData=SWAP_UINT32(0x02010600),	.replyId=0x18DAF110,	.replyLen=1,	.replyOffset=0,	.replyFlags=UDS_REPLY_OBD,	.replyValOffset=-128,	.replyScale=0.78125,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'%',}	}, //106		OBD 06 short term fuel trim
		[107]={.reqId=0x18DB33F1,	.reqLen=3,	.reqData=SWAP_UINT32(0x02010700),	.replyId=0x18DAF110,	.replyLen=1,	.replyOffset=0,	.replyFlags=UDS_REPLY_OBD,	.replyValOffset=-128,	.replyScale=0.78125,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'%',}	}, //107		OBD 07 long term fuel trim
		[108]={.reqId=0x18DB33F1,	.reqLen=3,	.reqData=SWAP_UINT32(0x02014600),	.replyId=0x18DAF110,	.replyLen=1,	.replyOffset=0,	.replyFlags=UDS_REPLY_OBD,	.replyValOffset=0,	.replyScale=1,	.replyScaleOffset=-40,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,'C',}	}, //108		OBD 46 ambient air temperature
		[109]={.reqId=0x18DB33F1,	.reqLen=3,	.reqData=SWAP_UINT32(0x02010E00),	.replyId=0x18DAF110,	.replyLen=1,	.replyOffset=0,	.replyFlags=UDS_REPLY_OBD,	.replyValOffset=-128,	.replyScale=0.5,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,}	}, //109		OBD 0E timing advance
		[110]={.reqId=0x18DB33F1,	.reqLen=3,	.reqData=SWAP_UINT32(0x02010400),	.replyId=0x18DAF110,	.replyLen=1,	.replyOffset=0,	.replyFlags=UDS_REPLY_OBD,	.replyValOffset=0,	.replyScale=0.39215686,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'%',}	}, //110		OBD 04 calculated load
		[111]={.reqId=0x18DB33F1,	.reqLen=3,	.reqData=SWAP_UINT32(0x02014400),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyFlags=UDS_REPLY_OBD,	.replyValOffset=0,	.replyScale=0.000030517578,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={' ',}	}, //111		OBD 44 commanded lambda
	
	};

	const char* dpfRegenEnumStrings[] = {
	    "NONE      ",
	    "DPF LO    ",
		"DPF HI    ",
		"NSC De-NOx",
		"NSC De-SOx",
		"SCR HeatUp",
		"NONE.     ",
		"?         "
	};

	const char* setbeltEnumStrings[] = {
	    "ON ",
	    "OFF",
		" ? "
	};

	const uint8_t gearArray[11]={'N','1','2','3','4','5','6','R','7','8','9'};

	const char* speedStatisticEnumStrings[] = {
		"MISSED ",
		"GO     ",
		"?      "
	};

	//decodes a single frame positive reply to service 0x22 for parameter p.
	//Returns 1 and writes the scaled value, or 0 if the frame is not the answer to this parameter
	//(negative response, other DID, too short). Pure function: also built by the host test.
	uint8_t udsDecodeReply(const uds_param_single_element *p, const uint8_t *rx, uint8_t dlc, float *out){
		const uint8_t *req=(const uint8_t *)&p->reqData; //bytes as sent: PCI, 0x22, DID high, DID low
		uint8_t n=p->replyLen;
		if(n>4) n=4;
		uint8_t first=4; //first data byte of a 0x62 reply
		if(p->replyFlags & UDS_REPLY_OBD){ //reply is PCI, 0x41, PID, data
			first=3;
			if(dlc<3+p->replyOffset+p->replyLen || rx[0]>7 || rx[0]<2+p->replyOffset+p->replyLen) return 0;
			if(rx[1]!=0x41 || rx[2]!=req[2]) return 0;
		}else{
			if(dlc<4+p->replyOffset+p->replyLen) return 0;
			if(rx[1]!=0x62 || rx[2]!=req[2] || rx[3]!=req[3]) return 0;
		}
		uint32_t raw=0;
		for(uint8_t i=0;i<n;i++) raw |= ((uint32_t)rx[first+p->replyOffset+i]) << (8*(n-1-i));
		int32_t val=(int32_t)raw;
		if(p->replyFlags & UDS_REPLY_SIGNED){
			if(n==1) val=(int8_t)raw;
			else if(n==2) val=(int16_t)raw;
		}
		val+=p->replyValOffset; //signed sum: the old uint32_t sum wrapped to 4 billions below zero
		*out=((float)val*p->replyScale)+(float)p->replyScaleOffset;
		return 1;
	}

	//warm-up timer: minutes from engine start until the oil reaches 80 C, then the number stays.
	//Called on every 0xFC frame with the tick in ms, so it does not depend on the screen refresh period.
	//A stop shorter than WARMUP_REARM_MS does not restart it. A later start with warm oil keeps the last result.
	static uint32_t warmupStartMs=0, warmupLastRunMs=0;
	static uint16_t warmupTenths=0;
	static uint8_t warmupState=0; //0=never run, 1=counting, 2=done
	void warmupTimerUpdate(uint32_t nowMs, uint32_t rpm, uint8_t oilRaw){
		if(rpm<400) return;
		if(warmupState==0 || (nowMs-warmupLastRunMs>WARMUP_REARM_MS && (warmupState==1 || oilRaw<WARMUP_OIL_RAW))){
			warmupState=1;
			warmupStartMs=nowMs;
		}
		warmupLastRunMs=nowMs;
		if(warmupState==1){
			uint32_t t=(nowMs-warmupStartMs)/6000; //tenths of a minute
			warmupTenths=(t>999) ? 999 : (uint16_t)t;
			if(oilRaw>=WARMUP_OIL_RAW) warmupState=2;
		}
	}
	float warmupTimerMinutes(void){
		return (float)warmupTenths/10.0f;
	}

	//older firmware stored only the bits of its own pages (the rest of the area held stack garbage). Without our
	//mark, every bit past those pages gets the default: shown, except the gasoline pages that put OBD functional
	//requests on the bus. Done with either fuel selected: the same bits serve both after a change of fuel.
	void shownParamsUpgrade(uint8_t *shown, uint16_t mark, uint8_t isDiesel){
		if(mark==SHOWN_PARAMS_MARK) return;
		uint8_t first=isDiesel ? UPSTREAM_PAGES_DIESEL : UPSTREAM_PAGES_GASOLINE;
		if(mark==SHOWN_PARAMS_MARK_72) first=THREE_VALUE_PAGES_FIRST; //the earlier bits are the driver's choice
		for(uint8_t i=first; i<UDS_PAGES_MAX; i++){
			shown[i]=(i>=OBD_PAGES_FIRST && i<=OBD_PAGES_LAST) ? 0 : 1;
		}
	}

	//copies into the stored settings (slot n is slots[n-1]) only what the main menu changes: slots 35, 36, 37 and 39.
	//Everything else keeps the stored value, so Setup menu changes still need SAVE&EXIT.
	uint8_t mergeMainMenuSlots(uint16_t *slots, uint16_t dnaND, uint16_t dnaAR, uint8_t autoScroll, uint8_t cold){
		const uint8_t index[4]={34,35,36,38};
		const uint16_t value[4]={dnaND,dnaAR,autoScroll,cold};
		uint8_t changed=0;
		for(uint8_t i=0;i<4;i++){
			if(slots[index[i]]!=value[i]){ slots[index[i]]=value[i]; changed=1; }
		}
		return changed;
	}

	//Params Setup names of the three value pages, from THREE_VALUE_PAGES_FIRST (their templates are only letters)
	const char * const threeValuePageLabels[7]={ "UAIR OIL WAT", "OIL WAT GBOX", "MAP BOOST IAT", "OILP OIL WAT", "BATT V A %", "GEAR TQ BOOST", "EGT CAT IAT" };
