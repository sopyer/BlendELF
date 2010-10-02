
void elfActorIpoCallback(elfFramePlayer* player)
{
	elfActor* actor;
	float frame;
	elfVec3f pos;
	elfVec3f rot;
	elfVec3f scale;
	elfVec4f qua;

	actor = (elfActor*)elfGetFramePlayerUserData(player);
	frame = elfGetFramePlayerFrame(player);

	if(actor->ipo->loc)
	{
		pos = elfGetIpoLoc(actor->ipo, frame);
		elfSetActorPosition(actor, pos.x, pos.y, pos.z);
	}
	if(actor->ipo->rot)
	{
		rot = elfGetIpoRot(actor->ipo, frame);
		elfSetActorRotation(actor, rot.x, rot.y, rot.z);
	}
	if(actor->ipo->scale)
	{
		scale = elfGetIpoScale(actor->ipo, frame);
		if(actor->objType == ELF_ENTITY) elfSetEntityScale((elfEntity*)actor, scale.x, scale.y, scale.z);
	}
	if(actor->ipo->qua)
	{
		qua = elfGetIpoQua(actor->ipo, frame);
		elfSetActorOrientation(actor, qua.x, qua.y, qua.z, qua.w);
	}
}

void elfInitActor(elfActor* actor, unsigned char camera)
{
	if(!camera) actor->transform = gfxCreateObjectTransform();
	else actor->transform = gfxCreateCameraTransform();

	actor->joints = elfCreateList();
	actor->sources = elfCreateList();
	actor->properties = elfCreateList();

	elfIncRef((elfObject*)actor->joints);
	elfIncRef((elfObject*)actor->sources);
	elfIncRef((elfObject*)actor->properties);

	actor->ipo = elfCreateIpo();
	elfIncRef((elfObject*)actor->ipo);

	actor->ipoPlayer = elfCreateFramePlayer();
	elfIncRef((elfObject*)actor->ipoPlayer);
	elfSetFramePlayerUserData(actor->ipoPlayer, actor);
	elfSetFramePlayerCallback(actor->ipoPlayer, elfActorIpoCallback);

	actor->pbbLengths.x = actor->pbbLengths.y = actor->pbbLengths.z = 1.0;
	actor->shape = ELF_NONE;
	actor->mass = 0.0;
	actor->linDamp = 0.0;
	actor->angDamp = 0.0;
	actor->linSleep = 0.8;
	actor->angSleep = 1.0;
	actor->restitution = 0.0;
	actor->anisFric.x = actor->anisFric.y = actor->anisFric.z = 1.0;
	actor->linFactor.x = actor->linFactor.y = actor->linFactor.z = 1.0;
	actor->angFactor.x = actor->angFactor.y = actor->angFactor.z = 1.0;

	actor->moved = ELF_TRUE;
}

void elfUpdateActor(elfActor* actor)
{
	static float oposition[3];
	static float oorient[4];
	static float position[3];
	static float orient[4];
	static elfAudioSource* source;

	if(actor->object && !elfIsPhysicsObjectStatic(actor->object))
	{
		gfxGetTransformPosition(actor->transform, oposition);
		gfxGetTransformOrientation(actor->transform, oorient);

		elfGetPhysicsObjectPosition(actor->object, position);
		elfGetPhysicsObjectOrientation(actor->object, orient);

		gfxSetTransformPosition(actor->transform, position[0], position[1], position[2]);
		gfxSetTransformOrientation(actor->transform, orient[0], orient[1], orient[2], orient[3]);

		if(actor->dobject)
		{
			elfSetPhysicsObjectPosition(actor->dobject, position[0], position[1], position[2]);
			elfSetPhysicsObjectOrientation(actor->dobject, orient[0], orient[1], orient[2], orient[3]);
		}

		if(memcmp(&oposition, &position, sizeof(float)*3)) actor->moved = ELF_TRUE;
		if(memcmp(&oorient, &orient, sizeof(float)*4)) actor->moved = ELF_TRUE;
	}
	else
	{
		elfGetActorPosition_(actor, position);
	}

	if(actor->script && actor->scene->runScripts)
	{
		eng->actor = (elfObject*)actor;
		elfIncRef((elfObject*)actor);

		elfRunString("me = elf.GetActor()");
		elfRunScript(actor->script);
		elfRunString("me = nil");

		elfDecRef((elfObject*)actor);
		eng->actor = NULL;
	}

	if(actor->object) elfClearPhysicsObjectCollisions(actor->object);

	for(source = (elfAudioSource*)elfBeginList(actor->sources); source;
		source = (elfAudioSource*)elfGetListNext(actor->sources))
	{
		if(elfGetObjectRefCount((elfObject*)source) < 2 &&
			!elfIsSoundPlaying(source) &&
			!elfIsSoundPaused(source))
		{
			elfRemoveListObject(actor->sources, (elfObject*)source);
		}
		else
		{
			elfSetSoundPosition(source, position[0], position[1], position[2]);
		}
	}

	elfUpdateFramePlayer(actor->ipoPlayer);
}

void elfActorPreDraw(elfActor* actor)
{
}

void elfActorPostDraw(elfActor* actor)
{
	actor->moved = ELF_FALSE;
}

void elfCleanActor(elfActor* actor)
{
	elfJoint* joint;

	if(actor->name) elfDestroyString(actor->name);
	if(actor->filePath) elfDestroyString(actor->filePath);
	if(actor->transform) gfxDestroyTransform(actor->transform);
	if(actor->object)
	{
		elfSetPhysicsObjectActor(actor->object, NULL);
		elfSetPhysicsObjectWorld(actor->object, NULL);
		elfDecRef((elfObject*)actor->object);
	}
	if(actor->dobject)
	{
		elfSetPhysicsObjectActor(actor->dobject, NULL);
		elfSetPhysicsObjectWorld(actor->dobject, NULL);
		elfDecRef((elfObject*)actor->dobject);
	}
	if(actor->script) elfDecRef((elfObject*)actor->script);

	for(joint = (elfJoint*)elfBeginList(actor->joints); joint;
		joint = (elfJoint*)elfGetListNext(actor->joints))
	{
		if(elfGetJointActorA(joint) == actor)
			elfRemoveListObject(elfGetJointActorB(joint)->joints, (elfObject*)joint);
		else elfRemoveListObject(elfGetJointActorA(joint)->joints, (elfObject*)joint);
		elfClearJoint(joint);
	}

	elfDecRef((elfObject*)actor->joints);
	elfDecRef((elfObject*)actor->sources);
	elfDecRef((elfObject*)actor->properties);

	elfDecRef((elfObject*)actor->ipo);
	elfDecRef((elfObject*)actor->ipoPlayer);
}

ELF_API const char* ELF_APIENTRY elfGetActorName(elfActor* actor)
{
	return actor->name;
}

ELF_API const char* ELF_APIENTRY elfGetActorFilePath(elfActor* actor)
{
	return actor->filePath;
}

ELF_API elfScript* ELF_APIENTRY elfGetActorScript(elfActor* actor)
{
	return actor->script;
}

ELF_API void ELF_APIENTRY elfSetActorName(elfActor* actor, const char* name)
{
	if(actor->name) elfDestroyString(actor->name);
	actor->name = elfCreateString(name);
}

ELF_API void ELF_APIENTRY elfSetActorScript(elfActor* actor, elfScript* script)
{
	if(actor->script) elfDecRef((elfObject*)actor->script);
	actor->script = script;
	if(actor->script) elfIncRef((elfObject*)actor->script);
}

ELF_API void ELF_APIENTRY elfClearActorScript(elfActor* actor)
{
	if(actor->script) elfDecRef((elfObject*)actor->script);
	actor->script = NULL;
}

