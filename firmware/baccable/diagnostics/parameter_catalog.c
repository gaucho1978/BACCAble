#include "diagnostics/parameter_catalog.h"

float displayed_parameter_values[2];

uint8_t parameter_page_visibility[240];
uint8_t parameter_setup_page_index = 0;
uint8_t parameter_page_count = 0;

uint8_t gasoline_page_count = 46;
uint8_t diesel_page_count = 55;
uint8_t selected_parameter_element = 0;

// Costruzione della stringa
// dashboard_format_values(template, power, torque, result);

// format string $x.yf for float params where y is decimal part and x is integer part
// format string $enum for enumerator values derived from specific enum arrays
const ParameterPage parameter_pages[2][60] = {
    {
        // Gasoline
        {.name = "PWR $3.0fCV $3.0fNm", .parameter_ids = {1, 2}}, // param couple: PWR and Torque
        {.name = "OIL $1.1fbar W.$3.0f"
                 "\xB0"
                 "C",
         .parameter_ids = {0, 42}}, // param couple: OIL pressure and Water Temp.
        {.name = "OIL $1.1fbar O.$3.0f"
                 "\xB0"
                 "C",
         .parameter_ids = {0, 5}}, // param couple: OIL pressure and Oil Temp.
        {.name = "OIL $3.0f"
                 "\xB0"
                 "C W.$3.0f"
                 "\xB0"
                 "C",
         .parameter_ids = {5, 42}}, // param couple: OIL temp. and Water Temp.
        {.name = "OIL $1.1fL Qual.$3.0f%",
         .parameter_ids = {28, 31}}, // param couple: OIL level and Oil Quality
        {.name = "BAT $3.0f% $2.1fA",
         .parameter_ids = {3, 4}}, // param couple: BAT State Of Charge and current
        {.name = "BAT $2.2fV $2.1fA", .parameter_ids = {35, 4}}, // param couple: BAT voltage and current
        {.name = "PWR: $3.2fCV   ", .parameter_ids = {1, 1}},    // Power
        {.name = "TORQUE: $3.2fNm", .parameter_ids = {2, 2}},    // Torque
        {.name = "IC AirOut: $3.1f"
                 "\xB0"
                 "C",
         .parameter_ids = {22, 22}}, // Intercooler output air temperature
        {.name = "IC AirIn:  $3.1f"
                 "\xB0"
                 "C",
         .parameter_ids = {23, 23}},                                // Intercooler input  air temperature
        {.name = "BOOST ABS: $2.1fbar", .parameter_ids = {24, 24}}, // Boost Absolute Pressure
        {.name = "BOOST: $3.1fbar",
         .parameter_ids = {25, 25}}, // Boost Pressure calculated from Absolute pressure
        {.name = "TURBO: $2.3fV  ", .parameter_ids = {26, 26}}, // Turbo Sensor Voltage
        {.name = "ODOM.LAST:$6.0fkm",
         .parameter_ids = {27, 27}}, // distance since last time odometer was zeroized
        {.name = "OIL: $1.3fL    ", .parameter_ids = {28, 28}}, // Oil Quantity
        {.name = "OIL: $1.3fbar  ", .parameter_ids = {29, 29}}, // Oil Pressure
        {.name = "OIL: $3.0f"
                 "\xB0"
                 "C   ",
         .parameter_ids = {30, 30}},                              // Oil Temperature
        {.name = "OIL QUALY: $3.0f%", .parameter_ids = {31, 31}}, // Oil Quality
        {.name = "OIL UnAir: $2.2f"
                 "\xB0"
                 "C",
         .parameter_ids = {32, 32}}, // Multiair Module Oil Temperature
        {.name = "GEARBOX: $2.2f"
                 "\xB0"
                 "C",
         .parameter_ids = {33, 33}},                                // Gearbox Temperature
        {.name = "BATT.: $3.0f%  ", .parameter_ids = {34, 34}},     // Battery State Of Charge
        {.name = "BATT.: $3.3fA  ", .parameter_ids = {4, 4}},       // Battery Current
        {.name = "BATT.: $3.3fV  ", .parameter_ids = {35, 35}},     // Battery Voltage
        {.name = "AIR COND.: $3.3fbar", .parameter_ids = {36, 36}}, // Air Conditioner Pressure
        {.name = "CUR. GEAR: $enum", .parameter_ids = {6, 6}},      // Current Gear
        {.name = "Time ON: $6.0fmin", .parameter_ids = {37, 37}},   // Time Since engine on
        {.name = "OVER RPM: $3.0fsec",
         .parameter_ids = {38, 38}}, // elapsed time in engine overspeed condition
        {.name = "OVER RPM: $6.0f",
         .parameter_ids = {39, 39}}, // number of times of engine overspeed condition
        {.name = "EXHAUST GAS:$4.0f"
                 "\xB0"
                 "C",
         .parameter_ids = {40, 40}}, // Exaust gas temperature
        {.name = "CATAL.: $4.1f"
                 "\xB0"
                 "C",
         .parameter_ids = {41, 41}}, // catalytic converter temperature sensor
        {.name = "WATER: $3.2f"
                 "\xB0"
                 "C ",
         .parameter_ids = {42, 42}},                                 // water temperature
        {.name = "KNOCK: $4.3fmV ", .parameter_ids = {43, 43}},      // head knock sensor voltage
        {.name = "KEY ID: $4.0f  ", .parameter_ids = {44, 44}},      // inserted Key ID
        {.name = "SparkCYL1: $1.2fdeg", .parameter_ids = {45, 45}},  // Cylinder correction
        {.name = "SparkCYL2: $1.2fdeg", .parameter_ids = {46, 46}},  // Cylinder correction
        {.name = "SparkCYL3: $1.2fdeg", .parameter_ids = {47, 47}},  // Cylinder correction
        {.name = "SparkCYL4: $1.2fdeg", .parameter_ids = {48, 48}},  // Cylinder correction
        {.name = "DRIVE STYLE: $enum", .parameter_ids = {15, 15}},   // Drive Style
        {.name = "SPEED: $3.3fkm/h", .parameter_ids = {7, 7}},       // Speed
        {.name = "Seatbelt Alarm:$enum", .parameter_ids = {13, 13}}, // SeatBelt Alarm
        {.name = "  0-100Km/h $1.3fs", .parameter_ids = {9, 9}},     // 0-100km/h time statistic
        {.name = "100-200Km/h $1.3fs", .parameter_ids = {10, 10}},   // 100-200km/h time statistic
        {.name = "Best  0-100: $1.2fs", .parameter_ids = {11, 11}},  // 0-100km/h Best time statistic
        {.name = "Best100-200: $1.2fs", .parameter_ids = {12, 12}},  // 100-200km/h Best time statistic
        {.name = "Pedal Map: $enum",
         .parameter_ids =
             {17, 17}}, // selected Pedal Map
                        //					{.name="RAM: $5.0fB",
                        //.parameter_ids={16,	16		}}, //Free RAM
                        //									{.name={'T',
                        //'Y', 'R', 'E', '
                        //', 'R', 'F', ':', ' ',},              .request_id=0x18DAC7F1,  .request_length=4,
                        //.request_data=SWAP_UINT32(0x022240B3),   .response_id=0x18DAF1C7, .value_length=1,
                        //.value_offset=4, .raw_offset=-50,    .scale=1,              .scaled_offset=0,
                        //.decimal_places=1, .unit={0xB0,'C'}                        }, // LIMITE BYTE
                        //									{.name={'T',
                        //'Y', 'R', 'E', '
                        //', 'R', 'R', ':', ' ',},              .request_id=0x18DAC7F1,  .request_length=4,
                        //.request_data=SWAP_UINT32(0x022230B4),   .response_id=0x18DAF1C7, .value_length=1,
                        //.value_offset=4, .raw_offset=-50,    .scale=1,              .scaled_offset=0,
                        //.decimal_places=1, .unit={0xB0,'C'}                        }, // LIMITE BYTE
                        //									{.name={'T',
                        //'Y', 'R', 'E', '
                        //', 'L', 'F', ':', ' ',},              .request_id=0x18DAC7F1,  .request_length=4,
                        //.request_data=SWAP_UINT32(0x022240B2),   .response_id=0x18DAF1C7, .value_length=1,
                        //.value_offset=4, .raw_offset=-50,    .scale=1,              .scaled_offset=0,
                        //.decimal_places=1, .unit={0xB0,'C'}                        }, // LIMITE BYTE
                        //									{.name={'T',
                        //'Y', 'R', 'E', '
                        //', 'L', 'F', ':', ' ',},              .request_id=0x18DAC7F1,  .request_length=4,
                        //.request_data=SWAP_UINT32(0x022240B1),   .response_id=0x18DAF1C7, .value_length=1,
                        //.value_offset=4, .raw_offset=-50,    .scale=1,              .scaled_offset=0,
                        //.decimal_places=1, .unit={0xB0,'C'}                        }, // LIMITE BYTE
                        // unit grams/km (wrong?)
                        // {.name={'P','A','R','T','I','C','U','L','.', ':','
                        // ',},				.request_id=0x18DA10F1,	.request_length=4,
                        // .request_data=SWAP_UINT32(0x032218AA),	.response_id=0x18DAF110,
                        // .value_length=2, .value_offset=0, .raw_offset=0,		.scale=0.001,
                        // .scaled_offset=0, .decimal_places=0,	.unit={'g','/','k','m'}
                        // }, // GRAMMI PER KM just to print stuff for debug:	{.name={'D',},
                        // .request_id=0x1F,		.request_length=4,
                        // .request_data=SWAP_UINT32(0x00000000), .response_id=0x00000000,
                        // .value_length=2,	.value_offset=0, .raw_offset=0, .scale=1,
                        // .scaled_offset=0,	.decimal_places=2,	.unit={'s', }
                        // }, //debug string

    },
    {
        // diesel
        {.name = "PWR $3.0fCV $3.0fNm", .parameter_ids = {1, 2}}, // param couple: PWR and Torque
        {.name = "OIL $1.1fbar W.$3.0f"
                 "\xB0"
                 "C",
         .parameter_ids = {0, 68}}, // param couple: OIL pressure and Water Temp.
        {.name = "OIL $1.1fbar O.$3.0f"
                 "\xB0"
                 "C",
         .parameter_ids = {0, 5}}, // param couple: OIL pressure and Oil Temp.
        {.name = "OIL $3.0f"
                 "\xB0"
                 "C W.$3.0f"
                 "\xB0"
                 "C",
         .parameter_ids = {5, 68}}, // param couple: OIL temp. and Water Temp.
        {.name = "OIL $2.1fmm Qu.$3.0f%",
         .parameter_ids = {64, 63}}, // param couple: OIL level and Oil Quality
        {.name = "BAT $3.0f% $2.1fA",
         .parameter_ids = {3, 4}}, // param couple: BAT State Of Charge and current
        {.name = "BAT $2.2fV $2.1fA", .parameter_ids = {62, 4}}, // param couple: BAT voltage and current
        {.name = "DPF $2.2f% $2.2f"
                 "\xB0"
                 "C",
         .parameter_ids = {55, 56}}, // param couple: DPF clogging percentage and temperature
        {.name = "REGEN $2.1f% $3.0f"
                 "\xB0"
                 "C",
         .parameter_ids = {57, 56}}, // param couple: DPF regeneration progress percentage and temperature
        {.name = "PWR: $3.2fCV   ", .parameter_ids = {1, 1}},   // Power
        {.name = "TORQUE: $3.2fNm", .parameter_ids = {2, 2}},   // Torque
        {.name = "DPF: $3.2f%    ", .parameter_ids = {55, 55}}, // DPF clogging percentage
        {.name = "DPF: $3.2f"
                 "\xB0"
                 "C   ",
         .parameter_ids = {56, 56}},                               // DPF Temperature
        {.name = "DPF REGEN: $3.2f%", .parameter_ids = {57, 57}},  // DPF regeneration progress percentage
        {.name = "REGEN: $enum   ", .parameter_ids = {8, 8}},      // DPF regeneration type
        {.name = "LAST REGEN:$5.0fkm", .parameter_ids = {58, 58}}, // DPF last regeneration distance in km
        {.name = "TOT REGEN: $5.0f", .parameter_ids = {59, 59}},   // DPF total number of regenerations
        {.name = "MEAN REGEN:$5.0fkm", .parameter_ids = {60, 60}}, // DPF mean regeneration distance in km
        {.name = "MEAN REGEN:$3.0fmin",
         .parameter_ids = {61, 61}},                              // DPF mean regeneration duration in minutes
        {.name = "BAT $2.2fV     ", .parameter_ids = {62, 62}},   // Battery Voltage
        {.name = "BAT $3.0f%     ", .parameter_ids = {3, 3}},     // Battery State Of Charge percentage
        {.name = "BAT $3.1fA     ", .parameter_ids = {4, 4}},     // Battery current
        {.name = "OIL QUALY: $3.0f%", .parameter_ids = {63, 63}}, // Oil Quality
        {.name = "OIL: $3.0f"
                 "\xB0"
                 "C   ",
         .parameter_ids = {5, 5}},                              // Oil temperature
        {.name = "OIL: $2.2fbar  ", .parameter_ids = {0, 0}},   // Oil pressure
        {.name = "OIL: $3.2fmm   ", .parameter_ids = {64, 64}}, // Oil quantity in mm
        {.name = "ADBLUE: $3.2fL ", .parameter_ids = {65, 65}}, // Adblue quantity in Liters
        {.name = "ADBLUE: $3.2f% ", .parameter_ids = {66, 66}}, // Adblue quantity in percentage
        {.name = "GEARBOX: $3.2f"
                 "\xB0"
                 "C",
         .parameter_ids = {33, 33}}, // gearbox temperature
        {.name = "EXHAUST GAS:$4.0f"
                 "\xB0"
                 "C",
         .parameter_ids = {67, 67}},                           // exhaust gas temperature (turbo input)
        {.name = "CUR. GEAR: $enum", .parameter_ids = {6, 6}}, // current gear
        {.name = "WATER: $3.0f"
                 "\xB0"
                 "C ",
         .parameter_ids = {68, 68}},                                // water temperature
        {.name = "EGR CMD:$2.2f% ", .parameter_ids = {73, 73}},     // EGR command
        {.name = "EGR:    $2.2f% ", .parameter_ids = {74, 74}},     // EGR status
        {.name = "TURBO REQ: $2.1fbar", .parameter_ids = {76, 76}}, // Turbo Request pressure
        {.name = "TURBO REQ: $2.2f%", .parameter_ids = {77, 77}},   // Turbo Request percentage
        {.name = "TURBO: $2.2f"
                 "\xB0"
                 "C ",
         .parameter_ids = {78, 78}},                                // Turbo temperature
        {.name = "TURBO: $2.2fbar", .parameter_ids = {79, 79}},     // Turbo pressure
        {.name = "TURBO: $2.2f%  ", .parameter_ids = {80, 80}},     // Turbo percentage
        {.name = "BOOST REQ.:$2.1fbar", .parameter_ids = {81, 81}}, // Boost Request pressure
        {.name = "BOOST: $1.2fV  ", .parameter_ids = {82, 82}},     // Boost sensor voltage
        {.name = "RAIL: $5.2fbar ", .parameter_ids = {83, 83}},     // Rail pressure
        {.name = "DIESEL: $2.2f"
                 "\xB0"
                 "C",
         .parameter_ids = {84, 84}},                               // Diesel temperature
        {.name = "ODOM.LAST: $5.0fkm", .parameter_ids = {85, 85}}, // Distance in km since last odometer reset
        {.name = "AIR COND.:$2.2fbar", .parameter_ids = {86, 86}}, // Air conditioner pressure
        {.name = "FUEL CONS.:$1.2fL/h", .parameter_ids = {87, 87}}, // Fuel Consume
        {.name = "DEBIMETER:$3.2f"
                 "\xB0"
                 "C",
         .parameter_ids = {88, 88}},                                 // Debimeter temperature
        {.name = "SPEED:$3.2fkm/h", .parameter_ids = {7, 7}},        // speed
        {.name = "Seatbelt Alarm:$enum", .parameter_ids = {13, 13}}, // Seatbelt Alarm Status
        {.name = "0-100Km/h:  $2.2fs", .parameter_ids = {9, 9}},     // 0-100km/h Statistic
        {.name = "100-200Km/h:$2.2fs", .parameter_ids = {10, 10}},   // 100-200km/h Statistic
        {.name = "Best  0-100:$2.2fs", .parameter_ids = {11, 11}},   // 0-100km/h Best Statistic
        {.name = "Best100-200:$2.2fs", .parameter_ids = {12, 12}},   // 0-100km/h Best Statistic
        {.name = "DRIVE STYLE: $enum", .parameter_ids = {15, 15}},   // Drive Style
        {.name = "Pedal Map: $enum",
         .parameter_ids = {17, 17}}, // selected Pedal Map
                                     //						{.name="RAM: $5.0fB",
                                     //.parameter_ids={16,	16		}}, //Free RAM

        //								{.name={'F','-','L','
        //','T','I','R','E',':',' ',},					.request_id=0x18DAC7F1,
        //.request_length=4,	.request_data=SWAP_UINT32(0x032240B1),	.response_id=0x18DAF1C7,
        //.value_length=1,	.value_offset=4, .raw_offset=-50,	.scale=1,
        //.scaled_offset=0,	.decimal_places=0,	.unit={0xB0,'C',}
        //},
        //								{.name={'F','-','R','
        //','T','I','R','E',':',' ',},					.request_id=0x18DAC7F1,
        //.request_length=4,	.request_data=SWAP_UINT32(0x032240B2),	.response_id=0x18DAF1C7,
        //.value_length=1,	.value_offset=4, .raw_offset=-50,	.scale=1,
        //.scaled_offset=0,	.decimal_places=0,	.unit={0xB0,'C',}
        //},
        //								{.name={'R','-','L','
        //','T','I','R','E',':',' ',},					.request_id=0x18DAC7F1,
        //.request_length=4,	.request_data=SWAP_UINT32(0x032240B3),	.response_id=0x18DAF1C7,
        //.value_length=1,	.value_offset=4, .raw_offset=-50,	.scale=1,
        //.scaled_offset=0,	.decimal_places=0,	.unit={0xB0,'C',}
        //},
        //								{.name={'R','-','R','
        //','T','I','R','E',':',' ',},					.request_id=0x18DAC7F1,
        //.request_length=4,	.request_data=SWAP_UINT32(0x032240B4),	.response_id=0x18DAF1C7,
        //.value_length=1,	.value_offset=4, .raw_offset=-50,	.scale=1,
        //.scaled_offset=0,	.decimal_places=0,	.unit={0xB0,'C',}
        //},
        // may be not received			{.name={'E','G','R',' ','C','M','D','1',':',},
        // .request_id=0x18DB33F1,	.request_length=4,	.request_data=SWAP_UINT32(0x0322012C),
        // .response_id=0x18DBF133,	.value_length=1,	.value_offset=0, .raw_offset=0,
        // .scale=0.3921568627,	.scaled_offset=0,	.decimal_places=1,	.unit={'%',}
        // },
        // sometimes wrong value			{.name={'E','G','R',' ','M','E','A','S','.', ':','
        // ',},				.request_id=0x18DA10F1,	.request_length=4,
        // .request_data=SWAP_UINT32(0x0322189C),	.response_id=0x18DAF110,	.value_length=2,
        // .value_offset=0, .raw_offset=-32767,	.scale=0.00305185095,	.scaled_offset=0,
        // .decimal_places=1,	.unit={'%',}							},
        // wrong value 43520g			{.name={'P','A','R','T','I','C','U','L','.', ':',' ',},
        // .request_id=0x18DA10F1,	.request_length=4,	.request_data=SWAP_UINT32(0x032218AA),
        // .response_id=0x18DAF110,	.value_length=2,	.value_offset=0, .raw_offset=0,
        // .scale=1,				.scaled_offset=0,	.decimal_places=0,	.unit={'g',}
        // },
        // may be not received			{.name={'T','U','R','B','O','1',':',},
        // .request_id=0x18DB33F1,	.request_length=4,	.request_data=SWAP_UINT32(0x03220175),
        // .response_id=0x18DBF133,	.value_length=2,	.value_offset=5,	.raw_offset=0,
        // .scale=0.1,			.scaled_offset=-40,	.decimal_places=1,	.unit={0xB0,'C',}
        // },
        // may be not received			{.name={'T','U','R','B','O','4',':',},
        // .request_id=0x18DB33F1,	.request_length=4,	.request_data=SWAP_UINT32(0x0322010B),
        // .response_id=0x18DBF133,	.value_length=1,	.value_offset=0,	.raw_offset=-100,
        // .scale=0.01,			.scaled_offset=0,	.decimal_places=1,	.unit={'b','a','r',}
        // },
        // stuck to 3,18V				{.name={'T','U','R','B','O','5',':',},
        // .request_id=0x18DA10F1,	.request_length=4,	.request_data=SWAP_UINT32(0x03221936),
        // .response_id=0x18DAF110,	.value_length=2,	.value_offset=0,	.raw_offset=0,
        // .scale=0.0001,			.scaled_offset=0,	.decimal_places=2,	.unit={'V',}
        // },
        // may be not received			{.name={'F','U','E','L',':',' ',},
        // .request_id=0x18DB33F1,	.request_length=4,	.request_data=SWAP_UINT32(0x03220123),
        // .response_id=0x18DBF133,	.value_length=2,	.value_offset=0, .raw_offset=0,
        // .scale=10,				.scaled_offset=0,	.decimal_places=0,
        // .unit={'k','P','a',}					},
        // just to print stuff for debug:{.name={'D',},
        // .request_id=0x1F,		.request_length=4,	.request_data=SWAP_UINT32(0x00000000),
        // .response_id=0x00000000,	.value_length=2,	.value_offset=0, .raw_offset=0,
        // .scale=1,				.scaled_offset=0,	.decimal_places=2,	.unit={'s', }
        // }, //debug string

    }};

