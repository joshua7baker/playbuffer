#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"

//Further features/changes made to the game

//Player
//Added a gun timer which only allow the player to shoot x bullets per y amount of frames
//The player has 3 lives meaning they can be hit 3 times before losing

//Coins get destroyed when the player restarts the game after losing
//Coins use the coins_2 sprite when being destroyed by the player for the visual effect

//Added levels (Each level has an assigned set of enemies to be killed before the level will progress to the next (UpdateLevelFunction)
//Levels are initialized in the "InitializeGame" function
	//level 1: spanners and drivers
	//level 2: enemies take two hits to kill
	//level 3: boss stage in level 3 (big tools)
//various tweaks to game variables to create a more balanced experience


int DISPLAY_WIDTH = 1280;
int DISPLAY_HEIGHT = 720;
int DISPLAY_SCALE = 1;

enum Agent8State
{
	STATE_APPEAR = 0,
	STATE_HALT,
	STATE_PLAY,
	STATE_DEAD
};

enum LevelState
{
	LEVEL_ONE = 0,
	LEVEL_TWO,
	LEVEL_THREE,
	LEVEL_COMPLETE
};

struct GameState
{
	int score = 0;
	Agent8State agentState = STATE_APPEAR;
	LevelState levelState = LEVEL_ONE;
};

GameState gameState;

enum GameObjectType 
{
	TYPE_NULL = -1,
	TYPE_AGENT8,
	TYPE_FAN,
	TYPE_TOOL,
	TYPE_COIN,
	TYPE_STAR,
	TYPE_LASER,
	TYPE_DESTROYED,
};

//score struct to hold individual score entries, set as a struct with for future username insertion
struct Score
{
	std::string name{"Default"};
	int score{ 0 };
};

std::vector<Score> scores;

std::list<Score> scoreboard;

//Player values
int playerCurrentHealth = 0;
int fireRate = 0;
int gunTimer = 0;
float projectileSpeed = 0;

//Level values
int enemyStartingHealth = 0;
int enemySpawnRate = 0;
int enemyCount = 0;

bool gameInitiliazing = true;
bool bossStage = false;
bool gameOver = false;
bool gameComplete = false;

VOID UpdateAgent8();
VOID UpdateFan();
VOID UpdateTools();
VOID UpdateCoinsAndStars();
VOID UpdateLasers();
VOID UpdateDestroyed();
VOID UpdateGunTimer();
VOID InitializeGame();
VOID UpdateLevel();
VOID DisplayHealthInterface();

// The entry point for a PlayBuffer program
void MainGameEntry( PLAY_IGNORE_COMMAND_LINE )
{
	Play::CreateManager( DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE );
	Play::CentreAllSpriteOrigins();
	Play::LoadBackground("Data\\Backgrounds\\background.png");
	Play::StartAudioLoop( "music");

	Play::CreateGameObject(TYPE_AGENT8, {115, 0}, 50, "agent8");
	int id_fan = Play::CreateGameObject(TYPE_FAN, { 1140, 217 }, 0, "fan");
	Play::GetGameObject(id_fan).velocity = {0,3};
	Play::GetGameObject(id_fan).animSpeed = 1.0f;

	InitializeGame();

}

// Called by PlayBuffer every frame (60 times a second!)
bool MainGameUpdate(float elapsedTime)
{
	Play::DrawBackground();
	UpdateAgent8();
	UpdateFan();
	UpdateTools();
	UpdateCoinsAndStars();
	UpdateLasers();
	UpdateDestroyed();
	UpdateGunTimer();

	//Score
	Play::DrawFontText("132px", "SCORE: " + std::to_string(gameState.score), { DISPLAY_WIDTH / 2, 50 }, Play::CENTRE);

	//Lives & Enemies
	Play::DrawFontText("64px", "Enemies Remaining: " + std::to_string(enemyCount), { 1120, 50 }, Play::CENTRE);
	Play::DrawFontText("64px", "Life " + std::to_string(playerCurrentHealth), { 50, 50 }, Play::CENTRE);

	//Game win/lose UI
	if (gameComplete)
		Play::DrawFontText("132px", "CONGRATULATIONS YOU WIN!", { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 }, Play::CENTRE);
	else
		Play::DrawFontText("64px", "ARROW KEYS TO MOVE UP AND DOWN AND SPACE TO FIRE", { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 30 }, Play::CENTRE);

	Play::PresentDrawingBuffer();

	return Play::KeyDown( VK_ESCAPE );
}