ELF_API void ELF_APIENTRY elfSetActorPosition(elfActor* actor, float x, float y, float z)
{
	actor->moved = ELF_TRUE;

	gfxSetTransformPosition(actor->transform, x, y, z);

	if(actor->object) elfSetPhysicsObjectPosition(actor->object, x, y, z);
	if(actor->dobject) elfSetPhysicsObjectPosition(actor->dobject, x, y, z);

	if(actor->objType == ELF_LIGHT) elfSetActorPosition((elfActor*)((elfLight*)actor)->shadowCamera, x, y, z);
}

ELF_API void ELF_APIENTRY elfSetActorRotation(elfActor* actor, float x, float y, float z)
{
	float orient[4];

	actor->moved = ELF_TRUE;

	gfxSetTransformRotation(actor->transform, x, y, z);
	gfxGetTransformOrientation(actor->transform, orient);

	if(actor->object) elfSetPhysicsObjectOrientation(actor->object, orient[0], orient[1], orient[2], orient[3]);
	if(actor->dobject) elfSetPhysicsObjectOrientation(actor->dobject, orient[0], orient[1], orient[2], orient[3]);

	if(actor->objType == ELF_LIGHT) elfSetActorRotation((elfActor*)((elfLight*)actor)->shadowCamera, x, y, z);
}

ELF_API void ELF_APIENTRY elfSetActorOrientation(elfActor* actor, float x, float y, float z, float w)
{
	actor->moved = ELF_TRUE;

	gfxSetTransformOrientation(actor->transform, x, y, z, w);

	if(actor->object) elfSetPhysicsObjectOrientation(actor->object, x, y, z, w);
	if(actor->dobject) elfSetPhysicsObjectOrientation(actor->dobject, x, y, z, w);

	if(actor->objType == ELF_LIGHT) elfSetActorOrientation((elfActor*)((elfLight*)actor)->shadowCamera, x, y, z, w);
}

ELF_API void ELF_APIENTRY elfRotateActor(elfActor* actor, float x, float y, float z)
{
	float orient[4];

	actor->moved = ELF_TRUE;

	gfxRotateTransform(actor->transform, x*eng->sync, y*eng->sync, z*eng->sync);
	gfxGetTransformOrientation(actor->transform, orient);

	if(actor->object) elfSetPhysicsObjectOrientation(actor->object, orient[0], orient[1], orient[2], orient[3]);
	if(actor->dobject) elfSetPhysicsObjectOrientation(actor->dobject, orient[0], orient[1], orient[2], orient[3]);

	if(actor->objType == ELF_LIGHT) elfRotateActor((elfActor*)((elfLight*)actor)->shadowCamera, x, y, z);
}

ELF_API void ELF_APIENTRY elfRotateActorLocal(elfActor* actor, float x, float y, float z)
{
	float orient[4];

	actor->moved = ELF_TRUE;

	gfxRotateTransformLocal(actor->transform, x*eng->sync, y*eng->sync, z*eng->sync);
	gfxGetTransformOrientation(actor->transform, orient);

	if(actor->object) elfSetPhysicsObjectOrientation(actor->object, orient[0], orient[1], orient[2], orient[3]);
	if(actor->dobject) elfSetPhysicsObjectOrientation(actor->dobject, orient[0], orient[1], orient[2], orient[3]);

	if(actor->objType == ELF_LIGHT) elfRotateActorLocal((elfActor*)((elfLight*)actor)->shadowCamera, x, y, z);
}

ELF_API void ELF_APIENTRY elfMoveActor(elfActor* actor, float x, float y, float z)
{
	float position[3];

	actor->moved = ELF_TRUE;

	gfxMoveTransform(actor->transform, x*eng->sync, y*eng->sync, z*eng->sync);
	gfxGetTransformPosition(actor->transform, position);

	if(actor->object) elfSetPhysicsObjectPosition(actor->object, position[0], position[1], position[2]);
	if(actor->dobject) elfSetPhysicsObjectPosition(actor->dobject, position[0], position[1], position[2]);

	if(actor->objType == ELF_LIGHT) elfMoveActor((elfActor*)((elfLight*)actor)->shadowCamera, x, y, z);
}

ELF_API void ELF_APIENTRY elfMoveActorLocal(elfActor* actor, float x, float y, float z)
{
	float position[3];

	actor->moved = ELF_TRUE;

	gfxMoveTransformLocal(actor->transform, x*eng->sync, y*eng->sync, z*eng->sync);
	gfxGetTransformPosition(actor->transform, position);

	if(actor->object) elfSetPhysicsObjectPosition(actor->object, position[0], position[1], position[2]);
	if(actor->dobject) elfSetPhysicsObjectPosition(actor->dobject, position[0], position[1], position[2]);

	if(actor->objType == ELF_LIGHT) elfMoveActorLocal((elfActor*)((elfLight*)actor)->shadowCamera, x, y, z);
}

ELF_API void ELF_APIENTRY elfSetActorPositionRelativeTo(elfActor* actor, elfActor* to, float x, float y, float z)
{
	elfVec3f vec;
	elfVec3f pos;
	elfVec3f result;
	elfVec4f orient;

	vec.x = x;
	vec.y = y;
	vec.z = z;

	elfGetActorOrientation_(to, &orient.x);
	elfGetActorPosition_(to, &pos.x);

	gfxMulQuaVec(&orient.x, &vec.x, &result.x);

	result.x += pos.x;
	result.y += pos.y;
	result.z += pos.z;

	elfSetActorPosition(actor, result.x, result.y, result.z);

	if(actor->objType == ELF_LIGHT) elfSetActorPositionRelativeTo((elfActor*)((elfLight*)actor)->shadowCamera, to, x, y, z);
}

ELF_API void ELF_APIENTRY elfSetActorRotationRelativeTo(elfActor* actor, elfActor* to, float x, float y, float z)
{
	elfVec4f lorient;
	elfVec4f orient;
	elfVec4f result;

	gfxQuaFromEuler(x, y, z, &lorient.x);

	elfGetActorOrientation_(to, &orient.x);

	gfxMulQuaQua(&orient.x, &lorient.x, &result.x);

	elfSetActorOrientation(actor, result.x, result.y, result.z, result.w);

	if(actor->objType == ELF_LIGHT) elfSetActorRotationRelativeTo((elfActor*)((elfLight*)actor)->shadowCamera, to, x, y, z);
}

void elfDirectActorAt(elfActor* actor, elfActor* at)
{
	elfVec3f dir;
	elfVec3f pos1;
	elfVec3f pos2;

	elfGetActorPosition_(actor, &pos1.x);
	elfGetActorPosition_(at, &pos2.x);

	dir.x = pos2.x-pos1.x;
	dir.y = pos2.y-pos1.y;
	dir.z = pos2.z-pos1.z;

	elfSetActorDirection(actor, dir.x, dir.y, dir.z);
}

void elfSetActorDirection(elfActor* actor, float x, float y, float z)
{
	elfVec4f orient;
	elfVec3f dir;

	dir.x = x;
	dir.y = y;
	dir.z = z;

	gfxVecNormalize(&dir.x);

	gfxQuaFromDirection(&dir.x, &orient.x);

	elfSetActorOrientation(actor, orient.x, orient.y, orient.z, orient.w);
}

ELF_API void ELF_APIENTRY elfSetActorOrientationRelativeTo(elfActor* actor, elfActor* to, float x, float y, float z, float w)
{
	elfVec4f lorient;
	elfVec4f orient;
	elfVec4f result;

	lorient.x = x;
	lorient.y = y;
	lorient.z = z;
	lorient.w = w;

	elfGetActorOrientation_(actor, &orient.x);

	gfxMulQuaQua(&lorient.x, &orient.x, &result.x);

	elfSetActorOrientation(actor, result.x, result.y, result.z, result.w);
}

