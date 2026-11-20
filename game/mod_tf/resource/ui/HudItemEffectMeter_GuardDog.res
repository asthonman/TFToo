"Resource/UI/HudItemEffectMeter_GuardDog.res"
{
	HudItemEffectMeter
	{
		"fieldName"		"HudItemEffectMeter"
		"visible"		"1"
		"enabled"		"1"
		"xpos"			"r194"
		"ypos"			"r160"
		"wide"			"100"
		"tall"			"50"
		"MeterFG"		"White"
		"MeterBG"		"Gray"
	}

	"ItemEffectMeterBG"
	{
		"ControlName"	"CTFImagePanel"
		"fieldName"		"ItemEffectMeterBG"
		"xpos"			"12"
		"ypos"			"0"
		"zpos"			"0"
		"wide"			"76"
		"tall"			"44"
		"visible"		"1"
		"enabled"		"1"
		"image"			"../hud/misc_ammo_area_blue"
		"scaleImage"	"1"
		"teambg_2"		"../hud/misc_ammo_area_red"
		"teambg_3"		"../hud/misc_ammo_area_blue"
	}

	"ItemEffectMeterLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"ItemEffectMeterLabel"
		"xpos"			"12"
		"ypos"			"27"
		"zpos"			"2"
		"wide"			"76"
		"tall"			"15"
		"visible"		"1"
		"enabled"		"1"
		"labelText"		"#TF_GuardDog_Sniff"
		"textAlignment"	"center"
		"font"			"TFFontSmall"
	}

	"ItemEffectMeter"
	{
		"ControlName"	"ContinuousProgressBar"
		"fieldName"		"ItemEffectMeter"
		"xpos"			"25"
		"ypos"			"23"
		"zpos"			"2"
		"wide"			"40"
		"tall"			"6"
		"visible"		"1"
		"enabled"		"1"
	}
}
