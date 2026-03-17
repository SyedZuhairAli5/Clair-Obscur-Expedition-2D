#include <iostream>
#include "raylib.h"
#include <string>
#include <vector>

using namespace std;

//game states: who's turn is it?
enum GameState{
    PLAYER_TURN,
    ENEMY_TURN,
    GAME_OVER
};

enum AttackState{
    ATTACK_WINDUP,
    ATTACK_ACTIVE,
    ATTACK_RECOVERY,
    ATTACK_FINISHED
};

enum PlayerDefenseState{
    NONE,
    PARRY,
    DODGE,
    COOLDOWN
};

struct AttackData{
    string attackName;
    float attackWindup;
    float attackActive;
    float attackRecovery;
    int damage;
    int numOfAttacks;
    int apCost;
};

struct PlayerTimers{
    float parryTime;
    float dodgeTime;
    float cooldownTime;
    float timer;
};

class Attack{
    private:
        AttackState attackState;
        float timer;
        string attackName;
        float windupTime;
        float activeTime;
        float recoveryTime;
        int attackDamage;
        int numOfAttacks;
        Rectangle attackHitbox;
        bool hitResolved;

    public:
        Attack(string n ,float wt, float at, float rt, int dmg, int noa, Rectangle hb): attackName(n), windupTime(wt), activeTime(at), recoveryTime(rt), attackDamage(dmg), numOfAttacks(noa), attackHitbox(hb), attackState(ATTACK_WINDUP), timer(0), hitResolved(false) {}
        
        void attackUpdate(float delta)
        {
            timer += delta;

            switch(attackState)
            {
                case ATTACK_WINDUP:
                    if(timer >= windupTime){
                        attackState = ATTACK_ACTIVE;
                        timer = 0;
                    }
                    break;
            
                case ATTACK_ACTIVE:
                    if(timer >= activeTime){
                        attackState = ATTACK_RECOVERY;
                        timer = 0;
                        hitResolved = false;
                    }
                    break;
                
                case ATTACK_RECOVERY:
                    if(timer >= recoveryTime){
                        attackState = ATTACK_FINISHED;
                        timer = 0;
                        numOfAttacks --;

                        if(numOfAttacks > 0){
                            attackState = ATTACK_WINDUP;
                        }
                        else{
                            attackState = ATTACK_FINISHED;
                        }
                    }
                    break;
                
                case ATTACK_FINISHED:
                    break;
            }
        }

        //setters
        void markHitResolved() {hitResolved = true;}

        //getters
        Rectangle getAttackHitbox() const {return attackHitbox;}
        int getAttackDmg() const {return attackDamage;}
        AttackState getAttackState() const {return attackState;}
        string getName() const {return attackName;}
        int getNumAttacks() const {return numOfAttacks;}
        bool canResolveHit() const {return !hitResolved ;}
};

class Fighters{
    protected:
        int hp;
        int damage;
        Vector3 position;

        Attack* currAttack;
        vector<AttackData> attackList;
        vector<AttackData> skillList;

    public:
        Rectangle hitboxRect;
        Color debugColor;

        Fighters(int h, int d, Vector3 pos): hp(h), damage(d), position(pos){
            hitboxRect.height = 100;
            hitboxRect.width = 100;
            currAttack = nullptr;
        }

        virtual ~Fighters(){
            if(currAttack){
                delete currAttack;
            }
        }

        //choose function which will be defined in player and enemy differently
        virtual void chooseAndStartAttack() = 0;

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

        //attack class shenanigans
        void startAttack(const AttackData& data){
            if(currAttack != nullptr){return;}
            currAttack = new Attack (data.attackName, data.attackWindup, data.attackActive, data.attackRecovery, data.damage, data.numOfAttacks, hitboxRect);
        }

        void updateAttack(float delta){
            if(currAttack == nullptr){
                return;
            }

            currAttack->attackUpdate(delta);

            if(currAttack->getAttackState() == ATTACK_FINISHED){
                delete currAttack;
                currAttack = nullptr;
            }
        }

