/*
  ==============================================================================

	LogicVariables.cpp
	Created: 6 Nov 2026
	Author:  DrawZone

  ==============================================================================
*/

// SpatialVariable

SpatialVariable::SpatialVariable(var params) :
	BaseItem(params.getProperty("varType", "Number").toString(), false),
	varType(getTypeFromName(params.getProperty("varType", "Number").toString()))
{
	nameCanBeChangedByUser = true;
	editorIsCollapsed = true;	// one compact row ; unfolded : its Default Value
	valuesCC.setNiceName(niceName);

	defaultValue = addValueParameter(this, varType, "Default Value", "Value the variable takes when the session loads and when it is reset");
	value = addValueParameter(&valuesCC, varType, "Value", "Current value, set by the Set nodes of the Logic Graph (or by hand)");
	valuesCC.resetTrigger = valuesCC.addTrigger("Reset", "Back to the Default Value");
	valuesCC.onReset = [this] { resetValue(); };
	valuesCC.saveAndLoadRecursiveData = false;	// the value is live : the Default Value is what is saved

	sendOSC = addBoolParameter("Send OSC", "Sends the value to the OSC outputs of the Cockpit each time it changes", false);
	oscAddress = addStringParameter("OSC Address", "Address of the messages. Empty : /drawzone/variables/<name of the variable>", "");
	valuesCC.onValueChanged = [this]
		{
			if (!sendOSC->boolValue()) return;
			if (SpatialVariableManager* m = dynamic_cast<SpatialVariableManager*>(parentContainer.get())) m->sendValue(this);
		};
}

SpatialVariable::~SpatialVariable()
{
}

Parameter* SpatialVariable::addValueParameter(ControllableContainer* cc, Type t, const String& name, const String& help)
{
	switch (t)
	{
	case ONOFF: return cc->addBoolParameter(name, help, false);
	case PERCENT:
	{
		//-1 too : "outside" (Zone X of a person out of any zone, a held slot) goes through as it is, shown --
		Parameter* p = cc->addParameter(new PercentParameter(name, help + " (%, -1 = outside)", 0));
		p->setRange(-1, 1);
		return p;
	}
	case TIME: return cc->addParameter(new TimecodeParameter(name, help + " (HH:MM:SS:FF)", 0));
	case METERS: return cc->addFloatParameter(name, help + " (meters)", 0);
	default: return cc->addFloatParameter(name, help, 0);
	}
}

LogicPin::Kind SpatialVariable::getPinKind(Type t)
{
	switch (t)
	{
	case ONOFF: return LogicPin::BOOL;
	case PERCENT: return LogicPin::PERCENT;
	case TIME: return LogicPin::TIME;
	case METERS: return LogicPin::METERS;
	default: return LogicPin::NUMBER;
	}
}

void SpatialVariable::resetValue()
{
	value->setValue(defaultValue->getValue());
}

SpatialVariable::Type SpatialVariable::getTypeFromName(const String& name)
{
	if (name == "On-Off") return ONOFF;
	if (name == "Percentage") return PERCENT;
	if (name == "Time") return TIME;
	if (name == "Meters") return METERS;
	return NUMBER;
}

String SpatialVariable::getTypeName(Type t)
{
	switch (t)
	{
	case ONOFF: return "On-Off";
	case PERCENT: return "Percentage";
	case TIME: return "Time";
	case METERS: return "Meters";
	default: return "Number";
	}
}

Colour SpatialVariable::getTypeColour(Type t)
{
	return LogicPin::getColour(getPinKind(t));
}

String SpatialVariable::formatValue() const
{
	return format(varType, value->getValue());
}

String SpatialVariable::format(Type t, const var& value)
{
	double v = (double)value;
	switch (t)
	{
	case ONOFF: return (bool)value ? "On" : "Off";
	case PERCENT: return v < 0 ? String("--") : PercentParameter::toPercent(v);	// -1 : outside
	case TIME: return TimecodeParameter::toTimecode(v);
	case METERS: return String(v, 2) + " m";
	default: return v == std::floor(v) ? String((int64)v) : String(v, 3);
	}
}

