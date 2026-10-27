/*
  ==============================================================================

	PlayerSlots.h
	Created: 27 Oct 2026
	Author:  DrawZone

	Player slots : each tracked person gets a stable player slot (Slot 1..N)
	for the whole scene. When all slots are taken, newcomers wait ; a slot
	released goes to the last person who arrived. A lost person's slot can
	also stay held at its last position, for anyone without a slot to walk in.

  ==============================================================================
*/

#pragma once

//one slot, shown in Values > Slots : renamable, with its own color
class PlayerSlot :
	public ControllableContainer
{
public:
	PlayerSlot(int index);

	ColorParameter* color;
	BoolParameter* occupied;
	IntParameter* personId;			// -1 when free
	Point2DParameter* position;		// meters, origin top left of the scene ; frozen while held
	BoolParameter* held;
	FloatParameter* holdRemaining;	// seconds, -1 = held without time limit
	Trigger* joined;
	Trigger* left;

	int trackedId = -1;		// id of the person in this slot, -1 = free
	uint32 heldSinceMs = 0;

	bool isFree() const { return trackedId < 0; }
	bool isHeld() const { return held->boolValue(); }

	static Colour getDefaultColor(int index);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerSlot)
};

class PlayerSlots :
	public EnablingControllableContainer
{
public:
	PlayerSlots();
	~PlayerSlots();

	enum ReleaseMode { NEXT_PERSON, HOLD_SPOT };

	IntParameter* slotCount;
	EnumParameter* releaseMode;
	FloatParameter* holdTime;		// seconds, 0 = until reclaimed
	FloatParameter* reclaimRadius;	// meters

	ControllableContainer slotsValues;
	IntParameter* freeSlots;
	Array<PlayerSlot*> slots;	// owned by slotsValues

	Array<int> waitingIds;	// tracked people without a slot, in order of arrival
	Array<int> releasedIds;	// slot removed by hand : no slot again until they leave the scene or are assigned by hand

	void update(const Array<TrackedPerson>& persons, Point<float> sceneSize);
	int getSlotIndexForPerson(int id) const;	// 0-based, -1 = no slot

	//by hand, from the Tracking Monitor : a person already in a slot swaps with the occupant,
	//else the occupant loses the slot and waits like anyone without a slot
	void assignByHand(int personId, Point<float> positionMeters, int slotIndex);
	void releaseByHand(int personId);

	void onContainerParameterChanged(Parameter* p) override;

	//slot names and colors are saved here, the slots themselves are live values
	var getJSONData(bool includeNonOverriden = false) override;
	void loadJSONDataInternal(var data) override;

private:
	void rebuildSlots();
	void assign(PlayerSlot* slot, int id, Point<float> positionMeters);
	void release(PlayerSlot* slot);
	void releaseAll();
	void updateFreeSlots();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerSlots)
};
