#include "../tsee.h"

bool TSEE_Rendering_Init(TSEE *tsee) {
	if (tsee->init->rendering) {
		return true;
	}
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		TSEE_Error("SDL could not initialize (%s)\n", SDL_GetError());
		return false;
	}

	if (IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP) == 0) {
		TSEE_Error("SDL_IMG could not initialize (%s)\n", IMG_GetError());
		return false;
	}

	tsee->window->window = SDL_CreateWindow("No window title set: Call TSEE_Window_SetTitle", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, tsee->window->width, tsee->window->height, SDL_WINDOW_SHOWN);
	if (tsee->window->window == NULL) {
		TSEE_Error("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		SDL_Quit();
		return false;
	}

	tsee->window->renderer = SDL_CreateRenderer(tsee->window->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (tsee->window->renderer == NULL) {
		TSEE_Error("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
		SDL_DestroyWindow(tsee->window->window);
		SDL_Quit();
		return false;
	}

	if (SDL_GetCurrentDisplayMode(0, &tsee->window->mode) != 0) {
		TSEE_Warn("Failed to get display mode (%s)\n", SDL_GetError());
		tsee->window->fps = 60;
	} else {
		tsee->window->fps = tsee->window->mode.refresh_rate;
		if (tsee->window->fps == 0) {
			TSEE_Warn("Unspecified refresh rate.\n");
			tsee->window->fps = 60;
		}
	}
	TSEE_Log("Framerate: %d\n", tsee->window->fps);

	tsee->init->rendering = true;
	return true;
}

void TSEE_Window_UpdateSize(TSEE *tsee) {
	SDL_SetWindowSize(tsee->window->window, tsee->window->width, tsee->window->height);
}

void TSEE_Window_Destroy(TSEE_Window *window) {
	SDL_DestroyRenderer(window->renderer);
	SDL_DestroyWindow(window->window);
	xfree(window->title);
}

bool TSEE_Window_SetTitle(TSEE *tsee, char *title) {
	if (tsee->window->window == NULL) {
		TSEE_Error("Window is not initialized\n");
		return false;
	}

	SDL_SetWindowTitle(tsee->window->window, title);
	tsee->window->title = strdup(title);
	return true;
}

bool TSEE_RenderAll(TSEE *tsee) {
	Uint64 start = SDL_GetPerformanceCounter();
	if (SDL_GetWindowFlags(tsee->window->window) & SDL_WINDOW_MINIMIZED) {
		SDL_Delay(25);
		return true;
	}
	SDL_SetRenderDrawColor(tsee->window->renderer, 0, 0, 0, 255);
	SDL_RenderClear(tsee->window->renderer);
	// Render parallax backgrounds
	if (!TSEE_Parallax_Render(tsee)) {
		TSEE_Warn("Failed to render all parallax backgrounds\n");
	}

	// Render all objects
	for (size_t i = 0; i < tsee->world->objects->size; i++) {
		TSEE_Object *obj = TSEE_Array_Get(tsee->world->objects, i);
		if (!TSEERenderObject(tsee, obj)) {
			TSEE_Warn("Failed to render object\n");
		}
	}

	// Render all text
	for (size_t i = 0; i < tsee->world->text->size; i++) {
		TSEE_Text *text = TSEE_Array_Get(tsee->world->text, i);
		if (!TSEE_Text_Render(tsee, text)) {
			TSEE_Warn("Failed to render text %s\n", text->text);
		}
	}

	if (!TSEE_UI_Render(tsee)) {
		TSEE_Warn("Failed to render all of UI\n");
	}

	if (tsee->debug->active) {
		char text[64];
		int height_off = 64;
		sprintf(text, "Event: %.3f ms", tsee->debug->event_time);
		TSEE_Text *tex = TSEE_Text_Create(tsee, "_default", text, (SDL_Color){255, 255, 255, SDL_ALPHA_OPAQUE});
		tex->texture->rect.x = 0;
		tex->texture->rect.y = height_off;
		height_off += tex->texture->rect.h;
		SDL_SetRenderDrawColor(tsee->window->renderer, 100, 100, 100, 255);
		SDL_RenderFillRect(tsee->window->renderer, &tex->texture->rect);
		TSEE_Text_Render(tsee, tex);
		TSEE_Text_Destroy(tex, true);
		sprintf(text, "Physics: %.3f ms", tsee->debug->physics_time);
		tex = TSEE_Text_Create(tsee, "_default", text, (SDL_Color){255, 255, 255, SDL_ALPHA_OPAQUE});
		tex->texture->rect.x = 0;
		tex->texture->rect.y = height_off;
		height_off += tex->texture->rect.h;
		SDL_SetRenderDrawColor(tsee->window->renderer, 100, 100, 100, 255);
		SDL_RenderFillRect(tsee->window->renderer, &tex->texture->rect);
		TSEE_Text_Render(tsee, tex);
		TSEE_Text_Destroy(tex, true);
		sprintf(text, "Render: %.3f ms", tsee->debug->render_time);
		tex = TSEE_Text_Create(tsee, "_default", text, (SDL_Color){255, 255, 255, SDL_ALPHA_OPAQUE});
		tex->texture->rect.x = 0;
		tex->texture->rect.y = height_off;
		height_off += tex->texture->rect.h;
		SDL_SetRenderDrawColor(tsee->window->renderer, 100, 100, 100, 255);
		SDL_RenderFillRect(tsee->window->renderer, &tex->texture->rect);
		TSEE_Text_Render(tsee, tex);
		TSEE_Text_Destroy(tex, true);
		sprintf(text, "Frame: %.3f ms", tsee->debug->frame_time);
		tex = TSEE_Text_Create(tsee, "_default", text, (SDL_Color){255, 255, 255, SDL_ALPHA_OPAQUE});
		tex->texture->rect.x = 0;
		tex->texture->rect.y = height_off;
		SDL_SetRenderDrawColor(tsee->window->renderer, 100, 100, 100, 255);
		SDL_RenderFillRect(tsee->window->renderer, &tex->texture->rect);
		TSEE_Text_Render(tsee, tex);
		TSEE_Text_Destroy(tex, true);
	}

	SDL_RenderPresent(tsee->window->renderer);
	tsee->debug->framerate = 1000 / (tsee->window->last_render - SDL_GetPerformanceCounter()) / (double)SDL_GetPerformanceFrequency();
	tsee->window->last_render = SDL_GetPerformanceCounter();
	tsee->debug->render_time = (tsee->window->last_render - start) * 1000 / (double)SDL_GetPerformanceFrequency();
	tsee->debug->frame_time = tsee->debug->event_time + tsee->debug->physics_time + tsee->debug->render_time;
	tsee->debug->event_time = 0;
	tsee->debug->physics_time = 0;
	return true;
}

bool TSEE_Rendering_IsReady(TSEE *tsee) {
	float timeBetweenFrames = 1.0f / tsee->window->fps;
	float dt = (float) ( (SDL_GetPerformanceCounter() - tsee->window->last_render) / (float) SDL_GetPerformanceFrequency() );
	return dt >= timeBetweenFrames;
}