void SpatialVariable::onContainerNiceNameChanged()
{
	BaseItem::onContainerNiceNameChanged();
	valuesCC.setNiceName(niceName);
}

InspectableEditor* SpatialVariable::getEditorInternal(bool isRoot, Array<Inspectable*>)
{
	return new SpatialVariableEditor(this, isRoot);
}

// SpatialVariableManager

SpatialVariableManager::SpatialVariableManager() :
	SpatialSectionManager("Variables", SpatialSection::VARIABLES)
{
	itemDataType = "Variable";
	selectItemWhenCreated = false;
	managerFactory = &factory;
	//+ : the type of the new variable
	for (auto& t : StringArray { "On-Off", "Number", "Percentage", "Time", "Meters" })
	{
		var params(new DynamicObject());
		params.getDynamicObject()->setProperty("varType", t);
		factory.defs.add(Factory<SpatialVariable>::Definition::createDef<SpatialVariable>("", t, params));
	}
}

SpatialVariableManager::~SpatialVariableManager()
{
}

Array<SpatialVariable*> SpatialVariableManager::getVariables(SpatialVariable::Type t) const
{
	Array<SpatialVariable*> result;
	for (auto& v : items) if (v->varType == t) result.add(v);
	return result;
}

void SpatialVariableManager::resetAll()
{
	for (auto& v : items) v->resetValue();
}

void SpatialVariableManager::addItemInternal(SpatialVariable* v, var)
{
	if (valuesParent != nullptr) valuesParent->addChildControllableContainer(&v->valuesCC);
	v->resetValue();
}

void SpatialVariableManager::addItemsInternal(Array<SpatialVariable*> vs, var data)
{
	for (auto& v : vs) addItemInternal(v, data);
}

void SpatialVariableManager::removeItemInternal(SpatialVariable* v)
{
	if (valuesParent != nullptr) valuesParent->removeChildControllableContainer(&v->valuesCC);
}

void SpatialVariableManager::removeItemsInternal(Array<SpatialVariable*> vs)
{
	for (auto& v : vs) removeItemInternal(v);
}

// SpatialVariableEditor

SpatialVariableEditor::SpatialVariableEditor(SpatialVariable* v, bool isRoot) :
	BaseItemEditor(Array<BaseItem*>{ v }, isRoot),
	variable(v)
{
	valueUI.reset(v->value->createDefaultUI());
	addAndMakeVisible(valueUI.get());
	resetUI.reset(v->valuesCC.resetTrigger->createButtonUI());
	resetUI->customLabel = "Reset";
	addAndMakeVisible(resetUI.get());
}

SpatialVariableEditor::~SpatialVariableEditor()
{
}

void SpatialVariableEditor::paint(Graphics& g)
{
	BaseItemEditor::paint(g);
	if (dotRect.isEmpty()) return;
	g.setColour(SpatialVariable::getTypeColour(variable->varType));
	g.fillEllipse(dotRect.toFloat().withSizeKeepingCentre(10, 10));
}

void SpatialVariableEditor::resizedInternalHeaderItemInternal(Rectangle<int>& r)
{
	//right to left : Reset, value, type dot
	resetUI->setBounds(r.removeFromRight(50).reduced(2));
	r.removeFromRight(4);
	valueUI->setBounds(r.removeFromRight(110).reduced(0, 2));
	r.removeFromRight(4);
	dotRect = r.removeFromRight(14);
}

// --- nodes

static SpatialVariable::Type getNodeVarType(var params)
{
	return SpatialVariable::getTypeFromName(params.getProperty("varType", "Number").toString());
}

void LogicVariableTargets::setup(TargetParameter* t, SpatialModule* m, SpatialVariable::Type type, bool anyType)
{
	if (m == nullptr) return;
	t->targetType = TargetParameter::CONTAINER;
	t->maxDefaultSearchLevel = 0;
	t->defaultContainerTypeCheckFunc = [type, anyType](ControllableContainer* cc)
		{
			SpatialVariable* v = dynamic_cast<SpatialVariable*>(cc);
			return v != nullptr && (anyType || v->varType == type);
		};
	t->setRootContainer(&m->variableManager, false, true);
}

