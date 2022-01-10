#include "include/tsee.h"

bool TSEEInitInput(TSEE *tsee) {
	if (!tsee->events) {
		TSEEWarn("Attempted to inititalise TSEEInput without initialising TSEEEvents");
		return false;
	}
	tsee->events->keypress = TSEEKeyPress;
	tsee->events->keyrelease = TSEEKeyRelease;
	return true;
}

void TSEEKeyPress(void *tsee, SDL_Keycode keycode) {
	TSEE *tseeReal = tsee;
	switch (keycode) {
		case SDLK_w:
			tseeReal->player->movement.up = true;
			break;
		case SDLK_s:
			tseeReal->player->movement.down = true;
			break;
		case SDLK_a:
			tseeReal->player->movement.left = true;
			break;
		case SDLK_d:
			tseeReal->player->movement.right = true;
			break;
	}
}

void TSEEKeyRelease(void *tsee, SDL_Keycode keycode) {
	TSEE *tseeReal = tsee;
	switch (keycode) {
		case SDLK_w:
			tseeReal->player->movement.up = false;
			break;
		case SDLK_s:
			tseeReal->player->movement.down = false;
			break;
		case SDLK_a:
			tseeReal->player->movement.left = false;
			break;
		case SDLK_d:
			tseeReal->player->movement.right = false;
			break;
	}
}