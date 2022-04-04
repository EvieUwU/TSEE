#include "../tsee.h"

char *TSEE_ReadFile_UntilNull(FILE *fp) {
	char c;
	int len = 0;
	char *buffer = NULL;
	while ((c = getc(fp)) != 0 && c != EOF) {
		char *newPtr = xrealloc(buffer, sizeof(*buffer) * (++len));
		if (!newPtr) {
			TSEE_Critical("Failed realloc for text buffer.\n");
			return NULL;
		}
		buffer = newPtr;
		buffer[len - 1] = c;
	}
	buffer[len] = '\0';
	return buffer;
}

bool TSEE_Map_Load(TSEE *tsee, char *fn) {
	// Open the file
	FILE *fp = fopen(fn, "rb");
	if (fp == NULL) {
		TSEE_Error("Failed to open map file (%s)\n", fn);
		return false;
	}

	// Clear arrays to be overwritten
	TSEE_Array_Clear(tsee->world->objects);

	// Reset the player
	tsee->player->grounded = true;
	tsee->player->jump_force = 0;
	tsee->player->movement.down = false;
	tsee->player->movement.up = false;
	tsee->player->movement.left = false;
	tsee->player->movement.right = false;
	tsee->player->physics_object = NULL;
	tsee->player->speed = 0;

	// Read all of the map header data
	char *mapName = NULL;
	if (!(mapName = TSEE_ReadFile_UntilNull(fp))) return false;
	char *mapAuthor = NULL;
	if (!(mapAuthor = TSEE_ReadFile_UntilNull(fp))) return false;
	char *mapVersion = NULL;
	if (!(mapVersion = TSEE_ReadFile_UntilNull(fp))) return false;
	char *mapDescription = NULL;
	if (!(mapDescription = TSEE_ReadFile_UntilNull(fp))) return false;
	TSEE_Log("Loading into %s by %s\nVersion: %s\n%s\n", mapName, mapAuthor, mapVersion, mapDescription);

	// Read the gravity (for some reason its here in the header?)
	float gravity = 0;
	fread(&gravity, sizeof(gravity), 1, fp);
	TSEE_World_SetGravity(tsee, gravity);

	// Read map's textures
	size_t numTextures = 0;
	fread(&numTextures, sizeof(numTextures), 1, fp);
	TSEE_Log("Loading %zu textures\n", numTextures);
	for (size_t i = 0; i < numTextures; i++) {
		char *texturePath = NULL;
		if (!(texturePath = TSEE_ReadFile_UntilNull(fp))) return false;
		TSEE_Texture *texture = TSEE_Texture_Create(tsee, texturePath);
		if (!texture) {
			TSEE_Error("Failed to load texture `%s` (%s)\n", texturePath, SDL_GetError());
			return false;
		}
	}

	// Read map's parallax layers
	size_t numParallax = 0;
	fread(&numParallax, sizeof(numParallax), 1, fp);
	TSEE_Log("Loading %zu parallax layers\n", numParallax);
	for (size_t i = 0; i < numParallax; i++) {
		TSEE_Parallax *parallax = xmalloc(sizeof(*parallax));
		size_t textureIdx = -1;
		fread(&textureIdx, sizeof(textureIdx), 1, fp);
		size_t dist = 0;
		fread(&dist, sizeof(dist), 1, fp);
		if (!TSEE_Parallax_Create(tsee, TSEE_Array_Get(tsee->textures, textureIdx), dist)) {
			TSEE_Warn("Failed to create parallax with texture index `%zu`\n", textureIdx);
		}
	}

	// Read objects
	size_t numObjects = 0;
	fread(&numObjects, sizeof(numObjects), 1, fp);
	TSEE_Log("Loading %zu objects\n", numObjects);
	for (size_t i = 0; i < numObjects; i++) {
		size_t texIdx = 0;
		fread(&texIdx, sizeof(texIdx), 1, fp);
		TSEE_Texture *tex = TSEE_Array_Get(tsee->textures, texIdx);
		int idx = TSEE_Create_Object(tsee, TSEE_Texture_Create(tsee, tex->path));
		TSEE_Object *object = TSEE_Array_Get(tsee->world->objects, idx);
		object->x = 0;
		object->y = 0;
		fread(&object->x, sizeof(object->x), 1, fp);
		fread(&object->y, sizeof(object->y), 1, fp);
		object->texture->rect.x = object->x;
		object->texture->rect.y = object->y;
		TSEE_Log("Loaded object `%s` at (%f, %f, %d, %d) with texture at (%d, %d)\n", object->texture->path, object->x, object->y, object->texture->rect.w, object->texture->rect.h, object->texture->rect.x, object->texture->rect.y);
	}

	// Read the physics objects
	size_t numPhysicsObjects = 0;
	fread(&numPhysicsObjects, sizeof(numPhysicsObjects), 1, fp);
	TSEE_Log("Loading %zu physics objects\n", numPhysicsObjects);
	for (size_t i = 0; i < numPhysicsObjects; i++) {
		size_t objectIdx = 0;
		fread(&objectIdx, sizeof(objectIdx), 1, fp);
		float mass = 0;
		fread(&mass, sizeof(mass), 1, fp);
		TSEE_Object *object = TSEE_Array_Get(tsee->world->objects, objectIdx);
		if (!TSEEConvertObjectToPhysicsObject(tsee, object, mass)) {
			TSEE_Warn("Failed to convert object to physics object\n");
		}
	}

	// Read the player
	int64_t playerIdx = 0;
	fread(&playerIdx, sizeof(playerIdx), 1, fp);
	if (playerIdx == -1)
		TSEE_Error("Failed to load player index.\n");
	else {
		TSEE_Log("Loaded player index %zu\n", playerIdx);
		TSEE_Player_Create(tsee, TSEE_Array_Get(tsee->world->physics_objects, playerIdx));
	}
	float speed = 0;
	fread(&speed, sizeof(speed), 1, fp);
	TSEE_Player_SetSpeed(tsee, speed);
	float jumpForce = 0;
	fread(&jumpForce, sizeof(jumpForce), 1, fp);
	TSEE_Player_SetJumpForce(tsee, jumpForce);

	fclose(fp);
	TSEE_Log("Map %s loaded successfully.\n", mapName);
	return true;
}

