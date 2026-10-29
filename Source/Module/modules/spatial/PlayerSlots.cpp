/*
  ==============================================================================

	PlayerSlots.cpp
	Created: 27 Oct 2026
	Author:  DrawZone

  ==============================================================================
*/

#include "PlayerSlots.h"

PlayerSlot::PlayerSlot(int index) :
	ControllableContainer("Slot " + String(index + 1))
{
	nameCanBeChangedByUser = true;

	color = addColorParameter("Color", "Color of this slot in the Tracking Monitor", getDefaultColor(index));
	occupied = addBoolParameter("Occupied", "Is this slot taken ? Stays true while the slot is held for a lost person", false);
	occupied->setControllableFeedbackOnly(true);
	personId = addIntParameter("Person Id", "Id of the person in this slot (-1 = free)", -1, -1);
	personId->setControllableFeedbackOnly(true);
	position = addPoint2DParameter("Position", "Position of the person in this slot, in meters from the top left corner of the scene. Frozen while the slot is held");
	position->setControllableFeedbackOnly(true);
	held = addBoolParameter("Held", "Hold Spot To Reclaim mode : the person was lost, the slot waits at its last position for anyone without a slot to walk in", false);
	held->setControllableFeedbackOnly(true);
	holdRemaining = addFloatParameter("Hold Remaining", "Seconds before a held slot is released (-1 = held until reclaimed, 0 = not held)", 0, -1);
	holdRemaining->setControllableFeedbackOnly(true);
	joined = addTrigger("Joined", "Triggered when a person takes this slot (not when a held slot is reclaimed)");
	joined->setControllableFeedbackOnly(true);
	left = addTrigger("Left", "Triggered when this slot is released");
	left->setControllableFeedbackOnly(true);
}

Colour PlayerSlot::getDefaultColor(int index)
{
	//distinct hues, spread by the golden ratio after the first 6
	const float hues[] = { 0, .6f, .33f, .15f, .8f, .08f };
	float hue = index < 6 ? hues[index] : std::fmod(index * .618034f, 1.0f);
	return Colour::fromHSV(hue, .7f, 1, 1);
}

PlayerSlots::PlayerSlots() :
	EnablingControllableContainer("Player Slots"),
	slotsValues("Slots")
{
	slotCount = addIntParameter("Slot Count", "Number of players. Each tracked person gets the lowest free slot and keeps it everywhere in the scene. When all slots are taken, newcomers wait", 4, 1, 64);

	releaseMode = addEnumParameter("Slot Release", "When a person is lost by the tracking : Next Person Takes Slot frees the slot at once for the last person who arrived ; Hold Spot To Reclaim keeps the slot at the last position, the first person without a slot who walks in takes it");
	releaseMode->addOption("Next Person Takes Slot", NEXT_PERSON)->addOption("Hold Spot To Reclaim", HOLD_SPOT);

	holdTime = addFloatParameter("Hold Time", "Hold Spot To Reclaim : time a held slot waits, in seconds, then it goes to the next person. 0 = waits until reclaimed", 5, 0);
	reclaimRadius = addFloatParameter("Reclaim Radius", "Hold Spot To Reclaim : a person without a slot closer than this to the held spot takes the slot, in meters", .5f, 0);

	slotsValues.saveAndLoadRecursiveData = false; //live data, names and colors are saved by getJSONData
	freeSlots = slotsValues.addIntParameter("Free Slots", "Number of free slots (a held slot is not free)", 0, 0);
	freeSlots->setControllableFeedbackOnly(true);

	onContainerParameterChanged(releaseMode);
	rebuildSlots();
}

PlayerSlots::~PlayerSlots()
{
}

void PlayerSlots::onContainerParameterChanged(Parameter* p)
{
	if (p == slotCount) rebuildSlots();
	else if (p == releaseMode)
	{
		bool hold = releaseMode->getValueDataAsEnum<ReleaseMode>() == HOLD_SPOT;
		holdTime->setEnabled(hold);
		reclaimRadius->setEnabled(hold);
	}
	else if (p == enabled && !enabled->boolValue())
	{
		releaseAll();
		waitingIds.clear();
		releasedIds.clear();
	}
}

void PlayerSlots::rebuildSlots()
{
	int count = slotCount->intValue();

	while (slots.size() > count)
	{
		PlayerSlot* s = slots.removeAndReturn(slots.size() - 1);
		slotsValues.removeChildControllableContainer(s); //owned : deleted here
	}

	while (slots.size() < count)
	{
		PlayerSlot* s = new PlayerSlot(slots.size());
		slotsValues.addChildControllableContainer(s, true);
		slots.add(s);
	}

	updateFreeSlots();
}

var PlayerSlots::getJSONData(bool includeNonOverriden)
{
	var data = EnablingControllableContainer::getJSONData(includeNonOverriden);

	var slotsData;
	for (auto& s : slots)
	{
		var sData;
		sData.append(s->niceName);
		sData.append(s->color->getColor().toString());
		slotsData.append(sData);
	}
	data.getDynamicObject()->setProperty("slots", slotsData);

	return data;
}

void PlayerSlots::loadJSONDataInternal(var data)
{
	//Slot Count is already loaded : one entry per slot
	var slotsData = data.getProperty("slots", var());
	if (!slotsData.isArray()) return;

	for (int i = 0; i < slots.size() && i < slotsData.size(); i++)
	{
		var sData = slotsData[i];
		if (!sData.isArray() || sData.size() < 2) continue;
		slots[i]->setNiceName(sData[0].toString());
		slots[i]->color->setColor(Colour::fromString(sData[1].toString()));
	}
}

