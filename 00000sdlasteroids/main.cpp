#include<SDL.h>
#include<sdl_ttf.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <list>
#include <cmath>
#include <fstream>
#include <sstream>
#include <time.h>
#include "game.h"

//la clase juego:
Game* Game::s_pInstance = 0;

Game::Game()
{
	m_pRenderer = NULL;
	m_pWindow = NULL;
}

Game::~Game()
{

}

SDL_Window* g_pWindow = 0;
SDL_Renderer* g_pRenderer = 0;

bool Game::init(const char* title, int xpos, int ypos, int width,
	int height, bool fullscreen)
{
	// almacenar el alto y ancho del juego.
	m_gameWidth = width;
	m_gameHeight = height;

	// attempt to initialize SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) == 0)
	{
		int flags = 0;
		if (fullscreen)
		{
			flags = SDL_WINDOW_FULLSCREEN;
		}

		std::cout << "SDL init success\n";
		// init the window
		m_pWindow = SDL_CreateWindow(title, xpos, ypos,
			width, height, flags);
		if (m_pWindow != 0) // window init success
		{
			std::cout << "window creation success\n";
			m_pRenderer = SDL_CreateRenderer(m_pWindow, -1, 0);
			if (m_pRenderer != 0) // renderer init success
			{
				std::cout << "renderer creation success\n";
				SDL_SetRenderDrawColor(m_pRenderer,
					255, 255, 255, 255);
			}
			else
			{
				std::cout << "renderer init fail\n";
				return false; // renderer init fail
			}
		}
		else
		{
			std::cout << "window init fail\n";
			return false; // window init fail
		}
	}
	else
	{
		std::cout << "SDL init fail\n";
		return false; // SDL init fail
	}
	if (TTF_Init() == 0)
	{
		std::cout << "sdl font initialization success\n";
	}
	else
	{
		std::cout << "sdl font init fail\n";
		return false;
	}

	std::cout << "init success\n";
	m_bRunning = true; // everything inited successfully, start the main loop

	//Joysticks
	InputHandler::Instance()->initialiseJoysticks();

	//load images, sounds, music and fonts
	AssetsManager::Instance()->loadAssets();
	Mix_Volume(-1, 16); //adjust sound/music volume for all channels

	for (int i = 0; i < 15; i++)
	{
		asteroid *a = new asteroid();
		a->settings("rock", Vector2D(rand() % m_gameWidth, rand() % m_gameHeight), 64, 64, 16, 0, 0, rand() % 360, 25);
		entities.push_back(a);
	}
	
	p = new player();
	p->settings("spaceship", Vector2D(200, 200), 40, 40, 0, 0, 1, 0, 20);
	p->m_shield = true;
	p->m_shieldTime = 100;
	entities.push_back(p);
	
	ReadHiScores();
	
	state = SPLASH;

	return true;
}

void Game::render()
{
	SDL_RenderClear(m_pRenderer); // clear the renderer to the draw color

	AssetsManager::Instance()->draw("background", 0, 0, 1200, 800, m_pRenderer, SDL_FLIP_NONE);

	if (state == SPLASH)
	{
		AssetsManager::Instance()->draw("splash", 0, 0, 1200, 800, m_pRenderer, SDL_FLIP_NONE);

		for (auto i : entities)
			i->draw();
	}

		if (state == MENU)
		{
			AssetsManager::Instance()->draw("menu", 0, 0, 1200, 800, m_pRenderer, SDL_FLIP_NONE);

			for (auto i : entities)
				i->draw();

			////Show hi scores
			int y = 350;
			AssetsManager::Instance()->Text("HiScores", "font", 580 - 50, y, SDL_Color({ 255,255,255,0 }), getRenderer());
			for (int i = 0; i < 5; i++) {
				y += 30;
				AssetsManager::Instance()->Text(std::to_string(vhiscores[i]), "font", 580, y, SDL_Color({ 255,255,255,0 }), getRenderer());
			}
		}

		if (state == GAME)
		{
			for (auto i : entities)
				i->draw();

			// Draw the score
			std::string sc = "Lives: " + std::to_string(lives) + "   Score: " + std::to_string(score);
			AssetsManager::Instance()->Text(sc, "font", 0, 0, SDL_Color({ 255,255,255,0 }), getRenderer());
		}

		if (state == END_GAME)
		{
			AssetsManager::Instance()->draw("gameover", 0, 0, 1200, 800, m_pRenderer, SDL_FLIP_NONE);

			for (auto i : entities)
				i->draw();
		}

	SDL_RenderPresent(m_pRenderer); // draw to the screen
}