// Gets called once when the player quits the game 
int MainGameExit( void )
{
	Play::DestroyManager();
	return PLAY_OK;
}

void HandlePlayerControls()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	if (Play::KeyDown(VK_UP))
	{
		obj_agent8.velocity = { 0,-4 };
		Play::SetSprite(obj_agent8, "agent8_climb", 0.25f);
	}
	else if (Play::KeyDown(VK_DOWN))
	{
		obj_agent8.velocity = { 0, 3 };
		Play::SetSprite(obj_agent8, "agent8_fall", 0);
	}
	else if ((obj_agent8.velocity.y > 5))
	{
		gameState.agentState = STATE_HALT;
		Play::SetSprite(obj_agent8, "agent8_halt", 0.333f);
		obj_agent8.acceleration = { 0,0 };
	}
	else
	{
		Play::SetSprite(obj_agent8, "agent8_hang", 0.02f);
		obj_agent8.velocity *= 0.5f;
		obj_agent8.acceleration = { 0,0 };

	}

	if (Play::KeyDown(VK_SPACE) && gunTimer >= fireRate)
	{
		Vector2D firePos = obj_agent8.pos + Vector2D(155, -75);
		int id = Play::CreateGameObject(TYPE_LASER, firePos, 30, "laser");
		Play::GetGameObject(id).velocity = { 32 , 0 };
		Play::PlayAudio("shoot");
		gunTimer = 0;
	}

	Play::UpdateGameObject(obj_agent8);

	if (Play::IsLeavingDisplayArea(obj_agent8))
	{
		obj_agent8.pos = obj_agent8.oldPos;
	}
}

void UpdateFan()
{
	GameObject& obj_fan = Play::GetGameObjectByType(TYPE_FAN);

	if (enemyCount >= 1)
	{
		if (bossStage && enemyCount >= 1)
		{
			if (Play::RandomRoll(100) == 100)
			{
				int id = Play::CreateGameObject(TYPE_TOOL, obj_fan.pos, 75, "boss");
				GameObject& obj_tool = Play::GetGameObject(id);
				Play::SetSprite(obj_tool, "spanner_boss", 0);
				obj_tool.velocity = Point2f(-8, Play::RandomRollRange(-1, 1) * 6);
				obj_tool.rotSpeed = 0.1f;
			}
			if (Play::RandomRoll(50) == 50)
			{
				int id = Play::CreateGameObject(TYPE_TOOL, obj_fan.pos, 45, "driver");
				GameObject& obj_tool = Play::GetGameObject(id);
				Play::SetSprite(obj_tool, "driver", 0);
				obj_tool.radius = 100;
				obj_tool.velocity.x = -7;
			}
		}
		else if (Play::RandomRoll(50) == 50)
		{
			int id = Play::CreateGameObject(TYPE_TOOL, obj_fan.pos, 50, "driver");
			GameObject& obj_tool = Play::GetGameObject(id);
			obj_tool.velocity = Point2f(-8, Play::RandomRollRange(-1, 1) * 4);
			obj_tool.currentHealth = enemyStartingHealth;

			if (Play::RandomRoll(2) == 1)
			{
				Play::SetSprite(obj_tool, "spanner", 0);
				obj_tool.radius = 100;
				obj_tool.velocity.x = -4;
				obj_tool.rotSpeed = 0.1f;
			}
			Play::PlayAudio("tool");
		}
	}


	if (Play::RandomRoll(150) == 1)
	{
		int id = Play::CreateGameObject(TYPE_COIN, obj_fan.pos, 50, "coin");
		GameObject& obj_coin = Play::GetGameObject(id);
		obj_coin.velocity = { -3, 0 };
		obj_coin.rotSpeed = 0.1f;
	}

	Play::UpdateGameObject(obj_fan);

	if (Play::IsLeavingDisplayArea(obj_fan))
	{
		obj_fan.pos - obj_fan.oldPos;
		obj_fan.velocity.y *= -1;
	}
	Play::DrawObject(obj_fan);
}