void PlayerSlots::assign(PlayerSlot* slot, int id, Point<float> positionMeters)
{
	slot->trackedId = id;
	slot->occupied->setValue(true);
	slot->personId->setValue(id);
	slot->position->setPoint(positionMeters);
	slot->held->setValue(false);
	slot->holdRemaining->setValue(0);
}

void PlayerSlots::release(PlayerSlot* slot)
{
	if (slot->isFree()) return;
	slot->trackedId = -1;
	slot->occupied->setValue(false);
	slot->personId->setValue(-1);
	slot->held->setValue(false);
	slot->holdRemaining->setValue(0);
	slot->left->trigger();
}

void PlayerSlots::releaseAll()
{
	for (auto& s : slots) release(s);
	updateFreeSlots();
}

void PlayerSlots::updateFreeSlots()
{
	int n = 0;
	for (auto& s : slots) if (s->isFree()) n++;
	freeSlots->setValue(n);
}

int PlayerSlots::getSlotIndexForPerson(int id) const
{
	for (int i = 0; i < slots.size(); i++) if (!slots[i]->isFree() && !slots[i]->isHeld() && slots[i]->trackedId == id) return i;
	return -1;
}

void PlayerSlots::assignByHand(int personId, Point<float> positionMeters, int slotIndex)
{
	if (!enabled->boolValue() || slotIndex < 0 || slotIndex >= slots.size()) return;

	PlayerSlot* target = slots[slotIndex];
	int currentIndex = getSlotIndexForPerson(personId);
	if (currentIndex == slotIndex) return;

	//occupant of the target (a held slot has nobody to move)
	int occupantId = !target->isFree() && !target->isHeld() ? target->trackedId : -1;
	Point<float> occupantPosition = target->position->getPoint();

	if (!target->isFree()) release(target);
	if (currentIndex >= 0)
	{
		PlayerSlot* current = slots[currentIndex];
		release(current);
		if (occupantId >= 0) //swap
		{
			assign(current, occupantId, occupantPosition);
			current->joined->trigger();
		}
	}

	assign(target, personId, positionMeters);
	target->joined->trigger();

	waitingIds.removeFirstMatchingValue(personId);
	releasedIds.removeFirstMatchingValue(personId);
	updateFreeSlots();
}

void PlayerSlots::releaseByHand(int personId)
{
	int index = getSlotIndexForPerson(personId);
	if (index < 0) return;

	release(slots[index]);
	releasedIds.addIfNotAlreadyThere(personId);
	updateFreeSlots();
}

void PlayerSlots::update(const Array<TrackedPerson>& persons)
{
	if (!enabled->boolValue()) return;

	uint32 now = Time::getMillisecondCounter();
	bool hold = releaseMode->getValueDataAsEnum<ReleaseMode>() == HOLD_SPOT;

	auto findPerson = [&persons](int id) -> const TrackedPerson* { for (auto& p : persons) if (p.id == id) return &p; return nullptr; };

	//people in a slot : follow them ; lost : release the slot, or hold it at the last position
	for (auto& s : slots)
	{
		if (s->isFree()) continue;

		if (const TrackedPerson* p = findPerson(s->trackedId)) //also the same id coming back while held
		{
			if (s->isHeld()) assign(s, p->id, p->position);
			else s->position->setPoint(p->position);
			continue;
		}

		if (!hold)
		{
			release(s);
			continue;
		}

		if (!s->isHeld())
		{
			s->held->setValue(true);
			s->heldSinceMs = now;
		}

		if (holdTime->floatValue() <= 0) s->holdRemaining->setValue(-1);
		else
		{
			float remaining = holdTime->floatValue() - (now - s->heldSinceMs) / 1000.0f;
			if (remaining <= 0) release(s);
			else s->holdRemaining->setValue(remaining);
		}
	}

	//people without a slot, in order of arrival
	for (int i = waitingIds.size() - 1; i >= 0; i--)
	{
		int id = waitingIds[i];
		if (findPerson(id) == nullptr || getSlotIndexForPerson(id) >= 0) waitingIds.remove(i);
	}
	for (int i = releasedIds.size() - 1; i >= 0; i--) if (findPerson(releasedIds[i]) == nullptr) releasedIds.remove(i);
	for (auto& p : persons) if (getSlotIndexForPerson(p.id) < 0 && !waitingIds.contains(p.id) && !releasedIds.contains(p.id)) waitingIds.add(p.id);

	//held spots : the nearest waiting person inside the radius reclaims the slot (same player : neither Left nor Joined)
	for (auto& s : slots)
	{
		if (!s->isHeld()) continue;

		int bestId = -1;
		float bestDistance = reclaimRadius->floatValue();
		for (auto& id : waitingIds)
		{
			float d = s->position->getPoint().getDistanceFrom(findPerson(id)->position);
			if (d <= bestDistance) { bestDistance = d; bestId = id; }
		}

		if (bestId < 0) continue;
		assign(s, bestId, findPerson(bestId)->position);
		waitingIds.removeFirstMatchingValue(bestId);
	}

	//free slots, lowest first : the last people who arrived get them (in their order of arrival)
	Array<PlayerSlot*> freeList;
	for (auto& s : slots) if (s->isFree()) freeList.add(s);

	int numAssigned = jmin(freeList.size(), waitingIds.size());
	int firstWaiting = waitingIds.size() - numAssigned;
	for (int i = 0; i < numAssigned; i++)
	{
		int id = waitingIds[firstWaiting + i];
		assign(freeList[i], id, findPerson(id)->position);
		freeList[i]->joined->trigger();
	}
	waitingIds.removeRange(firstWaiting, numAssigned);

	updateFreeSlots();
}
