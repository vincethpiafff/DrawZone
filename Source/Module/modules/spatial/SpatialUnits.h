/*
  ==============================================================================

	SpatialUnits.h
	Created: 6 Nov 2026
	Author:  DrawZone

	Times and percentages of the Cockpit. The value stays a plain float (seconds,
	0-1), so States, conditions, sequences and OSC of Chataigne still get a number.
	Only what is shown and typed changes :
	- time : HH:MM:SS:FF, FF counted at the Frame Rate of the Cockpit
	- percentage : 0-1 shown as 0-100 %

  ==============================================================================
*/

#pragma once

class TimecodeLabel;

class TimecodeParameter :
	public FloatParameter
{
public:
	TimecodeParameter(const String& niceName, const String& description, double initialValue, double minValue = 0, double maxValue = (double)INT32_MAX);

	//frames per second of FF, the Frame Rate of the Cockpit (one for the whole session)
	static int frameRate;
	static void setFrameRate(int fps);

	//seconds -> "HH:MM:SS:FF" ; negative -> "--:--:--:--" (no time)
	static String toTimecode(double seconds);
	//"HH:MM:SS:FF", "MM:SS:FF", "SS:FF" (':' or '/') or plain seconds -> seconds
	static double fromTimecode(const String& text);

	ControllableUI* createDefaultUI(Array<Controllable*> controllables = {}) override;
};

class TimecodeLabel :
	public TimeLabel
{
public:
	TimecodeLabel(Array<Parameter*> parameters);
	~TimecodeLabel();

	static void refreshAll();	// the frame rate changed

protected:
	String getValueString(const var& val) const override;
	void labelTextChanged(Label* l) override;
	void updateValueFromLabel() override;

private:
	static Array<TimecodeLabel*> instances;	// message thread

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimecodeLabel)
};

class PercentParameter :
	public FloatParameter
{
public:
	PercentParameter(const String& niceName, const String& description, double initialValue = 0);

	static String toPercent(double value) { return String(roundToInt(value * 100)) + " %"; }

	ControllableUI* createDefaultUI(Array<Controllable*> controllables = {}) override;
};

class PercentSliderUI :
	public FloatSliderUI
{
public:
	PercentSliderUI(Array<Parameter*> parameters) : FloatSliderUI(parameters) {}
	String getValueText() const override { return prefix + PercentParameter::toPercent(parameter->floatValue()) + suffix; }
};