SpatialVariable* LogicVariableTargets::get(TargetParameter* t)
{
	return dynamic_cast<SpatialVariable*>(t->targetContainer.get());
}

String LogicVariableTargets::getName(TargetParameter* t, const String& none)
{
	SpatialVariable* v = get(t);
	return v != nullptr ? v->niceName : none;
}

// GetOnOffVariableNode

GetOnOffVariableNode::GetOnOffVariableNode(var params) :
	StateNode(getTypeStringStatic(), params)
{
	variableTarget = addTargetParameter("Variable", "On-Off variable read : On when it becomes On, Off when it becomes Off, State = its value");
}

void GetOnOffVariableNode::setup(SpatialModule* m)
{
	StateNode::setup(m);
	LogicVariableTargets::setup(variableTarget, m, SpatialVariable::ONOFF);
}

bool GetOnOffVariableNode::getState(bool& valid) const
{
	SpatialVariable* v = LogicVariableTargets::get(variableTarget);
	valid = v != nullptr;
	return valid && v->value->boolValue();
}

// GetVariableNode

GetVariableNode::GetVariableNode(var params) :
	ValueNode("Get " + SpatialVariable::getTypeName(getNodeVarType(params)), params),
	varType(getNodeVarType(params))
{
	variableTarget = addTargetParameter("Variable", SpatialVariable::getTypeName(varType) + " variable read : Changed fires each time its value changes");
	addValuePin("Value", SpatialVariable::getPinKind(varType));
}

void GetVariableNode::setup(SpatialModule* m)
{
	ValueNode::setup(m);
	LogicVariableTargets::setup(variableTarget, m, varType);
}

bool GetVariableNode::read(LogicGraph&, Array<var>& values)
{
	SpatialVariable* v = LogicVariableTargets::get(variableTarget);
	if (v == nullptr) return false;
	values.add(v->value->getValue());
	return true;
}

// SetVariableNode

SetVariableNode::SetVariableNode(var params) :
	LogicNode("Set " + SpatialVariable::getTypeName(getNodeVarType(params)), params),
	varType(getNodeVarType(params)),
	valueSetting(nullptr)
{
	variableTarget = addTargetParameter("Variable", SpatialVariable::getTypeName(varType) + " variable written when In is reached");
	operation = addEnumParameter("Operation", varType == SpatialVariable::ONOFF ? "On, Off, or the opposite of its value (Toggle), when the Value input is not linked" : "Set : takes the value (all the time when only Value is linked, without In) ; Add / Subtract : adds it to the variable or takes it away when In is reached (a score +1)");
	if (varType == SpatialVariable::ONOFF) operation->addOption("On", ON)->addOption("Off", OFF)->addOption("Toggle", TOGGLE);
	else
	{
		operation->addOption("Set", SET)->addOption("Add", ADD)->addOption("Subtract", SUBTRACT);
		valueSetting = SpatialVariable::addValueParameter(this, varType, "Value", "Used when the Value input is not linked");
	}
	addPin("In", LogicPin::EXEC, false);
	addPin("Value", SpatialVariable::getPinKind(varType), false);	// On-Off : when linked, the variable takes it (the Operation is for a node without it)
	addPin("Then", LogicPin::EXEC, true);
}

void SetVariableNode::setup(SpatialModule* m)
{
	LogicNode::setup(m);
	LogicVariableTargets::setup(variableTarget, m, varType);
}

void SetVariableNode::execute(LogicGraph& g, const String&)
{
	if (!enabled->boolValue()) return;
	if (SpatialVariable* v = LogicVariableTargets::get(variableTarget))
	{
		int op = (int)operation->getValueData();
		if (varType == SpatialVariable::ONOFF)
		{
			var in = g.getInput(this, "Value");
			v->value->setValue(!in.isVoid() ? (bool)in : op == TOGGLE ? !v->value->boolValue() : op == ON);
		}
		else
		{
			var in = g.getInput(this, "Value");
			double x = in.isVoid() ? (double)valueSetting->getValue() : (double)in;
			double now = (double)v->value->getValue();
			v->value->setValue(op == ADD ? now + x : op == SUBTRACT ? now - x : x);
		}
	}
	g.trigger(this, "Then");
}

