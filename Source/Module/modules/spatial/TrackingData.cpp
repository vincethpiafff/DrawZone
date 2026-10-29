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

void TrackingData::countMessage(const void* source)
{
	const ScopedLock sl(lock);
	uint32 now = Time::getMillisecondCounter();
	moduleRate.add(now);
	sourceRates[source].add(now);
}

TrackingData::SourceStats TrackingData::getSourceStats(const void* source)
{
	const ScopedLock sl(lock);
	SourceStats s;
	auto it = sourceRates.find(source);
	if (it == sourceRates.end()) return s;
	s.messagesPerSecond = it->second.getRate(Time::getMillisecondCounter());
	s.lastMessageMs = it->second.lastMessageMs;
	return s;
}

void TrackingData::RateCounter::add(uint32 now)
{
	lastMessageMs = now;
	count++;
	uint32 elapsed = now - windowStartMs;
	if (elapsed >= 1000)
	{
		messagesPerSecond = count * 1000.0f / elapsed;
		count = 0;
		windowStartMs = now;
	}
}

float TrackingData::RateCounter::getRate(uint32 now) const
{
	return lastMessageMs == 0 || now - lastMessageMs > 1000 ? 0 : messagesPerSecond;
}

void TrackingData::setSceneSize(Point<float> sceneSize)
{
	const ScopedLock sl(lock);
	data.sceneSize = sceneSize;
}

Point<float> TrackingData::getSceneSize()
{
	const ScopedLock sl(lock);
	return data.sceneSize;
}

bool TrackingData::isIdUsed(int id) const
{
	for (auto& p : data.persons) if (p.id == id) return true;
	return false;
}

void TrackingData::updatePerson(const TrackedPerson& person)
{
	const ScopedLock sl(lock);
	for (auto& p : data.persons)
	{
		if (p.source == person.source && p.sourceId == person.sourceId)
		{
			int id = p.id;
			p = person;
			p.id = id;
			return;
		}
	}

	TrackedPerson p = person;
	p.id = person.sourceId;
	if (p.id < 0 || isIdUsed(p.id))
	{
		//the source id is taken by a person of another source : a free id from 10000
		while (isIdUsed(nextRemappedId)) nextRemappedId++;
		p.id = nextRemappedId++;
	}
	data.persons.add(p);
}

void TrackingData::removePerson(const void* source, int sourceId)
{
	const ScopedLock sl(lock);
	data.persons.removeIf([source, sourceId](const TrackedPerson& p) { return p.source == source && p.sourceId == sourceId; });
}

void TrackingData::removeSource(const void* source)
{
	const ScopedLock sl(lock);
	data.persons.removeIf([source](const TrackedPerson& p) { return p.source == source; });
	sourceRates.erase(source);
}

void TrackingData::removeStalePersons(uint32 timeoutMs)
{
	const ScopedLock sl(lock);
	uint32 now = Time::getMillisecondCounter();
	data.persons.removeIf([now, timeoutMs](const TrackedPerson& p) { return now - p.lastUpdateMs > timeoutMs; });
}

int TrackingData::getNumPersons()
{
	const ScopedLock sl(lock);
	return data.persons.size();
}

int TrackingData::getNumPersons(const void* source)
{
	const ScopedLock sl(lock);
	int n = 0;
	for (auto& p : data.persons) if (p.source == source) n++;
	return n;
}

TrackingData::Snapshot TrackingData::getSnapshot()
{
	const ScopedLock sl(lock);
	Snapshot s = data;
	s.messagesPerSecond = moduleRate.getRate(Time::getMillisecondCounter());
	s.lastMessageMs = moduleRate.lastMessageMs;
	return s;
}