void Game::quit()
{
	m_bRunning = false;
}

void Game::clean()
{
	WriteHiScores();
	std::cout << "cleaning game\n";
	InputHandler::Instance()->clean();
	AssetsManager::Instance()->clearFonts();
	TTF_Quit();
	SDL_DestroyWindow(m_pWindow);
	SDL_DestroyRenderer(m_pRenderer);
	Game::Instance()->m_bRunning = false;
	SDL_Quit();
}

void Game::handleEvents()
{
	InputHandler::Instance()->update();

	//HandleKeys
	if (state == SPLASH)
	{
		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_SPACE))
		{
			state = MENU;
		}
	}

	if (state == MENU)
	{
		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_S))
		{
			state = GAME;
			lives = 3;
			score = 0;
			p->m_shield = true;
			p->m_shieldTime = 100;
		}
	}

	if (state == GAME)
	{
		//space is the fire key
		p->m_waitTime--;
		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_SPACE))
		{
			if (p->m_shield == false && p->m_waitTime <= 0)
			{
				bullet *b = new bullet();
				b->settings("fire_blue", Vector2D(p->m_position), 32, 64, 16, 0, 0, p->m_angle-90, 16);
				entities.push_back(b);
				AssetsManager::Instance()->playSound("laser", 0);
				p->m_waitTime = 25;
			}
		}

		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_Z))
		{
			p->m_shield = true;
		}

		if (p->m_shieldTime > 0)
		{
			p->m_shieldTime--;
		}
		else
		{
			if (!InputHandler::Instance()->isKeyDown(SDL_SCANCODE_Z)) p->m_shield = false;
		}

		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_RIGHT)) p->m_angle += 3;
		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_LEFT)) p->m_angle -= 3;
		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_UP))
			p->thrust = true;
		else
			p->thrust = false;
	}

	if (state == END_GAME)
	{
		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_SPACE)) state = MENU;
	}

}

bool Game::isCollide(Entity *a, Entity *b)
{
	return (b->m_position.m_x - a->m_position.m_x)*(b->m_position.m_x - a->m_position.m_x) +
		(b->m_position.m_y - a->m_position.m_y)*(b->m_position.m_y - a->m_position.m_y) <
		(a->m_radius + b->m_radius)*(a->m_radius + b->m_radius);
}