ELF_API elfVec3f ELF_APIENTRY elfGetActorPosition(elfActor* actor)
{
	elfVec3f pos;
	gfxGetTransformPosition(actor->transform, &pos.x);
	return pos;
}

ELF_API elfVec3f ELF_APIENTRY elfGetActorRotation(elfActor* actor)
{
	elfVec3f rot;
	gfxGetTransformRotation(actor->transform, &rot.x);
	return rot;
}

ELF_API elfVec4f ELF_APIENTRY elfGetActorOrientation(elfActor* actor)
{
	elfVec4f orient;
	gfxGetTransformOrientation(actor->transform, &orient.x);
	return orient;
}

void elfGetActorPosition_(elfActor* actor, float* params)
{
	gfxGetTransformPosition(actor->transform, params);
}

void elfGetActorRotation_(elfActor* actor, float* params)
{
	gfxGetTransformRotation(actor->transform, params);
}

void elfGetActorOrientation_(elfActor* actor, float* params)
{
	gfxGetTransformOrientation(actor->transform, params);
}

ELF_API void ELF_APIENTRY elfSetActorBoundingLengths(elfActor* actor, float x, float y, float z)
{
	actor->pbbLengths.x = x;
	actor->pbbLengths.y = y;
	actor->pbbLengths.z = z;

	if(actor->object)
	{
		elfSetActorPhysics(actor, elfGetActorShape(actor),
			elfGetActorMass(actor));
	}
}

ELF_API void ELF_APIENTRY elfSetActorBoundingOffset(elfActor* actor, float x, float y, float z)
{
	actor->pbbOffsetSet = ELF_TRUE;

	actor->pbbOffset.x = x;
	actor->pbbOffset.y = y;
	actor->pbbOffset.z = z;

	if(actor->object)
	{
		elfSetActorPhysics(actor, elfGetActorShape(actor),
			elfGetActorMass(actor));
	}
}

void elfResetActorBoundingOffsetSetFlag(elfActor* actor)
{
	actor->pbbOffsetSet = ELF_FALSE;
}

ELF_API void ELF_APIENTRY elfSetActorPhysics(elfActor* actor, int shape, float mass)
{
	float position[3];
	float orient[4];
	float scale[3];
	float radius;
	elfJoint* joint;
	elfEntity* entity;

	elfDisableActorPhysics(actor);

	actor->shape = (unsigned char)shape;
	actor->mass = mass;
	actor->physics = ELF_TRUE;

	switch(shape)
	{
		case ELF_BOX:
		{
			actor->object = elfCreatePhysicsObjectBox(actor->pbbLengths.x/2.0,
				actor->pbbLengths.y/2.0, actor->pbbLengths.z/2.0, mass,
				actor->pbbOffset.x, actor->pbbOffset.y, actor->pbbOffset.z);
			break;
		}
		case ELF_SPHERE:
		{
			if(actor->pbbLengths.x > actor->pbbLengths.y && actor->pbbLengths.x > actor->pbbLengths.z)
				radius = actor->pbbLengths.x/2.0;
			else if(actor->pbbLengths.y > actor->pbbLengths.x && actor->pbbLengths.y > actor->pbbLengths.z)
				radius = actor->pbbLengths.y/2.0;
			else  radius = actor->pbbLengths.z/2.0;

			actor->object = elfCreatePhysicsObjectSphere(radius, mass,
				actor->pbbOffset.x, actor->pbbOffset.y, actor->pbbOffset.z);
			break;
		}
		case ELF_MESH:
		{
			if(actor->objType != ELF_ENTITY) return;
			entity = (elfEntity*)actor;
			if(!entity->model || !elfGetModelIndices(entity->model)) return;
			if(!entity->model->triMesh)
			{
				entity->model->triMesh = elfCreatePhysicsTriMesh(
					elfGetModelVertices(entity->model),
					elfGetModelIndices(entity->model),
					elfGetModelIndiceCount(entity->model));
				elfIncRef((elfObject*)entity->model->triMesh);
			}
			actor->object = elfCreatePhysicsObjectMesh(entity->model->triMesh, mass);
			break;
		}
		case ELF_CAPSULE_X:
		{
			if(actor->pbbLengths.z > actor->pbbLengths.y) radius = actor->pbbLengths.z/2.0;
			else radius = actor->pbbLengths.y/2.0;

			actor->object = elfCreatePhysicsObjectCapsule(ELF_CAPSULE_X, elfFloatMax(actor->pbbLengths.x-radius*2, 0.0), radius, mass,
				actor->pbbOffset.x, actor->pbbOffset.y, actor->pbbOffset.z);
			break;
		}
		case ELF_CAPSULE_Y:
		{
			if(actor->pbbLengths.z > actor->pbbLengths.x) radius = actor->pbbLengths.z/2.0;
			else radius = actor->pbbLengths.x/2.0;

			actor->object = elfCreatePhysicsObjectCapsule(ELF_CAPSULE_Y, elfFloatMax(actor->pbbLengths.y-radius*2, 0.0), radius, mass,
				actor->pbbOffset.x, actor->pbbOffset.y, actor->pbbOffset.z);
			break;
		}
		case ELF_CAPSULE_Z:
		{
			if(actor->pbbLengths.x > actor->pbbLengths.y) radius = actor->pbbLengths.x/2.0;
			else radius = actor->pbbLengths.y/2.0;

			actor->object = elfCreatePhysicsObjectCapsule(ELF_CAPSULE_Z, elfFloatMax(actor->pbbLengths.z-radius*2, 0.0), radius, mass,
				actor->pbbOffset.x, actor->pbbOffset.y, actor->pbbOffset.z);
			break;
		}
		case ELF_CONE_X:
		{
			if(actor->pbbLengths.z > actor->pbbLengths.y) radius = actor->pbbLengths.z/2.0;
			else radius = actor->pbbLengths.y/2.0;

			actor->object = elfCreatePhysicsObjectCone(ELF_CONE_X, actor->pbbLengths.x, radius, mass,
				actor->pbbOffset.x, actor->pbbOffset.y, actor->pbbOffset.z);
			break;
		}
		case ELF_CONE_Y:
		{
			if(actor->pbbLengths.z > actor->pbbLengths.x) radius = actor->pbbLengths.z/2.0;
			else radius = actor->pbbLengths.x/2.0;

			actor->object = elfCreatePhysicsObjectCone(ELF_CONE_Y, actor->pbbLengths.y, radius, mass,
				actor->pbbOffset.x, actor->pbbOffset.y, actor->pbbOffset.z);
			break;
		}
		case ELF_CONE_Z:
		{
			if(actor->pbbLengths.x > actor->pbbLengths.y) radius = actor->pbbLengths.x/2.0;
			else radius = actor->pbbLengths.y/2.0;

			actor->object = elfCreatePhysicsObjectCone(ELF_CONE_Z, actor->pbbLengths.z, radius, mass,
				actor->pbbOffset.x, actor->pbbOffset.y, actor->pbbOffset.z);
			break;
		}
		default: return;
	}

	elfSetPhysicsObjectActor(actor->object, (elfActor*)actor);
	elfIncRef((elfObject*)actor->object);

	gfxGetTransformPosition(actor->transform, position);
	gfxGetTransformOrientation(actor->transform, orient);
	gfxGetTransformScale(actor->transform, scale);

	elfSetPhysicsObjectPosition(actor->object, position[0], position[1], position[2]);
	elfSetPhysicsObjectOrientation(actor->object, orient[0], orient[1], orient[2], orient[3]);
	elfSetPhysicsObjectScale(actor->object, scale[0], scale[1], scale[2]);

	elfSetPhysicsObjectDamping(actor->object, actor->linDamp, actor->angDamp);
	elfSetPhysicsObjectSleepThresholds(actor->object, actor->linSleep, actor->angSleep);
	elfSetPhysicsObjectRestitution(actor->object, actor->restitution);
	elfSetPhysicsObjectAnisotropicFriction(actor->object, actor->anisFric.x, actor->anisFric.y, actor->anisFric.z);
	elfSetPhysicsObjectLinearFactor(actor->object, actor->linFactor.x, actor->linFactor.y, actor->linFactor.z);
	elfSetPhysicsObjectAngularFactor(actor->object, actor->angFactor.x, actor->angFactor.y, actor->angFactor.z);

	if(actor->scene) elfSetPhysicsObjectWorld(actor->object, actor->scene->world);

	// things are seriously going to blow up if we don't update the joints
	// when a new physics object has been created
	for(joint = (elfJoint*)elfBeginList(actor->joints); joint;
		joint = (elfJoint*)elfGetListNext(actor->joints))
	{
		elfSetJointWorld(joint, NULL);
		if(actor->scene) elfSetJointWorld(joint, actor->scene->world);
	}
}