const	ParameterDefinition parameter_definitions[100]={
		{.request_id=0x10,
        .request_length=4,
        .request_data=SWAP_UINT32(0x00000000),
        .response_id=0x000004B2,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.1,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'b','a','r',}					}, //0		oil pressure
		{.request_id=0x11,
        .request_length=4,
        .request_data=SWAP_UINT32(0x00000000),
        .response_id=0x000000FB,
        .value_length=2,
        .value_offset=0,
        .raw_offset=-500,
        .scale=0.000142378,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'C','V',}						}, //1		power
		{.request_id=0x12,
        .request_length=4,
        .request_data=SWAP_UINT32(0x00000000),
        .response_id=0x000000FB,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=-500,
        .decimal_places=0,
        .unit={'N','m',}						}, //2		torque
		{.request_id=0x13,
        .request_length=4,
        .request_data=SWAP_UINT32(0x00000000),
        .response_id=0x0000041A,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'%',}							}, //3		Battery percentage
		{.request_id=0x14,
        .request_length=4,
        .request_data=SWAP_UINT32(0x00000000),
        .response_id=0x0000041A,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.1,
        .scaled_offset=-250,
        .decimal_places=2,
        .unit={'A',}							}, //4		battery current
		{.request_id=0x15,
        .request_length=4,
        .request_data=SWAP_UINT32(0x00000000),
        .response_id=0x000004B2,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=-40,
        .decimal_places=0,
        .unit={0xB0,'C',}						}, //5		oil temperature

		{.request_id=0x17,
        .request_length=4,
        .request_data=SWAP_UINT32(0x00000000),
        .response_id=0x000002EF,
        .value_length=1,
        .value_offset=0,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=0,
        .unit={' ',}							}, //6		current gear
		{.request_id=0x18,
        .request_length=4,
        .request_data=SWAP_UINT32(0x00000000),
        .response_id=0x00000101,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.0625,
        .scaled_offset=0,
        .decimal_places=2,
        .unit={'k','m','/','h', }				}, //7		speed
		{.request_id=0x19,
        .request_length=4,
        .request_data=SWAP_UINT32(0x00000000),
        .response_id=0x000005AE,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=0,
        .unit={' ',}							}, //8		DPF Regeneration type
		{.request_id=0x1A,
        .request_length=4,
        .request_data=SWAP_UINT32(0x00000000),
        .response_id=0x00000000,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=2,
        .unit={'s', }							}, //9		statistic 0/100
		{.request_id=0x1B,
        .request_length=4,
        .request_data=SWAP_UINT32(0x00000000),
        .response_id=0x00000000,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=2,
        .unit={'s', }							}, //10		statistic 100/200
		{.request_id=0x1C,
        .request_length=4,
        .request_data=SWAP_UINT32(0x00000000),
        .response_id=0x00000000,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=2,
        .unit={'s', }							}, //11		best statistic 0/100
		{.request_id=0x1D,
        .request_length=4,
        .request_data=SWAP_UINT32(0x00000000),
        .response_id=0x00000000,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=2,
        .unit={'s', }							}, //12		best statistic 100/200
		{.request_id=0x1E,
        .request_length=4,
        .request_data=SWAP_UINT32(0x032255A0),
        .response_id=0x18DAF160,
        .value_length=1,
        .value_offset=0,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=0,
        .unit={' ',}							}, //13		seat belt alarm
		{.request_id=0x1F,
        .request_length=4,
        .request_data=SWAP_UINT32(0x00000000),
        .response_id=0x00000000,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=2,
        .unit={'s', }							}, //14		debug string
		{.request_id=0x20,
        .request_length=4,
        .request_data=SWAP_UINT32(0x00000000),
        .response_id=0x00000000,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=2,
        .unit={' ', }							}, //15		Drive Style
		{.request_id=0x21,
        .request_length=4,
        .request_data=SWAP_UINT32(0x00000000),
        .response_id=0x00000000,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=0,
        .unit={'B', }							}, //16		Free RAM
		{.request_id=0x22,
        .request_length=4,
        .request_data=SWAP_UINT32(0x00000000),
        .response_id=0x00000000,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=0,
        .unit={' ', }							}, //17		Selected Pedal Map
		{																																																																						}, //18
		{																																																																						}, //19
		{																																																																						}, //20
		{																																																																						}, //21
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03221935),
        .response_id=0x18DAF110,
        .value_length=1,
        .value_offset=0,
        .raw_offset=-40,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={0xB0,'C'}						}, //22		intercooler air out (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03223A58),
        .response_id=0x18DAF110,
        .value_length=1,
        .value_offset=0,
        .raw_offset=-40,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={0xB0,'C'}						}, //23		intercooler air in (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322195A),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=-1,
        .scale=0.001,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'B','A','R'}						}, //24		boost absolute pressure (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322195A),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=-1,
        .scale=0.001,
        .scaled_offset=-1,
        .decimal_places=1,
        .unit={'B','A','R'}						}, //25		boost pressure extracted from absolute pressure (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03221936),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.0001,
        .scaled_offset=0,
        .decimal_places=2,
        .unit={'V',}							}, //26		turbo (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03222002),
        .response_id=0x18DAF110,
        .value_length=3,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.1,
        .scaled_offset=0,
        .decimal_places=0,
        .unit={'k','m',}						}, //27		odometer last (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03223A41),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.001,
        .scaled_offset=0,
        .decimal_places=3,
        .unit={'L',},							}, //28		oil quantity (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322130A),
        .response_id=0x18DAF110,
        .value_length=1,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.039215686,
        .scaled_offset=0,
        .decimal_places=2,
        .unit={'B','a','r',}					}, //29		oil pressure (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03221302),
        .response_id=0x18DAF110,
        .value_length=1,
        .value_offset=1,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=0,
        .unit={0xB0,'C',}						}, //30		oil temperature (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03223813),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.0015259022,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'%',}							}, //31		oil quality (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322198E),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.0625,
        .scaled_offset=-40,
        .decimal_places=2,
        .unit={0xB0,'C',}						}, //32		multiair module oil temperature (gasoline)
		{.request_id=0x18DA18F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x032204FE),
        .response_id=0x18DAF118,
        .value_length=1,
        .value_offset=0,
        .raw_offset=-40,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={0xB0,'C'}						}, //33		gearbox temperature
		{.request_id=0x18DA40F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03221005),
        .response_id=0x18DAF140,
        .value_length=2,
        .value_offset=1,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'%',}							}, //34		battery percentage (gasoline)
		{.request_id=0x18DA40F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03221004),
        .response_id=0x18DAF140,
        .value_length=1,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.1,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'V',}							}, //35		battery voltage (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322192F),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.01,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'b','a','r',}					}, //36		air conditioner pressure (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03221009),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.25,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'m',}                            }, //37		time since engine ON (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03222006),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.01,
        .scaled_offset=0,
        .decimal_places=2,
        .unit={'s',}							}, //38		time spent in engine overspeed (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03222004),
        .response_id=0x18DAF110,
        .value_length=1,
        .value_offset=0,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=0,
        .unit={' ',}							}, //39		engine overspeed number of times (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x032218BA),
        .response_id=0x18DAF110,
        .value_length=1,
        .value_offset=0,
        .raw_offset=0,
        .scale=5,
        .scaled_offset=-50,
        .decimal_places=0,
        .unit={0xB0,'C',}                       }, //40		exhaust gas temperature (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03221837),
        .response_id=0x18DAF110,
        .value_length=1,
        .value_offset=0,
        .raw_offset=0,
        .scale=5,
        .scaled_offset=-50,
        .decimal_places=1,
        .unit={0xB0,'C',}                       }, //41		Catalytic sensor temperature (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03221003),
        .response_id=0x18DAF110,
        .value_length=1,
        .value_offset=0,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=-40,
        .decimal_places=1,
        .unit={0xB0,'C',}						}, //42		Water temperature (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03221841),
        .response_id=0x18DAF110,
        .value_length=1,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.125,
        .scaled_offset=0,
        .decimal_places=3,
        .unit={'m','V',}						}, //43		head knock (gasoline)
		{.request_id=0x18DA40F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03220131),
        .response_id=0x18DAF140,
        .value_length=1,
        .value_offset=0,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=0,
        .unit={' ',}							}, //44		Key ID (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322186C),
        .response_id=0x18DAF110,
        .value_length=1,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.0625,
        .scaled_offset=0,
        .decimal_places=3,
        .unit={'d','e','g',}                    }, //45		cylinder1 correction (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322186D),
        .response_id=0x18DAF110,
        .value_length=1,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.0625,
        .scaled_offset=0,
        .decimal_places=3,
        .unit={'d','e','g',}					}, //46		cylinder2 correction (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322186E),
        .response_id=0x18DAF110,
        .value_length=1,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.0625,
        .scaled_offset=0,
        .decimal_places=3,
        .unit={'d','e','g',}					}, //47		cylinder3 correction (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322186F),
        .response_id=0x18DAF110,
        .value_length=1,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.0625,
        .scaled_offset=0,
        .decimal_places=3,
        .unit={'d','e','g',}					}, //48		cylinder4 correction (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x032218F0),
        .response_id=0x18DAF110,
        .value_length=1,
        .value_offset=0,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=0,
        .unit={' ',}							}, //49		drive style selector position (gasoline)
		{.request_id=0x18DAC7F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x022240B3),
        .response_id=0x18DAF1C7,
        .value_length=1,
        .value_offset=4,
        .raw_offset=-50,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={0xB0,'C'}						}, //50		front right tyre pressure (not implemented now)
		{.request_id=0x18DAC7F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x022230B4),
        .response_id=0x18DAF1C7,
        .value_length=1,
        .value_offset=4,
        .raw_offset=-50,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={0xB0,'C'}						}, //51		rear right tyre pressure (not implemented now)
		{.request_id=0x18DAC7F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x022240B2),
        .response_id=0x18DAF1C7,
        .value_length=1,
        .value_offset=4,
        .raw_offset=-50,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={0xB0,'C'}						}, //52		front left tyre pressure (not implemented now)
		{.request_id=0x18DAC7F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x022240B1),
        .response_id=0x18DAF1C7,
        .value_length=1,
        .value_offset=4,
        .raw_offset=-50,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={0xB0,'C'}						}, //53		rear left tyre pressure (not implemented now)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x032218AA),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.001,
        .scaled_offset=0,
        .decimal_places=0,
        .unit={'g','/','k','m'}					}, //54		particulate (gasoline)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x032218E4),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.015259022,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'%',}							}, //55		DPF clogging percentage (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x032218DE),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.02,
        .scaled_offset=-40,
        .decimal_places=1,
        .unit={0xB0,'C',}						}, //56		DPF temperature (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322380B),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.001525902,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'%',}							}, //57		DPF regeneration progress percentage (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03223807),
        .response_id=0x18DAF110,
        .value_length=3,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.1,
        .scaled_offset=0,
        .decimal_places=0,
        .unit={'k','m',}						}, //58		last regeneration (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x032218A4),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=0,
        .unit={' ',}							}, //59		total regenerations number (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03223809),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=0,
        .unit={'k','m',}						}, //60		mean regeneration (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322380A),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.01666666666,
        .scaled_offset=0,
        .decimal_places=0,
        .unit={'m','i','n',}					}, //61		mean regeneration duration (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03221955),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.0005,
        .scaled_offset=0,
        .decimal_places=3,
        .unit={'V',}							}, //62		battery voltage (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03223813),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.0015259022,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'%',}							}, //63		oil quality (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322194E),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.1,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'m','m',}						}, //64		oil level [mm in oil pan (50-70mmm)] (diesel)
		{.request_id=0x18DA01F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322D930),
        .response_id=0x18DAF101,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.00097676774,
        .scaled_offset=0,
        .decimal_places=2,
        .unit={'L',}							}, //65		adblue level in liters (diesel)
		{.request_id=0x18DA01F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322D97C),
        .response_id=0x18DAF101,
        .value_length=1,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.390625,
        .scaled_offset=0,
        .decimal_places=2,
        .unit={'%',}							}, //66		adblue level in % (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03223836),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.02,
        .scaled_offset=-40,
        .decimal_places=1,
        .unit={0xB0,'C',}                       }, //67		exhaust gas temperature (turbo input) (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03221003),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.02,
        .scaled_offset=-40,
        .decimal_places=1,
        .unit={0xB0,'C',}						}, //68		water temperature (diesel)
		{.request_id=0x18DAC7F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x032240B1),
        .response_id=0x18DAF1C7,
        .value_length=1,
        .value_offset=4,
        .raw_offset=-50,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=0,
        .unit={0xB0,'C',}						}, //69		from left tire temperature (not implemented) (diesel)
		{.request_id=0x18DAC7F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x032240B2),
        .response_id=0x18DAF1C7,
        .value_length=1,
        .value_offset=4,
        .raw_offset=-50,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=0,
        .unit={0xB0,'C',}						}, //70		front right tire temperature (diesel)
		{.request_id=0x18DAC7F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x032240B3),
        .response_id=0x18DAF1C7,
        .value_length=1,
        .value_offset=4,
        .raw_offset=-50,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=0,
        .unit={0xB0,'C',}						}, //71		rear left tire temperature (diesel)
		{.request_id=0x18DAC7F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x032240B4),
        .response_id=0x18DAF1C7,
        .value_length=1,
        .value_offset=4,
        .raw_offset=-50,
        .scale=1,
        .scaled_offset=0,
        .decimal_places=0,
        .unit={0xB0,'C',}						}, //72		rear right tire temperature (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322189B),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=-32767,
        .scale=0.00305185095,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'%',}							}, //73		EGR command (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322189A),
        .response_id=0x18DAF110,
        .value_length=1,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.1953125,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'%',}							}, //74		EGR status (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322189C),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=-32767,
        .scale=0.00305185095,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'%',}							}, //75		EGR measured - sometimes shows wrong value - not used (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03221942),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.000030517578,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'b','a','r',}					}, //76		Turbo request pressure (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322189F),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.00152590219,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'%',}							}, //77		Turbo request percentage (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03221935),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.02,
        .scaled_offset=-40,
        .decimal_places=1,
        .unit={0xB0,'C',}						}, //78		Turbo temperature (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322195A),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=-32768,
        .scale=0.001,
        .scaled_offset=-1,
        .decimal_places=2,
        .unit={'b','a','r',}					}, //79		Turbo Pressure (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x032218A0),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.00152590219,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'%',}							}, //80		Turbo percentage (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03221959),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=-32768,
        .scale=0.001,
        .scaled_offset=-1,
        .decimal_places=1,
        .unit={'b','a','r',}					}, //81		Boost pressure Request  (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322195B),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.0001,
        .scaled_offset=0,
        .decimal_places=2,
        .unit={'V',}							}, //82		Boost sensor voltage (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03221947),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.05,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'b','a','r',}					}, //83		Rail pressure (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03221900),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.02,
        .scaled_offset=-40,
        .decimal_places=1,
        .unit={0xB0,'C',}						}, //84		Diesel temperature (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03222002),
        .response_id=0x18DAF110,
        .value_length=3,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.1,
        .scaled_offset=0,
        .decimal_places=0,
        .unit={'k','m',}						}, //85		Odometer Last (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322192F),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.01,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'b','a','r',}					}, //86		Air Conditioner pressure (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x03221942),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.0000394789,
        .scaled_offset=0,
        .decimal_places=1,
        .unit={'L','/','h',}					}, //87		Fuel consume (diesel)
		{.request_id=0x18DA10F1,
        .request_length=4,
        .request_data=SWAP_UINT32(0x0322193F),
        .response_id=0x18DAF110,
        .value_length=2,
        .value_offset=0,
        .raw_offset=0,
        .scale=0.02,
        .scaled_offset=-40,
        .decimal_places=1,
        .unit={0xB0,'C',}						}, //88		Debimeter temperature (diesel)

	};

const char *regeneration_labels[] = {"NONE      ", "DPF LO    ", "DPF HI    ", "NSC De-NOx",
                                     "NSC De-SOx", "SCR HeatUp", "NONE.     ", "?         "};

const char *seatbelt_labels[] = {"ON ", "OFF", " ? "};

const uint8_t gear_symbols[11] = {'N', '1', '2', '3', '4', '5', '6', 'R', '7', '8', '9'};

const char *statistics_labels[] = {"MISSED ", "GO     ", "?      "};