void Game::update()
{
	if (state == GAME)
	{
		for (auto a : entities)
		{
			for (auto b : entities)
			{
				if (a->m_name == "asteroid" && b->m_name == "bullet")
					if (isCollide(a, b))
					{
						a->m_life = false;
						b->m_life = false;
	
						//explosion
						Entity *e = new Entity();
						e->settings("type_C", Vector2D(a->m_position.m_x-32, a->m_position.m_y-32), 171, 171, 48, 0, 0, 0, 1);
						e->m_name = "explosion";
						entities.push_back(e);
						AssetsManager::Instance()->playSound("explosion", 0);
						score += 10;
	
						//create two little ones
						for (int i = 0; i < 2; i++)
						{
							if (a->m_radius == 15) continue;
							asteroid *e = new asteroid();
							e->settings("rock_small", Vector2D(a->m_position.m_x, a->m_position.m_y), 64, 64, 16, 0, 0, 0, 15);
							entities.push_back(e);
						}
	
					}
	
				if (a->m_name == "player" && b->m_name == "asteroid")
					if (isCollide(a, b) && a->m_shield == false)
					{
						b->m_life = false;
	
						//ship explosion
						Entity *e = new Entity();
						e->settings("type_B", Vector2D(a->m_position.m_x-44, a->m_position.m_y-44), 128, 128, 64, 0, 0, 0, 1); // 44 = (128 / 2) - (40 / 2)
						e->m_name = "explosion";
						entities.push_back(e);
						AssetsManager::Instance()->playSound("shipexplosion", 0);
						lives--;
						if (lives <= 0)
						{
							UpdateHiScores(score);
							AssetsManager::Instance()->playSound("gameover", 0);
							state = END_GAME;
						}
	
						//relocate the ship
						p->m_position.m_x = m_gameWidth / 2;
						p->m_position.m_y = m_gameHeight / 2;
						p->m_velocity.m_x = 0; p->m_velocity.m_y = 0;
						p->m_shield = true;
						p->m_shieldTime = 100;
					}
			}
		}
	
		if (p->thrust)
			p->m_currentRow = 1;
		else
			p->m_currentRow = 0;
	
	
		for (auto e : entities)
			if (e->m_name == "explosion")
				if (e->m_currentFrame > 45) e->m_life = false;
	
		/*if (rand() % 150 == 0)
		{
			asteroid *a = new asteroid();
			a->settings(sRock, 0, rand() % H, rand() % 360, 25);
			entities.push_back(a);
		}*/
	
		for (auto i = entities.begin(); i != entities.end();)
		{
			Entity *e = *i;
	
			e->update();
	
			if (e->m_life == false) { i = entities.erase(i); delete e; }
			else i++;
		}
	}
}

void Game::UpdateHiScores(int newscore)
{
	//new score to the end
	vhiscores.push_back(newscore);
	//sort
	sort(vhiscores.rbegin(), vhiscores.rend());
	//remove the last
	vhiscores.pop_back();
}

void Game::ReadHiScores()
{
	std::ifstream in("hiscores.dat");
	if (in.good())
	{
		std::string str;
		getline(in, str);
		std::stringstream ss(str);
		int n;
		for (int i = 0; i < 5; i++)
		{
			ss >> n;
			vhiscores.push_back(n);
		}
		in.close();
	}
	else
	{
		//if file does not exist fill with 5 scores
		for (int i = 0; i < 5; i++)
		{
			vhiscores.push_back(0);
		}
	}
}

void Game::WriteHiScores()
{
	std::ofstream out("hiscores.dat");
	for (int i = 0; i < 5; i++)
	{
		out << vhiscores[i] << " ";
	}
	out.close();
}

const int FPS = 60;
const int DELAY_TIME = 1000.0f / FPS;

int main(int argc, char* args[])
{
	srand(time(NULL));

	Uint32 frameStart, frameTime;

	std::cout << "game init attempt...\n";
	if (Game::Instance()->init("SDLAsteroids", 100, 100, 1200, 800,
		false))
	{
		std::cout << "game init success!\n";
		while (Game::Instance()->running())
		{
			frameStart = SDL_GetTicks(); //tiempo inicial

			Game::Instance()->handleEvents();
			Game::Instance()->update();
			Game::Instance()->render();

			frameTime = SDL_GetTicks() - frameStart; //tiempo final - tiempo inicial

			if (frameTime < DELAY_TIME)
			{
				SDL_Delay((int)(DELAY_TIME - frameTime)); //esperamos hasta completar los 60 fps
			}
		}
	}
	else
	{
		std::cout << "game init failure - " << SDL_GetError() << "\n";
		return -1;
	}
	std::cout << "game closing...\n";
	Game::Instance()->clean();
	return 0;
}