ELF_API unsigned char ELF_APIENTRY elfIsActorPhysics(elfActor* actor)
{
	return actor->physics;
}

ELF_API void ELF_APIENTRY elfDisableActorPhysics(elfActor* actor)
{
	if(actor->object)
	{
		elfSetPhysicsObjectActor(actor->object, NULL);
		elfSetPhysicsObjectWorld(actor->object, NULL);
		elfDecRef((elfObject*)actor->object);
		actor->object = NULL;
	}

	actor->physics = ELF_FALSE;
}

ELF_API void ELF_APIENTRY elfSetActorDamping(elfActor* actor, float linDamp, float angDamp)
{
	actor->linDamp = linDamp;
	actor->angDamp = angDamp;
	if(actor->object) elfSetPhysicsObjectDamping(actor->object, linDamp, angDamp);
}

ELF_API void ELF_APIENTRY elfSetActorSleepThresholds(elfActor* actor, float linThrs, float angThrs)
{
	actor->linSleep = linThrs;
	actor->angSleep = angThrs;
	if(actor->object) elfSetPhysicsObjectSleepThresholds(actor->object, linThrs, angThrs);
}

ELF_API void ELF_APIENTRY elfSetActorRestitution(elfActor* actor, float restitution)
{
	actor->restitution = restitution;
	if(actor->object) elfSetPhysicsObjectRestitution(actor->object, restitution);
}

ELF_API void ELF_APIENTRY elfSetActorAnisotropicFriction(elfActor* actor, float x, float y, float z)
{
	actor->anisFric.x = x; actor->anisFric.y = y; actor->anisFric.z = z;
	if(actor->object) elfSetPhysicsObjectAnisotropicFriction(actor->object, x, y, z);
}

ELF_API void ELF_APIENTRY elfSetActorLinearFactor(elfActor* actor, float x, float y, float z)
{
	actor->linFactor.x = x; actor->linFactor.y = y; actor->linFactor.z = z;
	if(actor->object) elfSetPhysicsObjectLinearFactor(actor->object, x, y, z);
}

ELF_API void ELF_APIENTRY elfSetActorAngularFactor(elfActor* actor, float x, float y, float z)
{
	actor->angFactor.x = x; actor->angFactor.y = y; actor->angFactor.z = z;
	if(actor->object) elfSetPhysicsObjectAngularFactor(actor->object, x, y, z);
}

ELF_API void ELF_APIENTRY elfAddActorForce(elfActor* actor, float x, float y, float z)
{
	if(actor->object) elfAddPhysicsObjectForce(actor->object, x, y, z);
}

ELF_API void ELF_APIENTRY elfAddActorForceLocal(elfActor* actor, float x, float y, float z)
{
	elfVec3f vec;
	elfVec3f result;
	elfVec4f orient;

	if(actor->object)
	{
		vec.x = x;
		vec.y = y;
		vec.z = z;
		elfGetActorOrientation_(actor, &orient.x);
		gfxMulQuaVec(&orient.x, &vec.x, &result.x);
		elfAddPhysicsObjectForce(actor->object, result.x, result.y, result.z);
	}
}

ELF_API void ELF_APIENTRY elfAddActorTorque(elfActor* actor, float x, float y, float z)
{
	if(actor->object) elfAddPhysicsObjectTorque(actor->object, x, y, z);
}

ELF_API void ELF_APIENTRY elfSetActorLinearVelocity(elfActor* actor, float x, float y, float z)
{
	if(actor->object) elfSetPhysicsObjectLinearVelocity(actor->object, x, y, z);
}

ELF_API void ELF_APIENTRY elfSetActorLinearVelocityLocal(elfActor* actor, float x, float y, float z)
{
	elfVec3f vec;
	elfVec3f result;
	elfVec4f orient;

	if(actor->object)
	{
		vec.x = x;
		vec.y = y;
		vec.z = z;
		elfGetActorOrientation_(actor, &orient.x);
		gfxMulQuaVec(&orient.x, &vec.x, &result.x);
		elfSetPhysicsObjectLinearVelocity(actor->object, result.x, result.y, result.z);
	}
}

ELF_API void ELF_APIENTRY elfSetActorAngularVelocity(elfActor* actor, float x, float y, float z)
{
	if(actor->object) elfSetPhysicsObjectAngularVelocity(actor->object, x, y, z);
}

ELF_API elfVec3f ELF_APIENTRY elfGetActorBoundingLengths(elfActor* actor)
{
	return actor->pbbLengths;
}

ELF_API elfVec3f ELF_APIENTRY elfGetActorBoundingOffset(elfActor* actor)
{
	return actor->pbbOffset;
}

ELF_API int ELF_APIENTRY elfGetActorShape(elfActor* actor)
{
	return actor->shape;
}

ELF_API float ELF_APIENTRY elfGetActorMass(elfActor* actor)
{
	return actor->mass;
}

ELF_API float ELF_APIENTRY elfGetActorLinearDamping(elfActor* actor)
{
	return actor->linDamp;
}

ELF_API float ELF_APIENTRY elfGetActorAngularDamping(elfActor* actor)
{
	return actor->angDamp;
}

ELF_API float ELF_APIENTRY elfGetActorLinearSleepThreshold(elfActor* actor)
{
	return actor->linSleep;
}

ELF_API float ELF_APIENTRY elfGetActorAngularSleepThreshold(elfActor* actor)
{
	return actor->angSleep;
}

ELF_API float ELF_APIENTRY elfGetActorRestitution(elfActor* actor)
{
	return actor->restitution;
}

ELF_API elfVec3f ELF_APIENTRY elfGetActorAnisotropicFriction(elfActor* actor)
{
	return actor->anisFric;
}

ELF_API elfVec3f ELF_APIENTRY elfGetActorLinearFactor(elfActor* actor)
{
	return actor->linFactor;
}

ELF_API elfVec3f ELF_APIENTRY elfGetActorAngularFactor(elfActor* actor)
{
	return actor->angFactor;
}

ELF_API elfVec3f ELF_APIENTRY elfGetActorLinearVelocity(elfActor* actor)
{
	elfVec3f result;
	memset(&result, 0x0, sizeof(elfVec3f));

	if(actor->object) elfGetPhysicsObjectLinearVelocity(actor->object, &result.x);

	return result;
}

ELF_API elfVec3f ELF_APIENTRY elfGetActorAngularVelocity(elfActor* actor)
{
	elfVec3f result;
	memset(&result, 0x0, sizeof(elfVec3f));

	if(actor->object) elfGetPhysicsObjectAngularVelocity(actor->object, &result.x);

	return result;
}

