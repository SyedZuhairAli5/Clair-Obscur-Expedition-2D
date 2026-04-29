#include <iostream>
#include "raylib.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;

enum AppState{MAIN_MENU, IN_GAME, EXIT};
//game states: who's turn is it?
enum GameState{PLAYER_TURN, ENEMY_TURN, GAME_OVER};
enum AttackState{ATTACK_WINDUP, ATTACK_ACTIVE, ATTACK_RECOVERY, ATTACK_FINISHED};
enum PlayerDefenseState{NONE, PARRY, DODGE, COOLDOWN};
enum AnimState{ IDLE, RUN, ATTACK1, ATTACK2, ATTACK3, PARRYANIM, DODGEANIM, HURT, DEAD };

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

struct FadeProperties{
    float fadeAlpha;
    float fadeSpeed;
    bool fadeDone;
};

struct Animation{
    Texture2D animTexture;
    int frameWidth;
    int frameHeight;
    int frameCount;
    int framesPerRow;
    int currentFrame;
    float frameTime;  //how long each frame lasts
    float timer;
    bool animLoops;
    bool animFinsihed;
    Rectangle source;
};

struct CharacterAnimations{
    Texture2D idle; 
    Texture2D run; 
    Texture2D walk;
    Texture2D attack1; 
    Texture2D attack2; 
    Texture2D attack3; 
    Texture2D parry; 
    Texture2D dodge; 
    Texture2D hurt; 
    Texture2D dead;
};

vector<AttackData> loadAttacksFromFile(const string& path);
void initAnimation(Animation& anim, Texture2D tex, int fw, int fh, int count, int framesPerRow, float fps, bool loop = true);


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

void updateAnimation(Animation& anim, float delta);
void drawBillboardAnimation(const Camera3D& cam, const Animation& anim, Vector3 worldPos, float scale, bool facingRight, Color tint = WHITE);

class Fighters{
    private:
        Rectangle hitboxRect;
        friend void drawDebugHitbox(const Fighters& f);

    protected:
        int hp;
        int damage;
        bool facingRight;
        float animLockTimer = 0.0f;
        Vector3 position;
        
        Attack* currAttack;
        vector<AttackData> attackList;
        vector<AttackData> skillList;

        AnimState animState;
        Animation currentAnim;

        Animation idleAnim;
        Animation runAnim;
        Animation walkAnim;
        Animation attack1Anim;
        Animation attack2Anim;
        Animation attack3Anim;
        Animation parryAnim;
        Animation dodgeAnim;
        Animation hurtAnim;
        Animation deadAnim;

        Animation& getAnimFromState(){
            switch (animState)
            {
                case IDLE: return idleAnim;
                case RUN: return runAnim;
                case ATTACK1: return attack1Anim;
                case ATTACK2: return attack2Anim;
                case ATTACK3: return attack3Anim;
                case PARRYANIM: return parryAnim;
                case DODGEANIM: return dodgeAnim;
                case HURT: return hurtAnim;
                case DEAD: return deadAnim;
                default: return idleAnim;
            }
        }

        void setHitboxPos(float x, float y){
            hitboxRect.x = x;
            hitboxRect.y = y;
        }

        void setAnimState(AnimState newState){
            if (animState != newState)
            {
                animState = newState;
                getAnimFromState().currentFrame = 0;
                getAnimFromState().timer = 0.0f;
                getAnimFromState().animFinsihed = false;
            }
        }

    public:
        Color debugColor;

        Fighters(int h, int d, Vector3 pos): hp(h), damage(d), position(pos){
            hitboxRect.height = 100;
            hitboxRect.width = 100;
            currAttack = nullptr;
            animState = IDLE;
        }

        virtual ~Fighters(){
            if(currAttack){
                delete currAttack;
            }
        }

        //choose function which will be defined in player and enemy differently
        virtual void chooseAndStartAttack() = 0;

        void updateFighterAnimation(float delta){
            if(animLockTimer > 0.0f){
                animLockTimer -= delta;
                updateAnimation(getAnimFromState(), delta);
                return;
            }

            updateAnimation(getAnimFromState(), delta);
        }

        void drawBillboard(const Camera3D& cam){
            drawBillboardAnimation(cam, getAnimFromState(), position, 0.035f,facingRight, WHITE);
        }

        void playIdle() {
            setAnimState(IDLE);
        }

        void playAttackAnim(int index) {
            if(index == 0) setAnimState(ATTACK1);
            else if(index == 1) setAnimState(ATTACK2);
            else setAnimState(ATTACK3);
        }

        void playParry() {
            animLockTimer = 0.35f;
            setAnimState(PARRYANIM);
        }

