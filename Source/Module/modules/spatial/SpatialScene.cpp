/*
  ==============================================================================

	SpatialScene.cpp
	Created: 5 Nov 2026
	Author:  DrawZone

  ==============================================================================
*/

SpatialScene::SpatialScene(var params) :
	BaseItem("Scene")
{
	setHasCustomColor(true);	// itemColor, used for the scene in the Monitor and the 3D view
	sizeMode = addEnumParameter("Size Mode", "Auto : the size comes from a source of this scene (Augmenta /scene message, Area Size of a PSN source) : the one marked Use As Scene, or the only one knowing its scene. Manual : set below");
	sizeMode->addOption("Auto (From Sources)", SIZE_AUTO)->addOption("Manual", SIZE_MANUAL);
	manualSize = addPoint2DParameter("Size", "Manual : width and height of the scene, in meters");
	manualSize->setBounds(.1f, .1f, 1000, 1000);
	manualSize->setDefaultPoint(10, 6);
	manualSize->setEnabled(false);

	position = addPoint3DParameter("Position", "Where the top left corner of the scene stands in the 3D view, in meters (z up)");
	rotation = addPoint3DParameter("Rotation", "Rotation of the scene in the 3D view, in degrees, around X then Y then Z. A wall at the back of a floor : X -90 and Position Z = the height of the wall");
	rotation->setBounds(-360, -360, -360, 360, 360, 360);

	status = addStringParameter("Status", "Size of the scene and where it comes from", "");
	status->setControllableFeedbackOnly(true);
	status->isSavable = false;
}

SpatialScene::~SpatialScene()
{
}

void SpatialScene::onContainerParameterChangedInternal(Parameter* p)
{
	BaseItem::onContainerParameterChangedInternal(p);
	if (p == sizeMode) manualSize->setEnabled(sizeMode->getValueDataAsEnum<SizeMode>() == SIZE_MANUAL);
}

// SpatialSceneManager

SpatialSceneManager::SpatialSceneManager() :
	SpatialSectionManager("Scenes", SpatialSection::SCENE)
{
	itemDataType = "SpatialScene";
	selectItemWhenCreated = false;
}

SpatialSceneManager::~SpatialSceneManager()
{
}

void SpatialSceneManager::addItemInternal(SpatialScene* item, var)
{
	//distinct hues, after those of the sources ; a color chosen by the user is kept
	const float hues[] = { .6f, .12f, .45f, .95f };
	int index = items.indexOf(item);
	float hue = index < 4 ? hues[index] : std::fmod(.3f + index * .618034f, 1.0f);
	if (!item->itemColor->isOverriden) item->itemColor->setDefaultValue(Colour::fromHSV(hue, .45f, .9f, 1));
}

SpatialScene* SpatialSceneManager::getScene(TargetParameter* target)
{
	if (target != nullptr)
		if (SpatialScene* s = dynamic_cast<SpatialScene*>(target->targetContainer.get())) return s;
	return items.isEmpty() ? nullptr : items[0];
}

void SpatialSceneManager::setupTarget(TargetParameter* target)
{
	target->targetType = TargetParameter::CONTAINER;
	target->maxDefaultSearchLevel = 0;
	target->defaultContainerTypeCheckFunc = [](ControllableContainer* cc) { return dynamic_cast<SpatialScene*>(cc) != nullptr; };
	target->setRootContainer(this, false, true);
}
