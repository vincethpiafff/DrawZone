/*
  ==============================================================================

	LogicVariables.h
	Created: 6 Nov 2026
	Author:  DrawZone

	Variables of the Cockpit (v0.7.2 brick 5) : values kept between two runs of
	the graphs (a score, "game started", a round time). Block Variables of the
	Cockpit, one item per variable, its type chosen with + (On-Off, Number,
	Percentage, Time, Meters). Read and written by the Get / Set / Reset
	Variables nodes of the Logic Graph.

	Each variable has its value in Values > Variables > <name> (Value + Reset),
	so States, conditions, mappings and OSC of Chataigne can use it too. The
	value is not saved : it starts at its Default Value when the session loads.
	Inspector (look A of 06/11/2026) : one compact row per variable, its type
	dot, its live value and a reset button in the header.

  ==============================================================================
*/

#pragma once

//Values > Variables > <name> : the value and its Reset, usable everywhere in Chataigne
class SpatialVariableValues :
	public ControllableContainer
{
public:
	SpatialVariableValues() : ControllableContainer("Variable") {}
	Trigger* resetTrigger = nullptr;
	std::function<void()> onReset;
	void onContainerTriggerTriggered(Trigger* t) override { if (t == resetTrigger && onReset) onReset(); }
};

class SpatialVariable :
	public BaseItem
{
public:
	enum Type { ONOFF, NUMBER, PERCENT, TIME, METERS };

	SpatialVariable(var params = var());
	~SpatialVariable();

	Type varType;
	Parameter* defaultValue;
	SpatialVariableValues valuesCC;	// Values > Variables > <name>
	Parameter* value;

	void resetValue();	// back to Default Value

	static Type getTypeFromName(const String& name);
	static String getTypeName(Type t);
	static Colour getTypeColour(Type t);	// the colour of the pins of this type
	static LogicPin::Kind getPinKind(Type t);
	//a parameter of this type in cc (On-Off : bool, Percentage : %, Time : HH:MM:SS:FF, others : float)
	static Parameter* addValueParameter(ControllableContainer* cc, Type t, const String& name, const String& help);
	String formatValue() const;			// as shown : On / Off, 12, 64 %, HH:MM:SS:FF, 2.35 m
	static String format(Type t, const var& v);

	void onContainerNiceNameChanged() override;	// its values follow its name

	InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables = Array<Inspectable*>()) override;

	String getTypeString() const override { return getTypeName(varType); }

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialVariable)
};

class SpatialVariableManager :
	public SpatialSectionManager<SpatialVariable>
{
public:
	SpatialVariableManager();
	~SpatialVariableManager();

	Factory<SpatialVariable> factory;
	ControllableContainer* valuesParent = nullptr;	// Values > Variables, set by the module before anything is added

	Array<SpatialVariable*> getVariables(SpatialVariable::Type t) const;
	void resetAll();

	void addItemInternal(SpatialVariable* v, var data) override;
	void addItemsInternal(Array<SpatialVariable*> vs, var data) override;
	void removeItemInternal(SpatialVariable* v) override;
	void removeItemsInternal(Array<SpatialVariable*> vs) override;
};

//compact row of a variable : type dot, value (editable), reset
class SpatialVariableEditor :
	public BaseItemEditor
{
public:
	SpatialVariableEditor(SpatialVariable* v, bool isRoot);
	~SpatialVariableEditor();

	SpatialVariable* variable;
	std::unique_ptr<ControllableUI> valueUI;
	std::unique_ptr<TriggerButtonUI> resetUI;

	void paint(Graphics& g) override;
	void resizedInternalHeaderItemInternal(Rectangle<int>& r) override;

private:
	Rectangle<int> dotRect;
};

// --- nodes of the variables (Logic Graph) : Get in Input Data > Variables, Set and Reset in Output Data > Set Variables

namespace LogicVariableTargets
{
	//Variable setting : the variables of one type (or all of them : anyType)
	void setup(TargetParameter* t, SpatialModule* m, SpatialVariable::Type type, bool anyType = false);
	SpatialVariable* get(TargetParameter* t);
	String getName(TargetParameter* t, const String& none);
}

//Get On-Off : On, Off (when it changes) and State, like Zone Presence
class GetOnOffVariableNode :
	public StateNode
{
public:
	GetOnOffVariableNode(var params = var());
	TargetParameter* variableTarget;

	Category getCategory() const override { return VARIABLE; }
	void setup(SpatialModule* m) override;
	bool getState(bool& valid) const override;
	String getSummary() const override { return LogicVariableTargets::getName(variableTarget, "(no variable)"); }

	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Get On-Off"; }
};

//Get Number / Percentage / Time / Meters : Changed + Value
class GetVariableNode :
	public ValueNode
{
public:
	GetVariableNode(var params = var());	// params.varType
	const SpatialVariable::Type varType;
	TargetParameter* variableTarget;

	Category getCategory() const override { return VARIABLE; }
	void setup(SpatialModule* m) override;
	bool read(LogicGraph& g, Array<var>& values) override;
	String getSummary() const override { return LogicVariableTargets::getName(variableTarget, "(no variable)"); }

	String getTypeString() const override { return "Get " + SpatialVariable::getTypeName(varType); }
};

//Set <type> : In -> the variable takes the Value input (or the Value setting when not linked) -> Then
//On-Off : Set To On / Off / Toggle ; numbers : Set, Add or Subtract
class SetVariableNode :
	public LogicNode
{
public:
	SetVariableNode(var params = var());	// params.varType
	const SpatialVariable::Type varType;
	TargetParameter* variableTarget;
	EnumParameter* operation;
	Parameter* valueSetting;	// numbers : used when the Value input is not linked

	enum Operation { SET, ADD, SUBTRACT, ON, OFF, TOGGLE };

	Category getCategory() const override { return VARIABLE; }
	void setup(SpatialModule* m) override;
	void execute(LogicGraph& g, const String& inPin) override;
	String getSummary() const override;
	void process(LogicGraph& g) override;	// In not linked, Value linked : the variable follows Value live (Set, On-Off)

	String getTypeString() const override { return "Set " + SpatialVariable::getTypeName(varType); }
};

//Reset Variables : In -> one variable, or all of them (Variable empty), back to their Default Value -> Then
class ResetVariablesNode :
	public LogicNode
{
public:
	ResetVariablesNode(var params = var());
	TargetParameter* variableTarget;

	Category getCategory() const override { return VARIABLE; }
	void setup(SpatialModule* m) override;
	void execute(LogicGraph& g, const String& inPin) override;
	String getSummary() const override { return LogicVariableTargets::getName(variableTarget, "All variables"); }

	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Reset Variables"; }
};