        void playDodge() {
            animLockTimer = 0.45f;
            setAnimState(DODGEANIM);
        }

        void playHurt(float lockTime = 0.35f) {
            animLockTimer = lockTime;
            setAnimState(HURT);
            getAnimFromState().animFinsihed = false;
        }

        void takeDamage(int dmg){
            hp -= dmg;
            playHurt();

            if(hp <= 0 ){
                hp = 0;
                setAnimState(DEAD);
            }
        }

        void draw(Model m, Vector3 pos, Color c){
            DrawModel(m, {pos.x, pos.y, pos.z}, 2, c);
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

//debug hitbox drawing
void drawDebugHitbox(const Fighters& f){
    DrawRectangle(f.hitboxRect.x, f.hitboxRect.y, f.hitboxRect.width, f.hitboxRect.height, f.debugColor);
}

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
            setHitboxPos(650, 270);
            debugColor = LIGHTGRAY;
            pt.parryTime = 0.3f;
            pt.dodgeTime = 0.5f;
            pt.cooldownTime = 0.2f;
            facingRight = false;

            //attack pool
            attackList = loadAttacksFromFile("player_attacks.txt");

            for (auto it = attackList.begin(); it != attackList.end(); ){
                if (it->apCost > 0) {
                    skillList.push_back(*it);
                    it = attackList.erase(it);
                } else {++it;}
            }
        }

        void initAnimations(const CharacterAnimations& tex){
            initAnimation(idleAnim,    tex.idle,    128, 128, 8, 8, 10, true);
            initAnimation(attack1Anim, tex.attack1, 128, 128, 6, 6, 14, false);
            initAnimation(attack2Anim, tex.attack2, 128, 128, 6, 6, 14, false);
            initAnimation(attack3Anim, tex.attack3, 128, 128, 6, 6, 14, false);
            initAnimation(parryAnim,   tex.parry,   128, 128, 4, 4, 10, false);
            initAnimation(dodgeAnim,   tex.dodge,   128, 128, 6, 6, 12, false);
            initAnimation(hurtAnim, tex.hurt, 128, 128, 3, 3, 12, false);
            initAnimation(deadAnim, tex.dead, 128, 128, 3, 3, 6, false);

            animState = IDLE;
        }

        void chooseAndStartAttack(){
            if(isAttacking()) return; //don't do anything if an attack already exists, exists function

            if (attackOrSkill == 0){
                DrawText("1) ATTACKS", 950, 610, 30, BLACK);
                DrawText("2) SKILLS", 950, 640, 30, BLACK);

                if (IsKeyPressed(KEY_ONE)) attackOrSkill = 1;
                if (IsKeyPressed(KEY_TWO)) attackOrSkill = 2;
            }
            else{
                if (IsKeyPressed(KEY_ONE)) selectedAttack = 0;
                if (IsKeyPressed(KEY_TWO)) selectedAttack = 1;
                if (IsKeyPressed(KEY_THREE)) selectedAttack = 2;
            }

            //drawing the options
            if(attackOrSkill == 1){
                for(int i=0; i<attackList.size(); i++){
                    DrawText(TextFormat("%i) %s", i+1, attackList[i].attackName.c_str()), 950, 610 + i * 25, 30, BLACK);
                }
            }
            if(attackOrSkill == 2){
                for(int i=0; i<skillList.size(); i++){
                    DrawText(TextFormat("%i) %s (AP: %i)", i+1, skillList[i].attackName.c_str(), skillList[i].apCost), 950, 610 + i * 25, 30, BLACK);
                }

            }

            if (IsKeyPressed(KEY_ENTER)){
                if (attackOrSkill == 1){
                    if (selectedAttack >= 0 && selectedAttack < attackList.size()){
                        startAttack(attackList[selectedAttack]);
                        playAttackAnim(selectedAttack);
                    }
                }

                else if (attackOrSkill == 2){
                    if (selectedAttack >= 0 && selectedAttack < skillList.size()){
                        if (AP >= skillList[selectedAttack].apCost){
                            startAttack(skillList[selectedAttack]);
                            AP -= skillList[selectedAttack].apCost;
                            playAttackAnim(2);
                        }
                    }
                }

                selectedAttack = 0;
                attackOrSkill = 0;
            }
        }