ELF_API elfJoint* ELF_APIENTRY elfAddActorHingeJoint(elfActor* actor, elfActor* actor2, const char* name, float px, float py, float pz, float ax, float ay, float az)
{
	elfJoint* joint;

	if(!actor->object || !actor2->object) return NULL;

	joint = elfCreateHingeJoint(actor, actor2, name, px, py, pz, ax, ay, az);

	elfAppendListObject(actor->joints, (elfObject*)joint);
	elfAppendListObject(actor2->joints, (elfObject*)joint);

	return joint;
}

ELF_API elfJoint* ELF_APIENTRY elfAddActorBallJoint(elfActor* actor, elfActor* actor2, const char* name, float px, float py, float pz)
{
	elfJoint* joint;

	if(!actor->object || !actor2->object) return NULL;

	joint = elfCreateBallJoint(actor, actor2, name, px, py, pz);

	elfAppendListObject(actor->joints, (elfObject*)joint);
	elfAppendListObject(actor2->joints, (elfObject*)joint);

	return joint;
}

ELF_API elfJoint* ELF_APIENTRY elfAddActorConeTwistJoint(elfActor* actor, elfActor* actor2, const char* name, float px, float py, float pz, float ax, float ay, float az)
{
	elfJoint* joint;

	if(!actor->object || !actor2->object) return NULL;

	joint = elfCreateConeTwistJoint(actor, actor2, name, px, py, pz, ax, ay, az);

	elfAppendListObject(actor->joints, (elfObject*)joint);
	elfAppendListObject(actor2->joints, (elfObject*)joint);

	return joint;
}

ELF_API elfJoint* ELF_APIENTRY elfGetActorJoint(elfActor* actor, const char* name)
{
	elfJoint* joint;

	for(joint = (elfJoint*)elfBeginList(actor->joints); joint;
		joint = (elfJoint*)elfGetListNext(actor->joints))
	{
		if(!strcmp(elfGetJointName(joint), name)) return joint;
	}

	return NULL;
}

ELF_API elfJoint* ELF_APIENTRY elfGetActorJointByIndex(elfActor* actor, int idx)
{
	return (elfJoint*)elfGetListObject(actor->joints, idx);
}

ELF_API unsigned char ELF_APIENTRY elfRemoveActorJoint(elfActor* actor, const char* name)
{
	elfJoint* joint;

	for(joint = (elfJoint*)elfBeginList(actor->joints); joint;
		joint = (elfJoint*)elfGetListNext(actor->joints))
	{
		if(!strcmp(elfGetJointName(joint), name))
		{
			if(elfGetJointActorA(joint) == actor)
				elfRemoveListObject(elfGetJointActorB(joint)->joints, (elfObject*)joint);
			else elfRemoveListObject(elfGetJointActorA(joint)->joints, (elfObject*)joint);
			elfClearJoint(joint);
			return elfRemoveListObject(actor->joints, (elfObject*)joint);
		}
	}

	return ELF_FALSE;
}

ELF_API unsigned char ELF_APIENTRY elfRemoveActorJointByIndex(elfActor* actor, int idx)
{
	elfJoint* joint;
	int i;

	for(i = 0, joint = (elfJoint*)elfBeginList(actor->joints); joint;
		joint = (elfJoint*)elfGetListNext(actor->joints), i++)
	{
		if(i == idx)
		{
			if(elfGetJointActorA(joint) == actor)
				elfRemoveListObject(elfGetJointActorB(joint)->joints, (elfObject*)joint);
			else elfRemoveListObject(elfGetJointActorA(joint)->joints, (elfObject*)joint);
			elfClearJoint(joint);
			return elfRemoveListObject(actor->joints, (elfObject*)joint);
		}
	}

	return ELF_FALSE;
}

ELF_API unsigned char ELF_APIENTRY elfRemoveActorJointByObject(elfActor* actor, elfJoint* joint)
{
	if(elfGetJointActorA(joint) == actor)
		elfRemoveListObject(elfGetJointActorB(joint)->joints, (elfObject*)joint);
	else elfRemoveListObject(elfGetJointActorA(joint)->joints, (elfObject*)joint);
	elfClearJoint(joint);
	return elfRemoveListObject(actor->joints, (elfObject*)joint);
}

ELF_API void ELF_APIENTRY elfSetActorIpo(elfActor* actor, elfIpo* ipo)
{
	if(!ipo) return;
	if(actor->ipo) elfDecRef((elfObject*)actor->ipo);
	actor->ipo = ipo;
	if(actor->ipo) elfIncRef((elfObject*)actor->ipo);
}

ELF_API elfIpo* ELF_APIENTRY elfGetActorIpo(elfActor* actor)
{
	return actor->ipo;
}

ELF_API void ELF_APIENTRY elfSetActorIpoFrame(elfActor* actor, float frame)
{
	elfSetFramePlayerFrame(actor->ipoPlayer, frame);
}

ELF_API void ELF_APIENTRY elfPlayActorIpo(elfActor* actor, float start, float end, float speed)
{
	elfPlayFramePlayer(actor->ipoPlayer, start, end, speed);
}

ELF_API void ELF_APIENTRY elfLoopActorIpo(elfActor* actor, float start, float end, float speed)
{
	elfLoopFramePlayer(actor->ipoPlayer, start, end, speed);
}

ELF_API void ELF_APIENTRY elfStopActorIpo(elfActor* actor)
{
	elfStopFramePlayer(actor->ipoPlayer);
}

ELF_API void ELF_APIENTRY elfPauseActorIpo(elfActor* actor)
{
	elfStopFramePlayer(actor->ipoPlayer);
}

ELF_API void ELF_APIENTRY elfResumeActorIpo(elfActor* actor)
{
	elfStopFramePlayer(actor->ipoPlayer);
}

ELF_API float ELF_APIENTRY elfGetActorIpoStart(elfActor* actor)
{
	return elfGetFramePlayerStart(actor->ipoPlayer);
}

ELF_API float ELF_APIENTRY elfGetActorIpoEnd(elfActor* actor)
{
	return elfGetFramePlayerEnd(actor->ipoPlayer);
}

ELF_API float ELF_APIENTRY elfGetActorIpoSpeed(elfActor* actor)
{
	return elfGetFramePlayerSpeed(actor->ipoPlayer);
}

ELF_API float ELF_APIENTRY elfGetActorIpoFrame(elfActor* actor)
{
	return elfGetFramePlayerFrame(actor->ipoPlayer);
}

ELF_API unsigned char ELF_APIENTRY elfIsActorIpoPlaying(elfActor* actor)
{
	return elfIsFramePlayerPlaying(actor->ipoPlayer);
}

ELF_API unsigned char ELF_APIENTRY elfIsActorIpoPaused(elfActor* actor)
{
	return elfIsFramePlayerPaused(actor->ipoPlayer);
}

ELF_API int ELF_APIENTRY elfGetActorCollisionCount(elfActor* actor)
{
	if(actor->object) return elfGetPhysicsObjectCollisionCount(actor->object);
	return 0;
}

ELF_API elfCollision* ELF_APIENTRY elfGetActorCollision(elfActor* actor, int idx)
{
	return elfGetPhysicsObjectCollision(actor->object, idx);
}

ELF_API int ELF_APIENTRY elfGetActorPropertyCount(elfActor* actor)
{
	return elfGetListLength(actor->properties);
}

ELF_API void ELF_APIENTRY elfAddPropertyToActor(elfActor* actor, elfProperty* property)
{
	elfAppendListObject(actor->properties, (elfObject*)property);
}

