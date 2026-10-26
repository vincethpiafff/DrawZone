/*
  ==============================================================================

	SpatialModule.cpp
	Created: 24 Oct 2026
	Author:  DrawZone

  ==============================================================================
*/

#include "TrackingData.cpp"
#include "ui/TrackingMonitorPanel.cpp"

SpatialModule::SpatialModule() :
	OSCModule(getDefaultTypeString(), 12000, 9000)
{
	personCount = valuesCC.addIntParameter("Person Count", "Number of tracked people currently in the scene", 0, 0);
	personCount->setControllableFeedbackOnly(true);

	startTimer(200);
}

SpatialModule::~SpatialModule()
{
	stopTimer();
}

void SpatialModule::processMessageInternal(const OSCMessage& msg)
{
	trackingData.countMessage();

	//Augmenta OSC protocol v2, see docs/drawzone/notes/augmenta-protocol.md
	String address = msg.getAddressPattern().toString();

	if (address == "/scene")
	{
		if (msg.size() < 4) return;
		trackingData.setScene(OSCHelpers::getIntArg(msg[0]), Point<float>(OSCHelpers::getFloatArg(msg[2]), OSCHelpers::getFloatArg(msg[3])));
		trackingData.removeStalePersons(personTimeoutMs);
	}
	else if (address == "/object/enter" || address == "/object/update")
	{
		processAugmentaPerson(msg);
	}
	else if (address == "/object/leave")
	{
		if (msg.size() < 2) return;
		trackingData.removePerson(OSCHelpers::getIntArg(msg[1]));
	}
	else
	{
		return;
	}

	personCount->setValue(trackingData.getNumPersons());
}

void SpatialModule::processAugmentaPerson(const OSCMessage& msg)
{
	if (msg.size() < 15) return;

	TrackedPerson p;
	p.id = OSCHelpers::getIntArg(msg[1]);
	p.age = OSCHelpers::getFloatArg(msg[3]);
	p.position = Point<float>(OSCHelpers::getFloatArg(msg[4]), OSCHelpers::getFloatArg(msg[5]));
	p.velocity = Point<float>(OSCHelpers::getFloatArg(msg[6]), OSCHelpers::getFloatArg(msg[7]));
	p.orientation = OSCHelpers::getFloatArg(msg[8]);
	p.height = OSCHelpers::getFloatArg(msg[14]);
	p.lastUpdateMs = Time::getMillisecondCounter();

	trackingData.updatePerson(p);
}

void SpatialModule::timerCallback()
{
	if (trackingData.removeAllIfSilent(sourceTimeoutMs)) personCount->setValue(0);
}
