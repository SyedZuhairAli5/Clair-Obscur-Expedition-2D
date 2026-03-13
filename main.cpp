#include <iostream>
#include "raylib.h"
#include <string>

using namespace std;

//game states: who's turn is it?
enum GameState{
    PLAYER_TURN,
    ENEMY_TURN,
    GAME_OVER
};

//enemy states: what is the enemy doing?
enum EnemyAttckState{
    WINDUP,
    ACTIVE, //where the hitbox will exist and be active
    RECOVERY
};

enum PlayerDefenseState{
    NONE,
    PARRY,
    DODGE,
    COOLDOWN
};

struct EnemyTimers{
    float windupTime;
    float attackActive;
    float recoveryTime;
    float timer;
};

struct PlayerTimers{
    float parryTime;
    float dodgeTime;
    float cooldownTime;
    float timer;
};

class Fighters{
    protected:
        int hp;
        int damage;
        Vector3 position;

    public:
        Rectangle hitboxRect;
        Color debugColor;

        Fighters(int h, int d, Vector3 pos): hp(h), damage(d), position(pos){
            hitboxRect.height = 100;
            hitboxRect.width = 100;
        }

        void takeDamage(int dmg){
            hp -= dmg;

            if(hp <= 0){
                hp = 0;
            }
        }

        void draw(Model m, Vector3 pos, Color c){
            DrawModel(m, {pos.x, pos.y, pos.z}, 2, c);
        }

        void drawHitbox(int x, int y, int width, int height){
            DrawRectangle(x, y, width, height, debugColor);
        }

        //getters
        bool isDead() const {return hp <= 0;}
        int getHP() const {return hp;}
        int getDmg() const {return damage;}
        Vector3 getPos() const {return position;}
        Rectangle getHitbox() const {return hitboxRect;}
};

class Player: public Fighters{
    private:
        PlayerTimers pt;
        bool parried;
        bool dodged;

    public:
        Player(int h, int d, Vector3 pos): Fighters(h, d, pos) {
            parried = false;
            dodged = false;
            hitboxRect.x = 650;
            hitboxRect.y = 270;
            debugColor = LIGHTGRAY;
        }

        void setPlayerTimes(float p, float d, float c){
            pt.parryTime = p;
            pt.dodgeTime = d;
            pt.cooldownTime = c;
        }

        //timer setters
        void resetPlayerTimer(){pt.timer = 0;}
        void updatePlayerTimer(float delta) {pt.timer += delta;}

        //getters
        bool getParried() const {return parried;}
        bool getDodged() const {return dodged;}
        float getParryTime() const {return pt.parryTime;}
        float getDodgeTime() const {return pt.dodgeTime;}
        float getCooldownTime() const {return pt.cooldownTime;}
        float getCurrentTime() const {return pt.timer;}

        //setters
        void setParried(bool tf) {parried = tf;}
        void setDodged(bool tf) {dodged = tf;}
};

class Enemy: public Fighters{
    private:
        EnemyTimers et;

    public:
        Enemy(int h, int d, Vector3 pos): Fighters(h, d, pos) {
            debugColor = YELLOW;
            hitboxRect.x = 570;
            hitboxRect.y = 270;
        }

        void setEnemyTimes(float wt, float at, float rt){
            et.windupTime = wt;
            et.attackActive = at;
            et.recoveryTime = rt;
            et.timer = 0;
        }

        //timer setters
        void resetEnemyTimer() {et.timer = 0;}
        void updateEnemyTimer(float delta) {et.timer += delta;}

        //getters for enemyTimes
        float getWindup() const {return et.windupTime;}
        float getAttackActive() const {return et.attackActive;}
        float getRecovery() const {return et.recoveryTime;}
        float getCurrentTime() const {return et.timer;}
};