String SetVariableNode::getSummary() const
{
	String name = LogicVariableTargets::getName(variableTarget, "(no variable)");
	int op = (int)operation->getValueData();
	if (varType == SpatialVariable::ONOFF) return name + " -> " + operation->getValueKey();
	if (op == SET) return name;
	return name + (op == ADD ? " +" : " -");
}

// ResetVariablesNode

ResetVariablesNode::ResetVariablesNode(var params) :
	LogicNode(getTypeStringStatic(), params)
{
	variableTarget = addTargetParameter("Variable", "Variable put back to its Default Value. Empty : all the variables (start of a game)");
	addPin("In", LogicPin::EXEC, false);
	addPin("Then", LogicPin::EXEC, true);
}

void ResetVariablesNode::setup(SpatialModule* m)
{
	LogicNode::setup(m);
	LogicVariableTargets::setup(variableTarget, m, SpatialVariable::NUMBER, true);
}

void ResetVariablesNode::execute(LogicGraph& g, const String&)
{
	if (!enabled->boolValue()) return;
	if (SpatialVariable* v = LogicVariableTargets::get(variableTarget)) v->resetValue();
	else if (variableTarget->stringValue().isEmpty() && module != nullptr) module->variableManager.resetAll();	// a variable removed since : nothing
	g.trigger(this, "Then");
}

//live mode of Set : no In link but a Value link -> the variable takes Value at each update (Add / Subtract would pile up : they need In)
void SetVariableNode::process(LogicGraph& g)
{
	if (!enabled->boolValue() || g.isLinked(this, "In") || !g.isLinked(this, "Value")) return;
	if (varType != SpatialVariable::ONOFF && (int)operation->getValueData() != SET) return;
	SpatialVariable* v = LogicVariableTargets::get(variableTarget);
	if (v == nullptr) return;
	var in = g.getInput(this, "Value");
	if (in.isVoid()) return;	// nothing to read (person not tracked...) : the variable keeps its value
	var now = varType == SpatialVariable::ONOFF ? var((bool)in) : var((double)in);
	if (v->value->getValue() != now) v->value->setValue(now);
}

// --- OSC (brick 6)

String SpatialVariable::getOSCAddress() const
{
	String a = oscAddress->stringValue().trim();
	if (a.isNotEmpty()) return a.startsWithChar('/') ? a : "/" + a;
	//from the name : lower case, spaces and other signs as _
	String slug;
	for (auto c : niceName.toLowerCase())
		slug += CharacterFunctions::isLetterOrDigit(c) ? String::charToString(c) : String("_");
	return "/drawzone/variables/" + slug;
}

void SpatialVariableManager::sendValue(SpatialVariable* v)
{
	if (oscModule == nullptr) return;
	try
	{
		OSCMessage m{ OSCAddressPattern(v->getOSCAddress()) };
		if (v->varType == SpatialVariable::ONOFF) m.addInt32(v->value->boolValue() ? 1 : 0);
		else m.addFloat32((float)v->value->floatValue());
		oscModule->sendOSC(m);
		v->lastSent = v->varType == SpatialVariable::ONOFF ? String(v->value->boolValue() ? 1 : 0) : String(v->value->floatValue(), 3);
		v->lastSentMs = Time::getMillisecondCounter();
	}
	catch (OSCFormatError&)
	{
		v->lastSent = "bad address";
	}
}

String SpatialVariableManager::getOutputsText() const
{
	if (oscModule == nullptr || oscModule->outputManager == nullptr) return "no OSC output";
	Array<OSCOutput*> on;
	for (auto& o : oscModule->outputManager->items) if (o->enabled->boolValue()) on.add(o);
	if (on.isEmpty()) return "no OSC output";
	if (on.size() > 1) return String(on.size()) + " outputs";
	return on[0]->remoteHost->stringValue() + ":" + String(on[0]->remotePort->intValue());
}

void SpatialVariable::onContainerParameterChangedInternal(Parameter* p)
{
	if ((p == sendOSC || p == oscAddress) && sendOSC->boolValue() && !isCurrentlyLoadingData)
		if (SpatialVariableManager* m = dynamic_cast<SpatialVariableManager*>(parentContainer.get())) m->sendValue(this);
}