ELF_API elfProperty* ELF_APIENTRY elfGetActorPropertyByName(elfActor* actor, const char* name)
{
	elfProperty* prop;

	if(!name || strlen(name) < 1) return ELF_FALSE;

	for(prop = (elfProperty*)elfBeginList(actor->properties); prop;
		prop = (elfProperty*)elfGetListNext(actor->properties))
	{
		if(!strcmp(prop->name, name))
		{
			return prop;
		}
	}

	return NULL;
}

ELF_API elfProperty* ELF_APIENTRY elfGetActorPropertyByIndex(elfActor* actor, int idx)
{
	return (elfProperty*)elfGetListObject(actor->properties, idx);
}

ELF_API unsigned char ELF_APIENTRY elfRemoveActorPropertyByName(elfActor* actor, const char* name)
{
	elfProperty* prop;

	if(!name || strlen(name) < 1) return ELF_FALSE;

	for(prop = (elfProperty*)elfBeginList(actor->properties); prop;
		prop = (elfProperty*)elfGetListNext(actor->properties))
	{
		if(!strcmp(prop->name, name))
		{
			elfRemoveListObject(actor->properties, (elfObject*)prop);
			return ELF_TRUE;
		}
	}

	return ELF_FALSE;
}

ELF_API unsigned char ELF_APIENTRY elfRemoveActorPropertyByIndex(elfActor* actor, int idx)
{
	elfProperty* prop;
	int i;

	if(idx < 0 && idx >= elfGetListLength(actor->properties)) return ELF_FALSE;

	for(i = 0, prop = (elfProperty*)elfBeginList(actor->properties); prop;
		prop = (elfProperty*)elfGetListNext(actor->properties), i++)
	{
		if(i == idx)
		{
			elfRemoveListObject(actor->properties, (elfObject*)prop);
			return ELF_TRUE;
		}
	}

	return ELF_FALSE;
}

ELF_API unsigned char ELF_APIENTRY elfRemoveActorPropertyByObject(elfActor* actor, elfProperty* property)
{
	return elfRemoveListObject(actor->properties, (elfObject*)property);
}

ELF_API void ELF_APIENTRY elfRemoveActorProperties(elfActor* actor)
{
	elfDecRef((elfObject*)actor->properties);
	actor->properties = elfCreateList();
	elfIncRef((elfObject*)actor->properties);
}

ELF_API void ELF_APIENTRY elfSetActorSelected(elfActor* actor, unsigned char selected)
{
	actor->selected = !selected == ELF_FALSE;
}

ELF_API unsigned char ELF_APIENTRY elfGetActorSelected(elfActor* actor)
{
	return actor->selected;
}