        void updateAnimationState(){
            if(animLockTimer > 0.0f) return;

            if (isDead()){
                setAnimState(DEAD);
                return;
            }

            if (isAttacking()){
                switch (attackingState())
                {
                    case ATTACK_WINDUP:
                    case ATTACK_ACTIVE:
                        // animation already chosen when attack started
                        break;

                    case ATTACK_FINISHED:
                        playIdle();
                        break;
                }
                return;
            }

            playIdle();
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
        int attackIndex;

    public:
        Enemy(int h, int d, Vector3 pos): Fighters(h, d, pos), enemyTurnResolved(false) {
            debugColor = YELLOW;
            setHitboxPos(570, 270);
            currAttack = nullptr;
            facingRight = true;

            //attack pool
            attackList.push_back({"Fast Jab" ,0.4f, 0.2f, 0.35f, 5, 1});
            attackList.push_back({"Repeating Thrusts", 0.35f, 0.15f, 0.4f, 3, 3});
            attackList.push_back({"Thrust Attack", 0.6f, 0.2f, 0.35f, 10, 1});
        }

         void initAnimations(const CharacterAnimations& tex){
            initAnimation(idleAnim, tex.idle, 128, 128, 8, 8, 10, true);
            initAnimation(attack1Anim, tex.attack1, 128, 128, 6, 6, 14, false);
            initAnimation(attack2Anim, tex.attack2, 128, 128, 6, 6, 14, false);
            initAnimation(attack3Anim, tex.attack3, 128, 128, 6, 6, 14, false);
            initAnimation(hurtAnim, tex.hurt, 128, 128, 2, 2, 12, false);
            initAnimation(deadAnim, tex.dead, 128, 128, 3, 3, 6, false);
            animState = IDLE;
        }

        void chooseAndStartAttack(){
            if(isAttacking()) return;

            attackIndex = GetRandomValue(0, attackList.size() - 1);
            startAttack(attackList[attackIndex]);
        }

        void updateAnimationState(){
            if(animLockTimer > 0.0f) return;

            if (isDead()){
                setAnimState(DEAD);
                return;
            }

            if (isAttacking()){
                if (attackingState() == ATTACK_WINDUP || attackingState() == ATTACK_ACTIVE){
                    //setAnimState(ATTACK1); // or randomize later
                    if(attackIndex == 0) setAnimState(ATTACK1);
                    else if(attackIndex == 1) setAnimState(ATTACK2);
                    else if(attackIndex == 2) setAnimState(ATTACK3);
                    
                }
                else if (attackingState() == ATTACK_FINISHED){
                    playIdle();
                }
                return;
            }

            playIdle();
        }

        //getters/setters
        void setEnemyTurnResolve(bool etr) {enemyTurnResolved = etr;}
        bool getEnemyTurnResolve() const {return enemyTurnResolved;}
};


void updateEnenmyTurn(float delta, Player& player, Enemy& enemy, PlayerDefenseState& playerDefenseState, GameState& gameState);

vector<AttackData> loadAttacksFromFile(const string& path){
    vector<AttackData> attacks;
    ifstream file(path);

    if (!file.is_open()) {
        cout << "Failed to open: " << path << endl;
        return attacks;
    }

    string line;

    while (getline(file, line)){
        stringstream ss(line);
        AttackData a;
        string temp;

        getline(ss, a.attackName, ',');
        getline(ss, temp, ','); a.attackWindup = stof(temp);
        getline(ss, temp, ','); a.attackActive = stof(temp);
        getline(ss, temp, ','); a.attackRecovery = stof(temp);
        getline(ss, temp, ','); a.damage = stoi(temp);
        getline(ss, temp, ','); a.numOfAttacks = stoi(temp);
        getline(ss, temp, ','); a.apCost = stoi(temp);
        attacks.push_back(a);
    }

    file.close();
    return attacks;
}

void fadeTransition(FadeProperties& fade , bool inOrOut, float delta){
    if(!fade.fadeDone){
        if(inOrOut){
            fade.fadeAlpha -= delta * fade.fadeSpeed;

            if(fade.fadeAlpha <= 0.0f){
                fade.fadeAlpha = 0.0f;
                fade.fadeDone = true;
            }
        }
        else if(!inOrOut){
            fade.fadeAlpha += delta * fade.fadeSpeed;

            if(fade.fadeAlpha >= 1.0f){
                fade.fadeAlpha = 1.0f;
                fade.fadeDone = true;
            }
        }
    }
}

void pulsatingEffect(float& alpha ,float delta){
    static bool pulse = false;
    const float pulseRate = 0.625f;

    if(pulse == false && alpha < 1.0f){
        alpha += pulseRate * delta;
    }
    else if(alpha >=1.0f){
        alpha = 1.0f;
        pulse = true;
    }
    
    if(pulse == true && alpha > 0.0f){
        alpha -= pulseRate * delta;
    }
    else if(alpha <= 0.0f){
        alpha = 0.0f;
        pulse = false;
    }
}

void initAnimation(Animation& anim, Texture2D tex, int fw, int fh, int count, int framesPerRow, float fps, bool loop){
    anim.animTexture = tex;
    anim.frameWidth = fw;
    anim.frameHeight = fh;
    anim.frameCount = count;
    anim.framesPerRow = framesPerRow;
    anim.currentFrame = 0;
    anim.frameTime = 1.0f / fps;
    anim.timer = 0.0f;
    anim.animLoops = loop;
    anim.animFinsihed = false;

    anim.source = {0, 0, (float)fw, (float)fh};
}

void updateAnimation(Animation& anim, float delta){
    if(anim.animFinsihed) {return;}

    anim.timer += delta;

    if(anim.timer >= anim.frameTime){
        anim.timer = 0.0f;
        anim.currentFrame++;

        if(anim.currentFrame >= anim.frameCount){
            if(anim.animLoops) {anim.currentFrame = 0;}
            else {
                anim.currentFrame = anim.frameCount - 1;
                anim.animFinsihed = true;
            }
        }

        int col = anim.currentFrame % anim.framesPerRow;
        int row = anim.currentFrame / anim.framesPerRow;

        anim.source.x = col * anim.frameWidth;
        anim.source.y = row * anim.frameHeight;
    }
}

void drawBillboardAnimation(const Camera3D& cam, const Animation& anim, Vector3 worldPos, float scale, bool facingRight, Color tint){
    Rectangle source = anim.source;

      if (!facingRight) {
        source.width = -source.width;
        source.x += anim.frameWidth;
    }

    Vector2 size = {anim.frameWidth * scale, anim.frameHeight * scale};
    Vector2 origin = { size.x / 2, size.y / 2 };

    DrawBillboardPro(cam, anim.animTexture, source, worldPos, {0, 1, 0}, size, origin, 0.0f, tint);
}

void drawAnimation(const Animation& anim, Vector2 pos, float scale, Color tint = WHITE){
    Rectangle dest = {pos.x, pos.y, anim.frameWidth * scale, anim.frameHeight * scale};
    DrawTexturePro(anim.animTexture, anim.source, dest, {0, 0}, 0.0f, tint);
}

int main()
{    
    const int screenW = 1280;
    const int screenH = 720;

    InitWindow(screenW, screenH, "Clair Obscur: Expedition 2D");
    SetTargetFPS(60);

    float delta;
    bool playerStartedAttack = false;

    //assets
    Texture2D e2dLogo = LoadTexture("assets/e2d logo.png");
    Texture2D backdrop = LoadTexture("assets/main menu backdrop.png");
    Font menuFont = LoadFont("assets/Cinzel-VariableFont_wght.ttf");
    Texture2D petals = LoadTexture("assets/petal_anim_spritesheet.png");
    Model scenery = LoadModel("assets/scenery.glb");

    //player and enemy animations
    CharacterAnimations playerTex;
    playerTex.idle = LoadTexture("assets/Fighter/Idle.png");
    playerTex.attack1= LoadTexture("assets/Fighter/Attack_1.png");
    playerTex.attack2= LoadTexture("assets/Fighter/Attack_2.png");
    playerTex.attack3= LoadTexture("assets/Fighter/Attack_3.png");
    playerTex.parry = LoadTexture("assets/Fighter/Shield.png");
    playerTex.dodge = LoadTexture("assets/Fighter/Jump.png");
    playerTex.hurt  = LoadTexture("assets/Fighter/Hurt.png");
    playerTex.dead  = LoadTexture("assets/Fighter/Dead.png");

    CharacterAnimations enemyTex;
    enemyTex.idle = LoadTexture("assets/Samurai/Idle.png");
    enemyTex.attack1 = LoadTexture("assets/Samurai/Attack_1.png");
    enemyTex.attack2 = LoadTexture("assets/Samurai/Attack_2.png");
    enemyTex.attack3 = LoadTexture("assets/Samurai/Attack_3.png");
    enemyTex.hurt = LoadTexture("assets/Samurai/Hurt.png");
    enemyTex.dead = LoadTexture("assets/Samurai/Dead.png");

    Animation petalFlowAnim;
    initAnimation(petalFlowAnim, petals, 512, 288, 64, 8, 18, true);

    //text centering and other text related things
    const char* continueText = "Press E to continue";
    int fontSize = 60;
    float fontSpacing = 2.0f;
    Vector2 textSize = MeasureTextEx(menuFont, continueText, fontSize, fontSpacing);
    Vector2 textPosition = {(screenW - textSize.x)/2, 550};
    SetTextureFilter(menuFont.texture, TEXTURE_FILTER_POINT);

    //screen fade logic and variables
    FadeProperties fadeOut = {1.0f, 0.25f, false};
    FadeProperties fadeIn = {0.0f, 0.75f, false};
    FadeProperties logoFadeIn = {0.0f, 0.5f, false};
    float timeToLogo = 0.7f;
    bool startTransToGame = false;
    float textAlpha = 0.0f;

    //fighters
    Player player(50, 5, {1.5, 2.35f, 0});
    Enemy enemy(50, 5, {-1.5, 2.35f, 0});

    player.initAnimations(playerTex);
    enemy.initAnimations(enemyTex);
    
    //camera
    Camera3D cam = Camera3D();
    cam.position = {0, 3.5f, 20.0f};
    cam.target = {0, 0, 0};
    cam.up = {0 , 1, 0};
    cam.fovy = 70;
    cam.projection = CAMERA_PERSPECTIVE;

    //models
    Mesh cubeMesh = GenMeshCube(1, 1, 1);
    Model cubeModel = LoadModelFromMesh(cubeMesh);

    //state management
    GameState gameState = PLAYER_TURN;
    PlayerDefenseState playerDefenseState = NONE;
    AppState appState = IN_GAME;

    //put the logic stuff before any of the drawing stuff unless you have to do so otherwise
    //IMPORTANT NOTE: YOU CAN HAVE PLAYER STATES AND ENEMY STATES RUN AT THE SAME TIME, THE WHILE LOOP IS RUNNING EVERY FRAME ANYWAYS, USE IT TO YOUR ADVANTAGE
    while (!WindowShouldClose())
    {
        delta = GetFrameTime();
    
        BeginDrawing();
        ClearBackground(RAYWHITE);

        switch(appState)
        {
            case MAIN_MENU:
            {
                DrawTexture(backdrop, 0, 0, WHITE);

                updateAnimation(petalFlowAnim, delta);
                drawAnimation(petalFlowAnim, {0, 0}, 2.5f, {255, 255, 255, 100});
                fadeTransition(fadeOut, true, delta);
                
                if(timeToLogo >= 0.0f){
                    timeToLogo -= delta;
                }
                else if(timeToLogo <= 0.0f){
                    fadeTransition(logoFadeIn, false, delta);
                }
                
                if(IsKeyPressed(KEY_E) && fadeOut.fadeDone && !startTransToGame ){
                    startTransToGame = true;
                }

                if(startTransToGame == true){
                    fadeTransition(fadeIn, false, delta);
                    if(fadeIn.fadeDone){
                        appState = IN_GAME;
                        fadeOut = {1.0f, 0.75f, false};
                    }
                }

                pulsatingEffect(textAlpha, delta);

                DrawTexture(e2dLogo, ((screenW - e2dLogo.width)/2), 150, Fade(WHITE, logoFadeIn.fadeAlpha));
                DrawTextEx(menuFont, continueText, textPosition, fontSize, fontSpacing, Fade(WHITE, textAlpha));

                if(!startTransToGame){
                    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, fadeOut.fadeAlpha));
                }
                else{
                    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, fadeIn.fadeAlpha));
                }
                