int main()
{    
    const int screenW = 1280;
    const int screenH = 720;

    InitWindow(screenW, screenH, "Clair Obscur: Expedition 2D");
    SetTargetFPS(60);

    float delta;

    //camera
    Camera3D cam = Camera3D();
    cam.position = {0, 3.5f, 10.0f};
    cam.target = {0, 0, 0};
    cam.up = {0 , 1, 0};
    cam.fovy = 65;
    cam.projection = CAMERA_PERSPECTIVE;

    //models
    Mesh cubeMesh = GenMeshCube(1, 1, 1);
    Model cubeModel = LoadModelFromMesh(cubeMesh);

    //fighters
    Player player(50, 5, {5, 1, 0});
    Enemy enemy(50, 5, {-5, 1, 0});
    enemy.setEnemyTimes(0.75f, 0.25f, 0.35f);
    player.setPlayerTimes(0.1f, 0.45f, 0.3f);

    //state management
    GameState gameState = PLAYER_TURN;
    EnemyAttckState enemyAttackState = WINDUP;
    PlayerDefenseState playerDefenseState = NONE;

    //put the logic stuff before any of the drawing stuff unless you have to do so otherwise
    //IMPORTANT NOTE: YOU CAN HAVE PLAYER STATES AND ENEMY STATES RUN AT THE SAME TIME, THE WHILE LOOP IS RUNNING EVERY FRAME ANYWAYS, USE IT TO YOUR ADVANTAGE
    while (!WindowShouldClose())
    {
        delta = GetFrameTime();
        ClearBackground(RAYWHITE);

        switch(gameState)
        {
            case PLAYER_TURN:
                if(IsKeyPressed(KEY_SPACE)){
                    enemy.takeDamage(player.getDmg());
                    if(enemy.isDead()){
                        gameState = GAME_OVER;
                    }
                    else{
                        gameState = ENEMY_TURN;
                    }
                }
                break;

            case ENEMY_TURN:
                switch(enemyAttackState)
                {
                    case WINDUP:
                        if(enemy.getCurrentTime() < enemy.getWindup()){
                            enemy.updateEnemyTimer(delta);
                        }
                        else if(enemy.getCurrentTime() >= enemy.getWindup()){
                            enemyAttackState = ACTIVE;
                            enemy.resetEnemyTimer();
                        }
                        break;

                    case ACTIVE:
                        if(enemy.getCurrentTime() < enemy.getAttackActive()){
                            enemy.updateEnemyTimer(delta);
                            enemy.debugColor = RED;
                        }
                        else if(enemy.getCurrentTime() >= enemy.getAttackActive()){
                            if(CheckCollisionRecs(player.getHitbox(), enemy.getHitbox())){
                                cout << "NEEEGGGAA" << endl;
                                if(playerDefenseState == PARRY){
                                    cout << "PARRIED" << endl;
                                }
                                else if(playerDefenseState == DODGE){
                                    cout << "DODGED" << endl;
                                }
                                else{
                                player.takeDamage(enemy.getDmg());
                                }
                            }

                            if(player.isDead()){
                                gameState = GAME_OVER;
                            }

                            enemyAttackState = RECOVERY;
                            enemy.resetEnemyTimer();
                            enemy.debugColor = YELLOW;
                        }

                        break;

                    case RECOVERY:
                        if(enemy.getCurrentTime() < enemy.getRecovery()){
                            enemy.updateEnemyTimer(delta);
                        }
                        else if(enemy.getCurrentTime() >= enemy.getRecovery()){
                            enemyAttackState = WINDUP;
                            enemy.resetEnemyTimer();
                            gameState = PLAYER_TURN;
                        }
                        break;
                }

                break;

            case GAME_OVER:
                break;
        }

        //player input states
        switch(playerDefenseState)
        {
            case NONE:
                if(IsKeyPressed(KEY_P)){
                    player.setParried(true);
                    player.debugColor = BLUE;
                    playerDefenseState = PARRY;
                }
                else if(IsKeyPressed(KEY_D)){
                    player.setDodged(true);
                    player.debugColor = BLUE;
                    playerDefenseState = DODGE;
                }

                break;

            case PARRY:
                if(player.getCurrentTime() < player.getParryTime()){
                    player.updatePlayerTimer(delta);
                }
                else if(player.getCurrentTime() >= player.getParryTime()){
                    player.resetPlayerTimer();
                    player.debugColor = LIGHTGRAY;
                    playerDefenseState = COOLDOWN;
                }

                break;

            case DODGE:
                if(player.getCurrentTime() < player.getDodgeTime()){
                    player.updatePlayerTimer(delta);
                }
                else if(player.getCurrentTime() >= player.getDodgeTime()){
                    player.resetPlayerTimer();
                    player.debugColor = LIGHTGRAY;
                    playerDefenseState = COOLDOWN;
                }
                break;

            case COOLDOWN:
                if(player.getCurrentTime() < player.getCooldownTime()){
                    player.updatePlayerTimer(delta);
                }
                else if(player.getCurrentTime() >= player.getCooldownTime()){
                    player.resetPlayerTimer();
                    playerDefenseState = NONE;
                }
                break;
        }

        BeginDrawing();

        BeginMode3D(cam);
        
        DrawGrid(15, 1);
        player.draw(cubeModel, player.getPos(), BLUE);
        enemy.draw(cubeModel, enemy.getPos(), RED);

        EndMode3D();

        player.drawHitbox(player.hitboxRect.x, player.hitboxRect.y, player.hitboxRect.width, player.hitboxRect.height);
        enemy.drawHitbox(enemy.hitboxRect.x, enemy.hitboxRect.y, enemy.hitboxRect.width, enemy.hitboxRect.height);

        DrawText(TextFormat("Player HP: %i", player.getHP()), 1000, 50, 30, BLACK);
        DrawText(TextFormat("Enemy HP: %i", enemy.getHP()), 50, 50, 30, BLACK);

        if(gameState == GAME_OVER){
            if(enemy.isDead()){
                DrawText(TextFormat("PLAYER WINS"), 500, 650, 30, BLACK);
            }
            else{
                DrawText(TextFormat("ENEMY WINS"), 500, 650, 30, BLACK);
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}