bool TSEE_Map_Save(TSEE *tsee, char *fn) {
	FILE *fp = fopen(fn, "wb");
	if (fp == NULL) {
		TSEE_Error("Failed to open map file (%s)\n", fn);
		return false;
	}
	// Write the map header
	char *mapName = "Test Map";
	char *mapAuthor = "Test Author";
	char *mapVersion = "1.0";
	char *mapDescription = "Test Description";
	fwrite(mapName, sizeof(*mapName) * strlen(mapName), 1, fp);
	putc(0, fp);
	fwrite(mapAuthor, sizeof(*mapAuthor) * strlen(mapAuthor), 1, fp);
	putc(0, fp);
	fwrite(mapVersion, sizeof(*mapVersion) * strlen(mapVersion), 1, fp);
	putc(0, fp);
	fwrite(mapDescription, sizeof(*mapDescription) * strlen(mapDescription), 1, fp);
	putc(0, fp);
	// Write the gravity
	float gravity = tsee->world->gravity;
	fwrite(&gravity, sizeof(gravity), 1, fp);
	// Write the number of textures
	size_t numTextures = tsee->textures->size;
	fwrite(&numTextures, sizeof(numTextures), 1, fp);
	for (size_t i = 0; i < tsee->textures->size; i++) {
		TSEE_Texture *texture = TSEE_Array_Get(tsee->textures, i);
		int pathSize = strlen(texture->path);
		fwrite(texture->path, pathSize * sizeof(char), 1, fp);
		putc(0, fp);
	}
	// Write the parallax backgrounds with texture indexes.
	fwrite(&tsee->world->parallax->size, sizeof(tsee->world->parallax->size), 1, fp);
	for (size_t i = 0; i < tsee->world->parallax->size; i++) {
		TSEE_Parallax *parallax = TSEE_Array_Get(tsee->world->parallax, i);
		for (size_t j = 0; j < tsee->textures->size; j++) {
			TSEE_Texture *texture = TSEE_Array_Get(tsee->textures, j);
			if (texture == parallax->texture) {
				size_t textureIdx = j;
				fwrite(&textureIdx, sizeof(textureIdx), 1, fp);
				break;
			}
		}
		size_t dist = parallax->distance;
		fwrite(&dist, sizeof(dist), 1, fp);
	}
	// Write the objects with texture indexes.
	fwrite(&tsee->world->objects->size, sizeof(tsee->world->objects->size), 1, fp);
	for (size_t i = 0; i < tsee->world->objects->size; i++) {
		bool written = false;
		TSEE_Object *object = TSEE_Array_Get(tsee->world->objects, i);
		for (size_t j = 0; j < tsee->textures->size; j++) {
			TSEE_Texture *texture = TSEE_Array_Get(tsee->textures, j);
			if (strcmp(texture->path, object->texture->path) == 0) {
				size_t textureIdx = j;
				fwrite(&textureIdx, sizeof(textureIdx), 1, fp);
				written = true;
				break;
			}
		}
		if (!written) {
			TSEE_Error("Couldn't write texture idx for texture!!\n");
		}
		fwrite(&object->x, sizeof(object->x), 1, fp);
		fwrite(&object->y, sizeof(object->y), 1, fp);
	}
	// Write the physics objects
	fwrite(&tsee->world->physics_objects->size, sizeof(tsee->world->physics_objects->size), 1, fp);
	for (size_t i = 0; i < tsee->world->physics_objects->size; i++) {
		TSEE_Physics_Object *object = TSEE_Array_Get(tsee->world->physics_objects, i);
		// Find which object it relates to
		for (size_t j = 0; j < tsee->world->objects->size; j++) {
			TSEE_Object *object2 = TSEE_Array_Get(tsee->world->objects, j);
			if (object2 == object->object) {
				size_t objectIdx = j;
				fwrite(&objectIdx, sizeof(objectIdx), 1, fp);
				break;
			}
		}
		fwrite(&object->mass, sizeof(object->mass), 1, fp);
	}
	// Write which physics object is the player
	int playerIdx = -1;
	if (tsee->player != NULL) {
		for (size_t i = 0; i < tsee->world->physics_objects->size; i++) {
			TSEE_Physics_Object *object = TSEE_Array_Get(tsee->world->physics_objects, i);
			if (object == tsee->player->physics_object) {
				playerIdx = i;
				break;
			}
		}
		if (playerIdx == -1) {
			TSEE_Error("Failed to find player physics object in map file\n");
		}
		int64_t towrite = playerIdx;
		fwrite(&towrite, sizeof(towrite), 1, fp);
		fwrite(&tsee->player->jump_force, sizeof(tsee->player->jump_force), 1, fp);
		fwrite(&tsee->player->speed, sizeof(tsee->player->speed), 1, fp);
	}
	fclose(fp);
	TSEE_Log("Saved map to %s\n", fn);
	return true;
}