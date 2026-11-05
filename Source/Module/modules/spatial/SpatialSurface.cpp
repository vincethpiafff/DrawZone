/*
  ==============================================================================

	SpatialSurface.cpp
	Created: 5 Nov 2026
	Author:  DrawZone

  ==============================================================================
*/

SpatialSurface::SpatialSurface(SpatialModule* module) :
	BaseItem("Surface"),
	videoOutput(module, this)
{
	setHasCustomColor(true);	// itemColor, used for the surface in the Monitor and the 3D view
	sizeMode = addEnumParameter("Size Mode", "Auto : the size comes from a source of this surface (Augmenta /scene message, Area Size of a PSN source) : the one marked Use As Surface, or the only one knowing its surface. Manual : set below");
	sizeMode->addOption("Auto (From Sources)", SIZE_AUTO)->addOption("Manual", SIZE_MANUAL);
	manualSize = addPoint2DParameter("Size", "Manual : width and height of the surface, in meters");
	manualSize->setBounds(.1f, .1f, 1000, 1000);
	manualSize->setDefaultPoint(10, 6);
	manualSize->setEnabled(false);

	position = addPoint3DParameter("Position", "Where the top left corner of the surface stands in the 3D view, in meters (z up)");
	rotation = addPoint3DParameter("Rotation", "Rotation of the surface in the 3D view, in degrees, around X then Y then Z. A wall at the back of a floor : X -90 and Position Z = the height of the wall");
	rotation->setBounds(-360, -360, -360, 360, 360, 360);

	status = addStringParameter("Status", "Size of the surface and where it comes from", "");
	status->setControllableFeedbackOnly(true);
	status->isSavable = false;

	//saved with the surface ; the Display options of the video output too
	saveAndLoadRecursiveData = true;
	videoOutput.saveAndLoadRecursiveData = true;
	addChildControllableContainer(&playerSlots);
	addChildControllableContainer(&videoOutput);
	playerSlots.editorIsCollapsed = true;
	videoOutput.editorIsCollapsed = true;
	onContainerNiceNameChanged();

	using namespace SpatialSection;
	auto chip = [](const String& text, bool on) { Chip c; c.text = text; c.on = on; return c; };

	playerSlots.customGetEditorFunc = [this, chip](bool isRoot, Array<ControllableContainer*> cc) -> InspectableEditor*
		{
			return new SpatialSectionEditor<EnablingControllableContainerEditor>(SLOTS, [this, chip]()
				{
					if (!playerSlots.enabled->boolValue()) return Array<Chip>({ chip("Disabled", false) });
					int n = playerSlots.slotCount->intValue();
					int taken = n - playerSlots.freeSlots->intValue();
					return Array<Chip>({ chip(String(n) + (n == 1 ? " slot" : " slots"), false), chip(String(taken) + " taken", taken > 0) });
				}, Inspectable::getArrayAs<ControllableContainer, EnablingControllableContainer>(cc), isRoot);
		};

	videoOutput.customGetEditorFunc = [this, chip](bool isRoot, Array<ControllableContainer*> cc) -> InspectableEditor*
		{
			return new SpatialSectionEditor<GenericControllableContainerEditor>(OUTPUT, [this, chip]()
				{
					bool s = videoOutput.spout->boolValue(), n = videoOutput.ndi->boolValue();
					return Array<Chip>({ chip(s ? "Spout on" : "Spout off", s), chip(n ? "NDI on" : "NDI off", n) });
				}, cc, isRoot);
		};
}

SpatialSurface::~SpatialSurface()
{
}

void SpatialSurface::onContainerParameterChangedInternal(Parameter* p)
{
	BaseItem::onContainerParameterChangedInternal(p);
	if (p == sizeMode) manualSize->setEnabled(sizeMode->getValueDataAsEnum<SizeMode>() == SIZE_MANUAL);
}

void SpatialSurface::onContainerNiceNameChanged()
{
	BaseItem::onContainerNiceNameChanged();
	playerSlots.slotsValues.setNiceName(niceName);
}

// SpatialSurfaceManager

SpatialSurfaceManager::SpatialSurfaceManager() :
	SpatialSectionManager("Surfaces", SpatialSection::SURFACE)
{
	itemDataType = "SpatialSurface";
	selectItemWhenCreated = false;
}

SpatialSurfaceManager::~SpatialSurfaceManager()
{
}

void SpatialSurfaceManager::addItemInternal(SpatialSurface* item, var)
{
	//distinct hues, after those of the sources ; a color chosen by the user is kept
	const float hues[] = { .6f, .12f, .45f, .95f };
	int index = items.indexOf(item);
	float hue = index < 4 ? hues[index] : std::fmod(.3f + index * .618034f, 1.0f);
	if (!item->itemColor->isOverriden) item->itemColor->setDefaultValue(Colour::fromHSV(hue, .45f, .9f, 1));
}

SpatialSurface* SpatialSurfaceManager::getSurface(TargetParameter* target)
{
	if (target != nullptr)
		if (SpatialSurface* s = dynamic_cast<SpatialSurface*>(target->targetContainer.get())) return s;
	return items.isEmpty() ? nullptr : items[0];
}

void SpatialSurfaceManager::setupTarget(TargetParameter* target)
{
	target->targetType = TargetParameter::CONTAINER;
	target->maxDefaultSearchLevel = 0;
	target->defaultContainerTypeCheckFunc = [](ControllableContainer* cc) { return dynamic_cast<SpatialSurface*>(cc) != nullptr; };
	target->setRootContainer(this, false, true);
}