void UpdateTools()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	std::vector<int> vTools = Play::CollectGameObjectIDsByType(TYPE_TOOL);

	for (int id : vTools)
	{
		GameObject& obj_tool = Play::GetGameObject(id);

		if (!obj_tool.hasCollided)
		{
			if (gameState.agentState != STATE_DEAD && Play::IsColliding(obj_tool, obj_agent8))
			{

				obj_agent8.currentHealth -= 1;
				playerCurrentHealth = obj_agent8.currentHealth;
				Play::PlayAudio("ouch");

				if (obj_agent8.currentHealth <= 0)
				{
					Play::StopAudioLoop("music");
					Play::PlayAudio("die");
					gameState.agentState = STATE_DEAD;
				}

				obj_tool.hasCollided = true;
			}
		}
		Play::UpdateGameObject(obj_tool);

		if (Play::IsLeavingDisplayArea(obj_tool, Play::VERTICAL))
		{
			obj_tool.pos = obj_tool.oldPos;
			obj_tool.velocity.y *= -1;
		}
		Play::DrawObjectRotated(obj_tool);

		if (!Play::IsVisible(obj_tool))
		{
			Play::DestroyGameObject(id);
		}
	}
}

void UpdateCoinsAndStars()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	std::vector<int> vCoins = Play::CollectGameObjectIDsByType(TYPE_COIN);

	for (int id_coin : vCoins)
	{
		GameObject& obj_coin = Play::GetGameObject(id_coin);
		bool hasCollided = false;

		if (gameState.agentState!= STATE_DEAD && Play::IsColliding(obj_coin, obj_agent8))
		{
			for (float rad{ 0.25f }; rad < 2.0f; rad += 0.5f)
			{
				int id = Play::CreateGameObject(TYPE_STAR, obj_agent8.pos, 0, "star");
				GameObject& obj_star = Play::GetGameObject(id);
				obj_star.rotSpeed = 0.1f;
				obj_star.acceleration = { 0.0f, 0.5f };
				Play::SetGameObjectDirection(obj_star, 16, rad * PLAY_PI);
			}
			hasCollided = true;
			gameState.score += 500;
			Play::PlayAudio("collect");
		}

		Play::UpdateGameObject(obj_coin);
		Play::DrawObjectRotated(obj_coin);

		if (!Play::IsVisible(obj_coin) || hasCollided)
		{
			Play::DestroyGameObject(id_coin);
		}
	}

	std::vector<int> vStars = Play::CollectGameObjectIDsByType(TYPE_STAR);

	for (int id_star : vStars)
	{
		GameObject& obj_star = Play::GetGameObject(id_star);

		Play::UpdateGameObject(obj_star);
		Play::DrawObjectRotated(obj_star);

		if (!Play::IsVisible(obj_star))
		{
			Play::DestroyGameObject(id_star);
		}
	}
}

void UpdateLasers()
{
	std::vector<int> vLasers = Play::CollectGameObjectIDsByType(TYPE_LASER);
	std::vector<int> vTools = Play::CollectGameObjectIDsByType(TYPE_TOOL);
	std::vector<int> vCoins = Play::CollectGameObjectIDsByType(TYPE_COIN);

	for (int id_laser : vLasers)
	{
		GameObject& obj_laser = Play::GetGameObject(id_laser);
		bool hasCollided = false;
		for (int id_tool : vTools)
		{
			GameObject& obj_tool = Play::GetGameObject(id_tool);
			if (Play::IsColliding(obj_laser, obj_tool))
			{
				hasCollided = true;
 				obj_tool.currentHealth -= 1;

				if (obj_tool.currentHealth <= 0)
				{
					obj_tool.type = TYPE_DESTROYED;
					enemyCount -= 1;

					if (enemyCount <= 0)
						UpdateLevel();
				}

				gameState.score += 100;
			}
		}

		for (int id_coin : vCoins)
		{
			GameObject& obj_coin = Play::GetGameObject(id_coin);
			bool hasCollided = false;
			if (Play::IsColliding(obj_laser, obj_coin))
			{
				hasCollided = true;
				Play::SetSprite(obj_coin, "coins_2", 0);
				obj_coin.type = TYPE_DESTROYED,
				Play::PlayAudio("error");
				gameState.score -= 300;
			}
		}
		if (gameState.score < 0)
			gameState.score = 0;

		Play::UpdateGameObject(obj_laser);
		Play::DrawObject(obj_laser);

		if (!Play::IsVisible(obj_laser) || hasCollided)
			Play::DestroyGameObject(id_laser);
	}
}

