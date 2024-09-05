#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>
#include "data_path.hpp"

#include <random>

#include <fstream>

PlayMode::PlayMode() {

	//CREDIT: Jude Markabawi (jmarkaba@andrew.cmu.edu):
	//
	// std::string bg_filename = "output_of_asset_gen/background.bin";
	// std::ifstream bg_file(bg_filename, std::ios_base::binary);
	// bg_file.read(reinterpret_cast<char*> (ppu.background.data()), sizeof(ppu.background));
	//
	// ^^ this was taken then edited accordingly to load the background, tile table, and palette table


	std::string bg_filename = data_path("output_of_asset_gen/background.bin");
	std::ifstream bg_file(bg_filename, std::ios_base::binary);
	bg_file.read(reinterpret_cast<char*> (ppu.background.data()), sizeof(ppu.background));

	std::string tile_filename = data_path("output_of_asset_gen/tile_table.bin");
	std::ifstream tile_file(tile_filename, std::ios_base::binary);
	tile_file.read(reinterpret_cast<char*> (ppu.tile_table.data()), sizeof(ppu.tile_table));

	std::string palette_filename = data_path("output_of_asset_gen/palette_table.bin");
	std::ifstream palette_file(palette_filename, std::ios_base::binary);
	palette_file.read(reinterpret_cast<char*> (ppu.palette_table.data()), sizeof(ppu.palette_table));

	//inits apples
	for (uint32_t x = 0; x < 16; ++x) {
		apples[x] = Apple{NO_APPLE,glm::vec2(0.0f),false}; //just populate with no apple init all pos 0
	}
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//UPDATE PLAYER
	constexpr float PlayerSpeed = 100.0f;
	if (left.pressed) player_at.x -= PlayerSpeed * elapsed;
	if (right.pressed) player_at.x += PlayerSpeed * elapsed;
	if (down.pressed) player_at.y -= PlayerSpeed * elapsed;
	if (up.pressed) player_at.y += PlayerSpeed * elapsed;

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;

	//UPDATE EXISTING APPLES (COLLISIONS AND OFFSCREENS, SPAWN IF NO APPLE CURRENTLY EXISTS)
	constexpr float AppleSpeed = 50.0f;
	uint8_t current_apple_count = 0;
	for (uint32_t x = 0; x < apples.size(); ++x) { //loop over all apples
		if (apples[x].appletype != NO_APPLE){
			if((apples[x].apple_at.y<10.0f+player_at.y) && (apples[x].apple_at.x<10.0f+player_at.x) && (apples[x].apple_at.x>player_at.x-10.0f)){ //if at acceptable collision height
				apples[x] = Apple{NO_APPLE,glm::vec2(0.0f),false};
			} else if (apples[x].apple_at.y<0.0f){
				apples[x] = Apple{NO_APPLE,glm::vec2(0.0f),false}; //it fell off the screen
			} else {
				apples[x].apple_at.y -= AppleSpeed * elapsed; //just populate with no apple init all pos 0
			}
			current_apple_count+= 1;
		} 
	}

	//SPAWN NEW APPLES
	//Spawn at either 4 or 3 * ppu background height minus some random height and 0 1 2 3 * ppu background width plus some raodom wdith
	//75/25 good vs bad apple
	//have around 6 on screen

	for (uint32_t x = 0; x < apples.size(); ++x) { //loop over all apples
		if ((apples[x].appletype == NO_APPLE)&&(current_apple_count!=6)){
			auto rand_goodbad = rand() % 4; //0 1 2 3 
			if (rand_goodbad < 3.0f){
				apples[x].appletype = GOOD_APPLE;
			} else {
				apples[x].appletype = BAD_APPLE;
			}

			auto rand_height_chunk = rand() % 2; //0 1
			auto rand_height_extra = rand() % PPU466::BackgroundHeight; //range of height inbetween
			auto rand_width_chunk = rand() % 4; //0 1 2 3 4
			auto rand_width_extra = rand() % PPU466::BackgroundWidth; //range of width inbetween
			apples[x].apple_at.x = PPU466::BackgroundWidth*rand_width_chunk + rand_width_extra;
			apples[x].apple_at.y = PPU466::BackgroundHeight*(rand_height_chunk+3) - rand_height_extra;
			apples[x].consumed = false;
			current_apple_count+= 1;
		} 
	}

}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---

	//player sprite:
	ppu.sprites[0].x = int8_t(player_at.x);
	ppu.sprites[0].y = int8_t(player_at.y);
	ppu.sprites[0].index = 162;
	ppu.sprites[0].attributes = 2;

	//Draw apples as additional sprites:
	for (uint32_t i = 0; i < apples.size(); ++i) { //draw all sprite indexes needed minus the player
		if(apples[i].appletype!=NO_APPLE && apples[i].consumed == false){
			ppu.sprites[i+1].x = apples[i].apple_at.x;
			ppu.sprites[i+1].y = apples[i].apple_at.y;
			if(apples[i].appletype == GOOD_APPLE){
				ppu.sprites[i+1].attributes = 4;
				ppu.sprites[i+1].index = 164;
			} else {
				ppu.sprites[i+1].attributes = 5;
				ppu.sprites[i+1].index = 163;
			}
		} else { //move it offscreen
			ppu.sprites[i+1].y = 240.0f;
		}
	}

	//--- actually draw ---
	ppu.draw(drawable_size);
}
