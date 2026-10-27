/*
  ==============================================================================

	TrackingData.cpp
	Created: 26 Oct 2026
	Author:  DrawZone

  ==============================================================================
*/

TrackingData::TrackingData()
{
}

TrackingData::~TrackingData()
{
}

void TrackingData::countMessage()
{
	const ScopedLock sl(lock);
	uint32 now = Time::getMillisecondCounter();
	data.lastMessageMs = now;
	messageCount++;

	uint32 elapsed = now - rateWindowStartMs;
	if (elapsed >= 1000)
	{
		data.messagesPerSecond = messageCount * 1000.0f / elapsed;
		messageCount = 0;
		rateWindowStartMs = now;
	}
}

void TrackingData::setScene(int frame, Point<float> sceneSize)
{
	const ScopedLock sl(lock);
	data.frame = frame;
	data.sceneSize = sceneSize;
}

void TrackingData::updatePerson(const TrackedPerson& person)
{
	const ScopedLock sl(lock);
	for (auto& p : data.persons)
	{
		if (p.id == person.id)
		{
			p = person;
			return;
		}
	}
	data.persons.add(person);
}

void TrackingData::removePerson(int id)
{
	const ScopedLock sl(lock);
	data.persons.removeIf([id](const TrackedPerson& p) { return p.id == id; });
}

void TrackingData::removeStalePersons(uint32 timeoutMs)
{
	const ScopedLock sl(lock);
	uint32 now = Time::getMillisecondCounter();
	data.persons.removeIf([now, timeoutMs](const TrackedPerson& p) { return now - p.lastUpdateMs > timeoutMs; });
}

//returns true if people were removed because the source stopped sending
bool TrackingData::removeAllIfSilent(uint32 timeoutMs)
{
	const ScopedLock sl(lock);
	if (data.persons.isEmpty()) return false;
	if (Time::getMillisecondCounter() - data.lastMessageMs <= timeoutMs) return false;
	data.persons.clear();
	return true;
}

void TrackingData::clear()
{
	const ScopedLock sl(lock);
	data = Snapshot();
	messageCount = 0;
}

int TrackingData::getNumPersons()
{
	const ScopedLock sl(lock);
	return data.persons.size();
}

TrackingData::Snapshot TrackingData::getSnapshot()
{
	const ScopedLock sl(lock);
	Snapshot s = data;
	//no message for more than a second : the source is silent, don't show an old rate
	if (data.lastMessageMs == 0 || Time::getMillisecondCounter() - data.lastMessageMs > 1000) s.messagesPerSecond = 0;
	return s;
}