void UpdateAgent8()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);

	switch (gameState.agentState)
	{
		case STATE_APPEAR:
			gameOver = false;
			obj_agent8.currentHealth = 3;
			playerCurrentHealth = obj_agent8.currentHealth;
			obj_agent8.velocity = { 0,12 };
			obj_agent8.acceleration = { 0, 0.5f };
			Play::SetSprite(obj_agent8, "agent8_fall", 0);
			obj_agent8.rotation = 0;
			if (obj_agent8.pos.y = DISPLAY_HEIGHT / 3)
				gameState.agentState = STATE_PLAY;
			break;

		case STATE_HALT:
			obj_agent8.velocity *= 0.9f;
			if (Play::IsAnimationComplete(obj_agent8))
				gameState.agentState = STATE_PLAY;
			break;

		case STATE_PLAY:
			HandlePlayerControls();
			break;

		case STATE_DEAD:
			obj_agent8.acceleration = { -0.3f, 0.5f };
			obj_agent8.rotation += 0.25f;

			gameOver = true;

			if (Play::KeyPressed(VK_SPACE))
			{
				Play::StopAudioLoop("music");
				Play::StartAudioLoop("music");

				gameState.agentState = STATE_APPEAR;
				obj_agent8.pos = { 115, 0 };
				obj_agent8.velocity = { 0,0 };
				obj_agent8.frame = 0;
				gameState.score = 0;

				for (int id_obj : Play::CollectGameObjectIDsByType(TYPE_TOOL))
					Play::GetGameObject(id_obj).type = TYPE_DESTROYED;

				for (int id_obj : Play::CollectGameObjectIDsByType(TYPE_COIN))
					Play::GetGameObject(id_obj).type = TYPE_DESTROYED;

				gameInitiliazing = true;
				InitializeGame();
			}
			break;
	} //End of switch on agent8 state

	Play::UpdateGameObject(obj_agent8);

	if (Play::IsLeavingDisplayArea(obj_agent8) && gameState.agentState != STATE_DEAD)
		obj_agent8.pos = obj_agent8.oldPos;

	Play::DrawLine({ obj_agent8.pos.x, 0 }, obj_agent8.pos, Play::cWhite);
	Play::DrawObjectRotated(obj_agent8);

}

void UpdateDestroyed()
{
	std::vector<int> vDestroyed = Play::CollectGameObjectIDsByType(TYPE_DESTROYED);

	for (int id_dead : vDestroyed)
	{
		GameObject& obj_dead = Play::GetGameObject(id_dead);
		obj_dead.animSpeed = 0.2f;
		Play::UpdateGameObject(obj_dead);

		if (obj_dead.frame % 2)
			Play::DrawObjectRotated(obj_dead, (10 - obj_dead.frame) / 10.0f);

		if (!Play::IsVisible(obj_dead) || obj_dead.frame >= 10)
			Play::DestroyGameObject(id_dead);
	}
}

void UpdateGunTimer()
{
	gunTimer++;
}

void InitializeGame()
{
	gameState.levelState = LEVEL_ONE;
	UpdateLevel();
	gameInitiliazing = false;
	bossStage = false;
	gameComplete = false;
}

void UpdateLevel()
{
	if (!gameInitiliazing)
	{
		if (enemyCount <= 0 && gameState.levelState == LEVEL_ONE)
		{
			gameState.levelState = LEVEL_TWO;
		}
		else if (enemyCount <= 0 && gameState.levelState == LEVEL_TWO)
		{
			gameState.levelState = LEVEL_THREE;
		}
		else if (enemyCount <= 0 && gameState.levelState == LEVEL_THREE)
		{
			gameState.levelState = LEVEL_COMPLETE;
		}
	}

	switch (gameState.levelState)
	{
		case LEVEL_ONE:
			enemyStartingHealth = 1;
			fireRate = 20;
			enemySpawnRate = 50;
			enemyCount = 15;
			break;

		case LEVEL_TWO:
			enemyStartingHealth = 2;
			fireRate = 17;
			enemySpawnRate = 75;
			enemyCount = 15;
			break;

		case LEVEL_THREE:
			enemyStartingHealth = 3;
			bossStage = true;
			fireRate = 16;
			enemyCount = 30;
			Play::StopAudioLoop("music");
			Play::StartAudioLoop("musicfast");
			break;

		case LEVEL_COMPLETE:
			gameComplete = true;

			for (int id_obj : Play::CollectGameObjectIDsByType(TYPE_TOOL))
				Play::GetGameObject(id_obj).type = TYPE_DESTROYED;

			Play::StopAudioLoop("musicfast");
			Play::StartAudioLoop("victorymusic");

	}
}








	


   