                break;
            }

            case IN_GAME:
            {
                BeginMode3D(cam);

                player.updateFighterAnimation(delta);
                enemy.updateFighterAnimation(delta);
                player.updateAnimationState();
                enemy.updateAnimationState();

                player.drawBillboard(cam);
                enemy.drawBillboard(cam);

                DrawModelEx(scenery, {-5.5f, 0, 0}, {0, 1, 0}, 90.0f, {1.0f, 1.0f, 1.0f}, WHITE);

                EndMode3D();

                if(cam.position.z >=12.0f)
                cam.position.z -= 3.5 * delta;

                fadeTransition(fadeOut, true, delta);

                if(fadeOut.fadeDone){
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
                                if(IsKeyPressed(KEY_R)){
                                    player.debugColor = BLUE;
                                    playerDefenseState = PARRY;
                                    player.playParry();
                                }
                                else if(IsKeyPressed(KEY_Q)){
                                    player.debugColor = BLUE;
                                    playerDefenseState = DODGE;
                                    player.playDodge();
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
                }

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

                DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, fadeOut.fadeAlpha));

                break;
            }

            case EXIT:
            {
                break;
            }
        }

        EndDrawing();
    }

    UnloadModel(scenery);

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
            DrawText(TextFormat("ENEMY READYING ATTACK: %s", enemy.getAttackName().c_str()), 500, 650, 30, BLACK);
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