        //attack getters
        bool isAttacking() const {return currAttack != nullptr;}

        AttackState attackingState() const{
            if(currAttack != nullptr) {return currAttack->getAttackState();}
            else {return ATTACK_FINISHED;}
        }

        Rectangle getAttackHitbox() const{
            if(currAttack != nullptr) {return currAttack->getAttackHitbox();}
            else {return Rectangle{0,0,0,0};}
        }

        int getAttackDmg() const {
            if(currAttack != nullptr) {return currAttack->getAttackDmg();}
            else {return 0;}
        }

        int getAttacksNum() const{
            if(currAttack != nullptr) {return currAttack->getNumAttacks();}
            else {return 0;}
        }

        string getAttackName() const {return currAttack->getName();}
        bool canAttackHit() const {return currAttack && currAttack->canResolveHit();}
        void resolveAttackHit() {
            if(currAttack) {currAttack->markHitResolved();} //hit can only be applied once per frame per ATTACK_ACTIVE instead of every frame it is active
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
        bool parriedAll;
        bool hasDodged;
        int selectedAttack;
        int attackOrSkill;
        int AP;

    public:
        Player(int h, int d, Vector3 pos): Fighters(h, d, pos), parriedAll(true), hasDodged(false), selectedAttack(0), attackOrSkill(0), AP(0){
            hitboxRect.x = 650;
            hitboxRect.y = 270;
            debugColor = LIGHTGRAY;
            pt.parryTime = 0.2f;
            pt.dodgeTime = 0.45f;
            pt.cooldownTime = 0.2f;

            //attack pool
            attackList.push_back({"Light Slash", 0.3f, 0.2f, 0.3f, 5, 1, 0}); //name, windup, active, recovery, damage, numAttacks, apCost
            attackList.push_back({"Heavy Slash", 0.6f, 0.25f, 0.5f, 10, 1, 0});
            skillList.push_back({"Multi Strike", 0.4f, 0.15f, 0.35f, 3, 3, 3}); //skill
        }

        void chooseAndStartAttack(){
            if(isAttacking()) {return;} //don't do anything if an attack already exists, exists function

            if(attackOrSkill == 0){

                DrawText(TextFormat("1) ATTACKS"), 950, 610, 20, BLACK);
                DrawText(TextFormat("2) SKILLS"), 950, 630, 20, BLACK);

                if(IsKeyPressed(KEY_ONE)){
                    attackOrSkill = 1;

                    if(IsKeyPressed(KEY_ONE)) {selectedAttack = 0;}
                    if(IsKeyPressed(KEY_TWO)) {selectedAttack = 1;}
                }

                if(IsKeyPressed(KEY_TWO)){
                    attackOrSkill = 2;

                    if(IsKeyPressed(KEY_ONE)) {selectedAttack = 0;}
                }
            }

            //drawing the options
            if(attackOrSkill == 1){
                DrawText(TextFormat("1) Light Attack"), 950, 610, 20, BLACK);
                DrawText(TextFormat("2) Heavy Slash"), 950, 630, 20, BLACK);
            }
            if(attackOrSkill == 2){
                DrawText(TextFormat("1) Multi Strike"), 950, 610, 20, BLACK);
            }

            if(IsKeyPressed(KEY_ENTER)){
                if(attackOrSkill == 1) {startAttack(attackList[selectedAttack]);}
                else if(attackOrSkill == 2){
                    if(AP >= skillList[selectedAttack].apCost){
                        startAttack(skillList[selectedAttack]);
                        AP -= skillList[selectedAttack].apCost;
                    }
                }

                selectedAttack = 0;
                attackOrSkill = 0;
            }
        }

        //timer, parry, dodge and AP setters
        void resetPlayerTimer(){pt.timer = 0;}
        void updatePlayerTimer(float delta) {pt.timer += delta;}
        void setParriedAll(bool pa) {parriedAll = pa;}
        void setHasDodged(bool hd) {hasDodged = hd;}
        void increaseAP() {AP++;}

        //getters 
        float getParryTime() const {return pt.parryTime;}
        float getDodgeTime() const {return pt.dodgeTime;}
        float getCooldownTime() const {return pt.cooldownTime;}
        float getCurrentTime() const {return pt.timer;}
        bool getParriedAll() const {return parriedAll;}
        bool getHasDodged() const {return hasDodged;}
        int getAP() const {return AP;}
};

class Enemy: public Fighters{
    private:
        bool enemyTurnResolved;

