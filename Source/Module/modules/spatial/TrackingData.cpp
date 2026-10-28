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
	data.lastMessageMs = now;
	lastSourceMessageMs[source] = now;
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
	lastSourceMessageMs.erase(source);
}

void TrackingData::removeStalePersons(uint32 timeoutMs)
{
	const ScopedLock sl(lock);
	uint32 now = Time::getMillisecondCounter();
	data.persons.removeIf([now, timeoutMs](const TrackedPerson& p) { return now - p.lastUpdateMs > timeoutMs; });
}

//a source that stopped sending loses its people
bool TrackingData::removeSilentSources(uint32 timeoutMs)
{
	const ScopedLock sl(lock);
	uint32 now = Time::getMillisecondCounter();
	int before = data.persons.size();
	data.persons.removeIf([this, now, timeoutMs](const TrackedPerson& p)
		{
			auto it = lastSourceMessageMs.find(p.source);
			return it == lastSourceMessageMs.end() || now - it->second > timeoutMs;
		});
	return data.persons.size() != before;
}

void TrackingData::clear()
{
	const ScopedLock sl(lock);
	Point<float> sceneSize = data.sceneSize;
	data = Snapshot();
	data.sceneSize = sceneSize;
	lastSourceMessageMs.clear();
	messageCount = 0;
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
	//no message for more than a second : the source is silent, don't show an old rate
	if (data.lastMessageMs == 0 || Time::getMillisecondCounter() - data.lastMessageMs > 1000) s.messagesPerSecond = 0;
	return s;
}