void elfDrawActorDebug(elfActor* actor, gfxShaderParams* shaderParams)
{
	float min[3];
	float max[3];
	float* vertexBuffer;
	float step;
	float size;
	int i;
	float halfLength;

	if(!actor->selected) return;

	if(elfIsActorPhysics(actor)) gfxSetColor(&shaderParams->materialParams.diffuseColor, 0.5, 1.0, 0.5, 1.0);
	else gfxSetColor(&shaderParams->materialParams.diffuseColor, 0.5, 0.5, 1.0, 1.0);
	gfxSetShaderParams(shaderParams);

	vertexBuffer = (float*)gfxGetVertexDataBuffer(eng->lines);

	if(actor->shape == ELF_BOX)
	{
		min[0] = -actor->pbbLengths.x/2+actor->pbbOffset.x;
		min[1] = -actor->pbbLengths.y/2+actor->pbbOffset.y;
		min[2] = -actor->pbbLengths.z/2+actor->pbbOffset.z;
		max[0] = actor->pbbLengths.x/2+actor->pbbOffset.x;
		max[1] = actor->pbbLengths.y/2+actor->pbbOffset.y;
		max[2] = actor->pbbLengths.z/2+actor->pbbOffset.z;

		vertexBuffer[0] = min[0];
		vertexBuffer[1] = max[1];
		vertexBuffer[2] = max[2];
		vertexBuffer[3] = min[0];
		vertexBuffer[4] = max[1];
		vertexBuffer[5] = min[2];

		vertexBuffer[6] = min[0];
		vertexBuffer[7] = max[1];
		vertexBuffer[8] = min[2];
		vertexBuffer[9] = min[0];
		vertexBuffer[10] = min[1];
		vertexBuffer[11] = min[2];

		vertexBuffer[12] = min[0];
		vertexBuffer[13] = min[1];
		vertexBuffer[14] = min[2];
		vertexBuffer[15] = min[0];
		vertexBuffer[16] = min[1];
		vertexBuffer[17] = max[2];

		vertexBuffer[18] = min[0];
		vertexBuffer[19] = min[1];
		vertexBuffer[20] = max[2];
		vertexBuffer[21] = min[0];
		vertexBuffer[22] = max[1];
		vertexBuffer[23] = max[2];

		vertexBuffer[24] = max[0];
		vertexBuffer[25] = max[1];
		vertexBuffer[26] = max[2];
		vertexBuffer[27] = max[0];
		vertexBuffer[28] = max[1];
		vertexBuffer[29] = min[2];

		vertexBuffer[30] = max[0];
		vertexBuffer[31] = max[1];
		vertexBuffer[32] = min[2];
		vertexBuffer[33] = max[0];
		vertexBuffer[34] = min[1];
		vertexBuffer[35] = min[2];

		vertexBuffer[36] = max[0];
		vertexBuffer[37] = min[1];
		vertexBuffer[38] = min[2];
		vertexBuffer[39] = max[0];
		vertexBuffer[40] = min[1];
		vertexBuffer[41] = max[2];

		vertexBuffer[42] = max[0];
		vertexBuffer[43] = min[1];
		vertexBuffer[44] = max[2];
		vertexBuffer[45] = max[0];
		vertexBuffer[46] = max[1];
		vertexBuffer[47] = max[2];

		vertexBuffer[48] = min[0];
		vertexBuffer[49] = max[1];
		vertexBuffer[50] = max[2];
		vertexBuffer[51] = max[0];
		vertexBuffer[52] = max[1];
		vertexBuffer[53] = max[2];

		vertexBuffer[54] = min[0];
		vertexBuffer[55] = min[1];
		vertexBuffer[56] = max[2];
		vertexBuffer[57] = max[0];
		vertexBuffer[58] = min[1];
		vertexBuffer[59] = max[2];

		vertexBuffer[60] = min[0];
		vertexBuffer[61] = min[1];
		vertexBuffer[62] = min[2];
		vertexBuffer[63] = max[0];
		vertexBuffer[64] = min[1];
		vertexBuffer[65] = min[2];

		vertexBuffer[66] = min[0];
		vertexBuffer[67] = max[1];
		vertexBuffer[68] = min[2];
		vertexBuffer[69] = max[0];
		vertexBuffer[70] = max[1];
		vertexBuffer[71] = min[2];

		gfxSetShaderParams(shaderParams);
		gfxDrawLines(24, eng->lines);
	}
	else if(actor->shape == ELF_SPHERE)
	{
		step = (360.0/((float)128))*GFX_PI_DIV_180;
		
		if(actor->pbbLengths.x > actor->pbbLengths.y && actor->pbbLengths.x > actor->pbbLengths.z)
			size = actor->pbbLengths.x/2.0;
		else if(actor->pbbLengths.y > actor->pbbLengths.x && actor->pbbLengths.y > actor->pbbLengths.z)
			size = actor->pbbLengths.y/2.0;
		else size = actor->pbbLengths.z/2.0;

		for(i = 0; i < 128; i++)
		{
			vertexBuffer[i*3] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.x;
			vertexBuffer[i*3+1] = ((float)cos((float)(step*i)))*size+actor->pbbOffset.y;
			vertexBuffer[i*3+2] = actor->pbbOffset.z;
		}

		gfxDrawLineLoop(128, eng->lines);

		for(i = 0; i < 128; i++)
		{
			vertexBuffer[i*3] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.x;
			vertexBuffer[i*3+1] = actor->pbbOffset.y;
			vertexBuffer[i*3+2] = ((float)cos((float)(step*i)))*size+actor->pbbOffset.z;
		}

		gfxDrawLineLoop(128, eng->lines);

		for(i = 0; i < 128; i++)
		{
			vertexBuffer[i*3] = actor->pbbOffset.x;
			vertexBuffer[i*3+1] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.y;
			vertexBuffer[i*3+2] = ((float)cos((float)(step*i)))*size+actor->pbbOffset.z;
		}

		gfxDrawLineLoop(128, eng->lines);
	}
	else if(actor->shape == ELF_CAPSULE_X)
	{
		step = (360.0/((float)128))*GFX_PI_DIV_180;

		if(actor->pbbLengths.y > actor->pbbLengths.z) size = actor->pbbLengths.y/2.0;
		else size = actor->pbbLengths.z/2.0;

		halfLength = elfFloatMax(actor->pbbLengths.x-size*2, 0.0)/2.0;

		for(i = 0; i < 32; i++)
		{
			vertexBuffer[i*3] = ((float)cos((float)(step*i)))*size+halfLength+actor->pbbOffset.x;
			vertexBuffer[i*3+1] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.y;
			vertexBuffer[i*3+2] = actor->pbbOffset.z;
		}

		for(i = 32; i < 64; i++)
		{
			vertexBuffer[i*3] = ((float)cos((float)(step*i)))*size-halfLength+actor->pbbOffset.x;
			vertexBuffer[i*3+1] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.y;
			vertexBuffer[i*3+2] = actor->pbbOffset.z;
		}

		for(i = 64; i < 96; i++)
		{
			vertexBuffer[i*3] = ((float)cos((float)(step*i)))*size-halfLength+actor->pbbOffset.x;
			vertexBuffer[i*3+1] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.y;
			vertexBuffer[i*3+2] = actor->pbbOffset.z;
		}

		for(i = 96; i < 128; i++)
		{
			vertexBuffer[i*3] = ((float)cos((float)(step*i)))*size+halfLength+actor->pbbOffset.x;
			vertexBuffer[i*3+1] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.y;
			vertexBuffer[i*3+2] = actor->pbbOffset.z;
		}

		gfxDrawLineLoop(128, eng->lines);

		for(i = 0; i < 32; i++)
		{
			vertexBuffer[i*3] = ((float)cos((float)(step*i)))*size+halfLength+actor->pbbOffset.x;
			vertexBuffer[i*3+1] = actor->pbbOffset.y;
			vertexBuffer[i*3+2] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.z;
		}

		for(i = 32; i < 64; i++)
		{
			vertexBuffer[i*3] = ((float)cos((float)(step*i)))*size-halfLength+actor->pbbOffset.x;
			vertexBuffer[i*3+1] = actor->pbbOffset.y;
			vertexBuffer[i*3+2] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.z;
		}

		for(i = 64; i < 96; i++)
		{
			vertexBuffer[i*3] = ((float)cos((float)(step*i)))*size-halfLength+actor->pbbOffset.x;
			vertexBuffer[i*3+1] = +actor->pbbOffset.y;
			vertexBuffer[i*3+2] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.z;
		}

		for(i = 96; i < 128; i++)
		{
			vertexBuffer[i*3] = ((float)cos((float)(step*i)))*size+halfLength+actor->pbbOffset.x;
			vertexBuffer[i*3+1] = actor->pbbOffset.y;
			vertexBuffer[i*3+2] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.z;
		}

		gfxDrawLineLoop(128, eng->lines);
	}
	else if(actor->shape == ELF_CAPSULE_Y)
	{
		step = (360.0/((float)128))*GFX_PI_DIV_180;

		if(actor->pbbLengths.x > actor->pbbLengths.z) size = actor->pbbLengths.x/2.0;
		else size = actor->pbbLengths.z/2.0;

		halfLength = elfFloatMax(actor->pbbLengths.y-size*2, 0.0)/2.0;

		for(i = 0; i < 32; i++)
		{
			vertexBuffer[i*3] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.x;
			vertexBuffer[i*3+1] = ((float)cos((float)(step*i)))*size+halfLength+actor->pbbOffset.y;
			vertexBuffer[i*3+2] = actor->pbbOffset.z;
		}

		for(i = 32; i < 64; i++)
		{
			vertexBuffer[i*3] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.x;
			vertexBuffer[i*3+1] = ((float)cos((float)(step*i)))*size-halfLength+actor->pbbOffset.y;
			vertexBuffer[i*3+2] = actor->pbbOffset.z;
		}

		for(i = 64; i < 96; i++)
		{
			vertexBuffer[i*3] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.x;
			vertexBuffer[i*3+1] = ((float)cos((float)(step*i)))*size-halfLength+actor->pbbOffset.y;
			vertexBuffer[i*3+2] = actor->pbbOffset.z;
		}

		for(i = 96; i < 128; i++)
		{
			vertexBuffer[i*3] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.x;
			vertexBuffer[i*3+1] = ((float)cos((float)(step*i)))*size+halfLength+actor->pbbOffset.y;
			vertexBuffer[i*3+2] = actor->pbbOffset.z;
		}

		gfxDrawLineLoop(128, eng->lines);

		for(i = 0; i < 32; i++)
		{
			vertexBuffer[i*3] = actor->pbbOffset.x;
			vertexBuffer[i*3+1] = ((float)cos((float)(step*i)))*size+halfLength+actor->pbbOffset.y;
			vertexBuffer[i*3+2] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.z;
		}

		for(i = 32; i < 64; i++)
		{
			vertexBuffer[i*3] = actor->pbbOffset.x;
			vertexBuffer[i*3+1] = ((float)cos((float)(step*i)))*size-halfLength+actor->pbbOffset.y;
			vertexBuffer[i*3+2] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.z;
		}

		for(i = 64; i < 96; i++)
		{
			vertexBuffer[i*3] = actor->pbbOffset.x;
			vertexBuffer[i*3+1] = ((float)cos((float)(step*i)))*size-halfLength+actor->pbbOffset.y;
			vertexBuffer[i*3+2] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.z;
		}

		for(i = 96; i < 128; i++)
		{
			vertexBuffer[i*3] = actor->pbbOffset.x;
			vertexBuffer[i*3+1] = ((float)cos((float)(step*i)))*size+halfLength+actor->pbbOffset.y;
			vertexBuffer[i*3+2] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.z;
		}

		gfxDrawLineLoop(128, eng->lines);
	}
	else if(actor->shape == ELF_CAPSULE_Z)
	{
		step = (360.0/((float)128))*GFX_PI_DIV_180;

		if(actor->pbbLengths.x > actor->pbbLengths.y) size = actor->pbbLengths.x/2.0;
		else size = actor->pbbLengths.y/2.0;

		halfLength = elfFloatMax(actor->pbbLengths.z-size*2, 0.0)/2.0;

		for(i = 0; i < 32; i++)
		{
			vertexBuffer[i*3] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.x;
			vertexBuffer[i*3+1] = actor->pbbOffset.y;
			vertexBuffer[i*3+2] = ((float)cos((float)(step*i)))*size+halfLength+actor->pbbOffset.z;
		}

		for(i = 32; i < 64; i++)
		{
			vertexBuffer[i*3] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.x;
			vertexBuffer[i*3+1] = actor->pbbOffset.y;
			vertexBuffer[i*3+2] = ((float)cos((float)(step*i)))*size-halfLength+actor->pbbOffset.z;
		}

		for(i = 64; i < 96; i++)
		{
			vertexBuffer[i*3] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.x;
			vertexBuffer[i*3+1] = actor->pbbOffset.y;
			vertexBuffer[i*3+2] = ((float)cos((float)(step*i)))*size-halfLength+actor->pbbOffset.z;
		}

		for(i = 96; i < 128; i++)
		{
			vertexBuffer[i*3] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.x;
			vertexBuffer[i*3+1] = actor->pbbOffset.y;
			vertexBuffer[i*3+2] = ((float)cos((float)(step*i)))*size+halfLength+actor->pbbOffset.z;
		}

		gfxDrawLineLoop(128, eng->lines);

		for(i = 0; i < 32; i++)
		{
			vertexBuffer[i*3] = actor->pbbOffset.x;
			vertexBuffer[i*3+1] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.y;
			vertexBuffer[i*3+2] = ((float)cos((float)(step*i)))*size+halfLength+actor->pbbOffset.z;
		}

		for(i = 32; i < 64; i++)
		{
			vertexBuffer[i*3] = actor->pbbOffset.x;
			vertexBuffer[i*3+1] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.y;
			vertexBuffer[i*3+2] = ((float)cos((float)(step*i)))*size-halfLength+actor->pbbOffset.z;
		}

		for(i = 64; i < 96; i++)
		{
			vertexBuffer[i*3] = actor->pbbOffset.x;
			vertexBuffer[i*3+1] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.y;
			vertexBuffer[i*3+2] = ((float)cos((float)(step*i)))*size-halfLength+actor->pbbOffset.z;
		}

		for(i = 96; i < 128; i++)
		{
			vertexBuffer[i*3] = actor->pbbOffset.x;
			vertexBuffer[i*3+1] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.y;
			vertexBuffer[i*3+2] = ((float)cos((float)(step*i)))*size+halfLength+actor->pbbOffset.z;
		}

		gfxDrawLineLoop(128, eng->lines);
	}
	else if(actor->shape == ELF_CONE_X)
	{
		step = (360.0/((float)128))*GFX_PI_DIV_180;
		
		if(actor->pbbLengths.y > actor->pbbLengths.z) size = actor->pbbLengths.y/2.0;
		else size = actor->pbbLengths.z/2.0;

		halfLength = actor->pbbLengths.x/2.0;

		for(i = 0; i < 128; i++)
		{
			vertexBuffer[i*3] = -halfLength+actor->pbbOffset.z;
			vertexBuffer[i*3+1] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.x;
			vertexBuffer[i*3+2] = ((float)cos((float)(step*i)))*size+actor->pbbOffset.y;
		}

		gfxDrawLineLoop(128, eng->lines);

		vertexBuffer[0] = halfLength;
		vertexBuffer[1] = 0.0;
		vertexBuffer[2] = 0.0;
		vertexBuffer[3] = -halfLength;
		vertexBuffer[4] = size;
		vertexBuffer[5] = 0.0;
		vertexBuffer[6] = halfLength;
		vertexBuffer[7] = 0.0;
		vertexBuffer[8] = 0.0;
		vertexBuffer[9] = -halfLength;
		vertexBuffer[10] = -size;
		vertexBuffer[11] = 0.0;
		vertexBuffer[12] = halfLength;
		vertexBuffer[13] = 0.0;
		vertexBuffer[14] = 0.0;
		vertexBuffer[15] = -halfLength;
		vertexBuffer[16] = 0.0;
		vertexBuffer[17] = size;
		vertexBuffer[18] = halfLength;
		vertexBuffer[19] = 0.0;
		vertexBuffer[20] = 0.0;
		vertexBuffer[21] = -halfLength;
		vertexBuffer[22] = 0.0;
		vertexBuffer[23] = -size;

		gfxDrawLines(8, eng->lines);
	}
	else if(actor->shape == ELF_CONE_Y)
	{
		step = (360.0/((float)128))*GFX_PI_DIV_180;
		
		if(actor->pbbLengths.x > actor->pbbLengths.z) size = actor->pbbLengths.x/2.0;
		else size = actor->pbbLengths.z/2.0;

		halfLength = actor->pbbLengths.y/2.0;

		for(i = 0; i < 128; i++)
		{
			vertexBuffer[i*3] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.x;
			vertexBuffer[i*3+1] = -halfLength+actor->pbbOffset.z;
			vertexBuffer[i*3+2] = ((float)cos((float)(step*i)))*size+actor->pbbOffset.y;
		}

		gfxDrawLineLoop(128, eng->lines);

		vertexBuffer[0] = 0.0;
		vertexBuffer[1] = halfLength;
		vertexBuffer[2] = 0.0;
		vertexBuffer[3] = size;
		vertexBuffer[4] = -halfLength;
		vertexBuffer[5] = 0.0;
		vertexBuffer[6] = 0.0;
		vertexBuffer[7] = halfLength;
		vertexBuffer[8] = 0.0;
		vertexBuffer[9] = -size;
		vertexBuffer[10] = -halfLength;
		vertexBuffer[11] = 0.0;
		vertexBuffer[12] = 0.0;
		vertexBuffer[13] = halfLength;
		vertexBuffer[14] = 0.0;
		vertexBuffer[15] = 0.0;
		vertexBuffer[16] = -halfLength;
		vertexBuffer[17] = size;
		vertexBuffer[18] = 0.0;
		vertexBuffer[19] = halfLength;
		vertexBuffer[20] = 0.0;
		vertexBuffer[21] = 0.0;
		vertexBuffer[22] = -halfLength;
		vertexBuffer[23] = -size;

		gfxDrawLines(8, eng->lines);
	}
	else if(actor->shape == ELF_CONE_Z)
	{
		step = (360.0/((float)128))*GFX_PI_DIV_180;
		
		if(actor->pbbLengths.x > actor->pbbLengths.y) size = actor->pbbLengths.x/2.0;
		else size = actor->pbbLengths.y/2.0;

		halfLength = actor->pbbLengths.z/2.0;

		for(i = 0; i < 128; i++)
		{
			vertexBuffer[i*3] = -((float)sin((float)(step*i)))*size+actor->pbbOffset.x;
			vertexBuffer[i*3+1] = ((float)cos((float)(step*i)))*size+actor->pbbOffset.y;
			vertexBuffer[i*3+2] = -halfLength+actor->pbbOffset.z;
		}

		gfxDrawLineLoop(128, eng->lines);

		vertexBuffer[0] = 0.0;
		vertexBuffer[1] = 0.0;
		vertexBuffer[2] = halfLength;
		vertexBuffer[3] = size;
		vertexBuffer[4] = 0.0;
		vertexBuffer[5] = -halfLength;
		vertexBuffer[6] = 0.0;
		vertexBuffer[7] = 0.0;
		vertexBuffer[8] = halfLength;
		vertexBuffer[9] = -size;
		vertexBuffer[10] = 0.0;
		vertexBuffer[11] = -halfLength;
		vertexBuffer[12] = 0.0;
		vertexBuffer[13] = 0.0;
		vertexBuffer[14] = halfLength;
		vertexBuffer[15] = 0.0;
		vertexBuffer[16] = size;
		vertexBuffer[17] = -halfLength;
		vertexBuffer[18] = 0.0;
		vertexBuffer[19] = 0.0;
		vertexBuffer[20] = halfLength;
		vertexBuffer[21] = 0.0;
		vertexBuffer[22] = -size;
		vertexBuffer[23] = -halfLength;

		gfxDrawLines(8, eng->lines);
	}
}

