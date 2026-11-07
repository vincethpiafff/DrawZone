/*
  ==============================================================================

	SpatialUnits.cpp
	Created: 6 Nov 2026
	Author:  DrawZone

  ==============================================================================
*/

// TimecodeParameter

int TimecodeParameter::frameRate = 25;

TimecodeParameter::TimecodeParameter(const String& niceName, const String& description, double initialValue, double minValue, double maxValue) :
	FloatParameter(niceName, description, initialValue, minValue, maxValue)
{
}

void TimecodeParameter::setFrameRate(int fps)
{
	fps = jlimit(1, 1000, fps);
	if (fps == frameRate) return;
	frameRate = fps;
	TimecodeLabel::refreshAll();
}

String TimecodeParameter::toTimecode(double seconds)
{
	if (seconds < 0) return "--:--:--:--";
	int64 frames = (int64)std::floor(seconds * frameRate + 1e-6);
	int ff = (int)(frames % frameRate);
	int64 totalSeconds = frames / frameRate;
	int ss = (int)(totalSeconds % 60);
	int mm = (int)((totalSeconds / 60) % 60);
	int64 hh = totalSeconds / 3600;
	auto two = [](int64 v) { return String(v).paddedLeft('0', 2); };
	return two(hh) + ":" + two(mm) + ":" + two(ss) + ":" + two(ff);
}

double TimecodeParameter::fromTimecode(const String& text)
{
	String t = text.trim();
	if (t.startsWith("-")) return -1;
	StringArray parts;
	parts.addTokens(t.replaceCharacter('/', ':'), ":", "");
	parts.trim();
	parts.removeEmptyStrings();
	if (parts.size() <= 1) return t.getDoubleValue();

	//from the right : FF, then SS, MM, HH
	double frames = parts[parts.size() - 1].getDoubleValue();
	double seconds = parts[parts.size() - 2].getDoubleValue();
	double minutes = parts.size() >= 3 ? parts[parts.size() - 3].getDoubleValue() : 0;
	double hours = parts.size() >= 4 ? parts[parts.size() - 4].getDoubleValue() : 0;
	return hours * 3600 + minutes * 60 + seconds + frames / frameRate;
}

ControllableUI* TimecodeParameter::createDefaultUI(Array<Controllable*> controllables)
{
	//a display chosen by hand (slider...) stays possible
	if (customUI != NONE) return FloatParameter::createDefaultUI(controllables);
	Array<Parameter*> parameters = Inspectable::getArrayAs<Controllable, Parameter>(controllables);
	if (parameters.size() == 0) parameters = { this };
	return new TimecodeLabel(parameters);
}

// TimecodeLabel

Array<TimecodeLabel*> TimecodeLabel::instances;

TimecodeLabel::TimecodeLabel(Array<Parameter*> parameters) :
	TimeLabel(parameters)
{
	instances.add(this);
	valueChanged(parameter->getValue());
	handlePaintTimer();
}

TimecodeLabel::~TimecodeLabel()
{
	instances.removeFirstMatchingValue(this);
}

void TimecodeLabel::refreshAll()
{
	for (auto& l : instances)
	{
		if (l->parameter.wasObjectDeleted()) continue;
		l->valueChanged(l->parameter->getValue());
		l->handlePaintTimer();
	}
}

String TimecodeLabel::getValueString(const var& val) const
{
	return TimecodeParameter::toTimecode((double)val);
}

void TimecodeLabel::labelTextChanged(Label*)
{
	String s = valueLabel.getText();
	s = s.substring(prefix.length(), s.length() - suffix.length());
	parameter->setUndoableValueForSelected(parameter->getValue(), cropUIValue(TimecodeParameter::fromTimecode(s)));
	shouldRepaint = true;
}

void TimecodeLabel::updateValueFromLabel()
{
	double v = TimecodeParameter::fromTimecode(valueLabel.getText());
	if (v == parameter->doubleValue()) valueLabel.setText(getValueString(v), dontSendNotification);
}

// PercentParameter

PercentParameter::PercentParameter(const String& niceName, const String& description, double initialValue) :
	FloatParameter(niceName, description, initialValue, 0, 1)
{
}

ControllableUI* PercentParameter::createDefaultUI(Array<Controllable*> controllables)
{
	if (customUI != NONE) return FloatParameter::createDefaultUI(controllables);
	Array<Parameter*> parameters = Inspectable::getArrayAs<Controllable, Parameter>(controllables);
	if (parameters.size() == 0) parameters = { this };
	return new PercentSliderUI(parameters);
}