    public:
        Enemy(int h, int d, Vector3 pos): Fighters(h, d, pos), enemyTurnResolved(false) {
            debugColor = YELLOW;
            hitboxRect.x = 570;
            hitboxRect.y = 270;
            currAttack = nullptr;

            //attack pool
            attackList.push_back({"Fast Jab" ,0.4f, 0.2f, 0.35f, 5, 1});
            attackList.push_back({"Repeating Thrusts", 0.35f, 0.15f, 0.4f, 3, 3});
            attackList.push_back({"Thrust Attack", 0.6f, 0.2f, 0.35f, 10, 1});
        }

        void chooseAndStartAttack(){
            if(isAttacking()) return;

            cout << "KASHDK" << endl;

            int index = GetRandomValue(0, attackList.size() - 1);
            startAttack(attackList[index]);
        }

        //getters/setters
        void setEnemyTurnResolve(bool etr) {enemyTurnResolved = etr;}
        bool getEnemyTurnResolve() const {return enemyTurnResolved;}
};


void updateEnenmyTurn(float delta, Player& player, Enemy& enemy, PlayerDefenseState& playerDefenseState, GameState& gameState);

int main()
{    
    const int screenW = 1280;
    const int screenH = 720;

    InitWindow(screenW, screenH, "Clair Obscur: Expedition 2D");
    SetTargetFPS(60);

    float delta;
    bool playerStartedAttack = false;

    //fighters
    Player player(50, 5, {5, 1, 0});
    Enemy enemy(50, 5, {-5, 1, 0});
    
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
    //state management
    GameState gameState = PLAYER_TURN;
    PlayerDefenseState playerDefenseState = NONE;

    //put the logic stuff before any of the drawing stuff unless you have to do so otherwise
    //IMPORTANT NOTE: YOU CAN HAVE PLAYER STATES AND ENEMY STATES RUN AT THE SAME TIME, THE WHILE LOOP IS RUNNING EVERY FRAME ANYWAYS, USE IT TO YOUR ADVANTAGE
    while (!WindowShouldClose())
    {
        delta = GetFrameTime();
    
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(cam);
        
        DrawGrid(15, 1);
        player.draw(cubeModel, player.getPos(), BLUE);
        enemy.draw(cubeModel, enemy.getPos(), RED);

        EndMode3D();

        switch(gameState)
        {
            case PLAYER_TURN:
            {
                player.chooseAndStartAttack();

                if(player.isAttacking()){
                    playerStartedAttack = true;
                }

                player.updateAttack(delta);

                if(player.attackingState() == ATTACK_ACTIVE){
                    player.debugColor = GREEN;
                    if(player.canAttackHit() && CheckCollisionRecs(enemy.getHitbox(), player.getAttackHitbox())){
                        player.resolveAttackHit();
                        enemy.takeDamage(player.getAttackDmg());

                        if(enemy.isDead()){
                            gameState = GAME_OVER;
                        }
                    }
                }

                if(playerStartedAttack && !player.isAttacking())
                {
                    player.debugColor = LIGHTGRAY;
                    playerStartedAttack = false;
                    enemy.setEnemyTurnResolve(false);
                    player.setParriedAll(true);
                    player.setHasDodged(false);
                    player.increaseAP();
                    gameState = ENEMY_TURN;
                }
                break;
            }

            case ENEMY_TURN: //add attack class thingy
            {
                updateEnenmyTurn(delta, player, enemy, playerDefenseState, gameState);                

                break;
            }

            case GAME_OVER:
                break;
        }

        //player input states
        if(gameState == ENEMY_TURN)
        {
            switch(playerDefenseState)
            {
                case NONE:
                {
                    if(IsKeyPressed(KEY_P)){
                        player.debugColor = BLUE;
                        playerDefenseState = PARRY;
                    }
                    else if(IsKeyPressed(KEY_D)){
                        player.debugColor = BLUE;
                        playerDefenseState = DODGE;
                    }

                    break;
                }

                case PARRY:
                {
                    if(player.getCurrentTime() < player.getParryTime()){
                        player.updatePlayerTimer(delta);
                    }
                    else if(player.getCurrentTime() >= player.getParryTime()){
                        player.resetPlayerTimer();
                        playerDefenseState = COOLDOWN;
                    }

                    break;
                }

                case DODGE:
                {
                    if(player.getCurrentTime() < player.getDodgeTime()){
                        player.updatePlayerTimer(delta);
                    }
                    else if(player.getCurrentTime() >= player.getDodgeTime()){
                        player.resetPlayerTimer();
                        player.debugColor = LIGHTGRAY;
                        playerDefenseState = COOLDOWN;
                    }

                    break;
                }

                case COOLDOWN:
                {
                    player.debugColor = LIGHTGRAY;

                    if(player.getCurrentTime() < player.getCooldownTime()){
                        player.updatePlayerTimer(delta);
                    }
                    else if(player.getCurrentTime() >= player.getCooldownTime()){
                        player.resetPlayerTimer();
                        playerDefenseState = NONE;
                    }
                    break;
                }
            } 
        }

        player.drawHitbox(player.hitboxRect.x, player.hitboxRect.y, player.hitboxRect.width, player.hitboxRect.height);
        enemy.drawHitbox(enemy.hitboxRect.x, enemy.hitboxRect.y, enemy.hitboxRect.width, enemy.hitboxRect.height);

        DrawText(TextFormat("Player HP: %i", player.getHP()), 1000, 50, 30, BLACK);
        DrawText(TextFormat("AP: %i", player.getAP()), 1000, 100, 30, BLACK);
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

void updateEnenmyTurn(float delta, Player& player, Enemy& enemy, PlayerDefenseState& playerDefenseState, GameState& gameState)
{
    if (!enemy.isAttacking()){
            enemy.chooseAndStartAttack();
    }

    enemy.updateAttack(delta);

    if (enemy.isAttacking()){

        if(enemy.attackingState() == ATTACK_WINDUP){
            DrawText(TextFormat("ENEMY READYING ATTACK"), 500, 650, 30, BLACK);
            enemy.debugColor = ORANGE;
            cout << enemy.getAttackName() << endl;
        }
        else if (enemy.attackingState() == ATTACK_ACTIVE){
            enemy.debugColor = RED;
            if (enemy.canAttackHit() && CheckCollisionRecs(player.getHitbox(), enemy.getAttackHitbox())){

                enemy.resolveAttackHit();

                if(playerDefenseState == PARRY){
                    cout << "PARRY" << endl;
                    player.increaseAP();
                }
                else if(playerDefenseState == DODGE){
                    cout << "DODGE" << endl;
                    player.increaseAP();
                    player.setHasDodged(true);
                }
                else{
                    cout << "HIT" << endl;
                    player.takeDamage(enemy.getAttackDmg());
                    player.setParriedAll(false);
                }
                                
                if(player.isDead()){
                    gameState = GAME_OVER;
                }
            }
        }
    }

    else
    {
        if(enemy.getEnemyTurnResolve()) return;

        if(player.getParriedAll() && !player.getHasDodged()){
            enemy.takeDamage(player.getDmg() * 2);
            cout << "COUNTER ATTACK" << endl;
            enemy.setEnemyTurnResolve(true);
        }

        if(enemy.isDead()){
            gameState = GAME_OVER;
        }
        else{
            playerDefenseState = NONE;
            player.resetPlayerTimer();
            player.setHasDodged(false);
            player.debugColor = LIGHTGRAY;
            enemy.debugColor = YELLOW;
            gameState = PLAYER_TURN;
        }
    }
}