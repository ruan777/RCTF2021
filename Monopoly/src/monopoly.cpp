#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <string>
#include <cstdlib>

using namespace std;


// 地皮类型
#define GO              0
#define LAND            1
#define FREE_PARKING    2
#define CHANCE          3

#define EASE_LEVEL      1
#define MEDIUM_LEVEL    2
#define HARD_LEVEL      3


class Player{
public:
    string name;
    uint8_t property_idx[0x40];
    uint32_t property_count;
    int money;
    uint32_t location;
    Player();
    void init(uint32_t player_type);
    void reset();
    void show_property();
};

struct Random{
    uint8_t randBytes[0x1008];
    uint32_t idx;
    uint32_t size;
};

class Property{
public:
    string name;            // 地名
    string owner;
    Player* owner_ptr;

    uint32_t count;         // 玩家购买次数
    uint32_t price[5];      // 其他玩家到达该地皮时给owner的钱
    uint32_t worth;         // 买下地皮的价格
    uint32_t toll_road;     // 过路费
    uint32_t mortgage;      // 抵押费
    uint32_t type;

    Property();
    Property(const char* n,int p,int worth_,int toll,int type_);
    void reset();
    void show();
};

Random r;
string nobody;
Property* game_map[64];

Player player;
Player ai;
uint32_t level;
uint32_t use_srand = 0;


void error_exit(const char* msg){
    puts(msg);
    exit(-1);
}

void init_random(){
    int fd = open("/dev/urandom",O_RDONLY);
    if(fd < 0){
        error_exit("open urandom failed! what happen??");
    }
    r.size = 0x1000;
    r.idx = 0;
    read(fd,r.randBytes,r.size);
    close(fd);
}


Property::Property(){}

Property::Property(const char* n,int p,int worth_,int toll,int type_){
    if(type_ == LAND){
        // 其他玩家过路时该给owner的价钱
        for(int i = 0;i < 5;i++){
            this->price[i] = (p/2)*(i+1);
        }
        this->owner = nobody;
        this->owner_ptr = nullptr;
        this->count = -1;
        this->worth = worth_;
        this->toll_road = toll;
        this->mortgage = worth_ / 2;
    }
    this->type = type_;
    this->name = string(n);
}

void Property::reset(){
    this->owner = nobody;
    this->count = -1;
    this->owner_ptr = nullptr;
}

void Property::show(){
    printf("owner: %s\n",this->owner.c_str());
    printf("worth: %d\n",this->worth);
    if(this->owner == nobody){
        printf("toll_road: %d\n",this->toll_road);
    }else{
        printf("toll_road: %d\n",this->price[this->count]);
    }
}

Player::Player(){}

void Player::init(uint32_t player_type = 0){
    if(player_type == 0){
        puts("what's your name?");
        cin >> this->name;
    }else{
        this->name = string("AI");
    }
    
    memset(this->property_idx,0,sizeof(this->property_idx));
    this->property_count = 0;
    this->money = 600000;
    this->location = 0;
}

void Player::reset(){
    memset(this->property_idx,0,sizeof(this->property_idx));
    this->property_count = 0;
    this->location = 0;
}

void Player::show_property(){
    for(int i = 0;i < this->property_count;i++){
        printf("%d : %s\n",i,game_map[this->property_idx[i]]->name.c_str());
    }
}

void init_map(){
    nobody = string("nobody");

    // 起点
    game_map[0] = new Property("Arbington",0,0,0,GO);          
    // 免费经过地
    game_map[16] = new Property("Bredwardine",0,0,0,FREE_PARKING);     
    game_map[32] = new Property("Dangarnon",0,0,0,FREE_PARKING); 
    game_map[48] = new Property("Hwen",0,0,0,FREE_PARKING); 
    game_map[11] = new Property("Fallkirk",0,0,0,FREE_PARKING); 
    game_map[19] = new Property("Coombe",0,0,0,FREE_PARKING); 
    game_map[26] = new Property("Kameeraska",0,0,0,FREE_PARKING); 
    game_map[37] = new Property("Blackburn",0,0,0,FREE_PARKING); 
    game_map[56] = new Property("Farnfoss",0,0,0,FREE_PARKING); 
    // 机会
    game_map[3] =  new Property("Tylwaerdreath",0,0,0,CHANCE);     
    game_map[22] = new Property("Lhanbryde",0,0,0,CHANCE); 
    game_map[40] = new Property("Holmfirth",0,0,0,CHANCE); 
    game_map[51] = new Property("Blaenau",0,0,0,CHANCE); 
    // 地皮
    game_map[1] =  new Property("Emelle",0x1000,0x2200,0x600,LAND); 
    game_map[2] =  new Property("Bracklewhyte",0x2000,0x4100,0x900,LAND); 
    game_map[4] =  new Property("Aethelney",0x2000,0x4200,0x800,LAND); 
    game_map[5] =  new Property("Warthford",0x3000,0x5700,0x1400,LAND);
    game_map[6] =  new Property("Tywardreath",0x2000,0x3900,0x900,LAND); 
    game_map[7] =  new Property("Frostford",0x3000,0x5800,0x1500,LAND); 
    game_map[8] =  new Property("Stanmore",0x4000,0x7000,0x1900,LAND); 
    game_map[9] =  new Property("Caerleon",0x2000,0x4000,0x900,LAND); 
    game_map[10] = new Property("Wimborne",0x3000,0x5700,0x1400,LAND); 
    game_map[12] = new Property("Arkaley",0x2000,0x4200,0x850,LAND); 
    game_map[13] = new Property("Stamford",0x4000,0x7100,0x1900,LAND); 
    game_map[14] = new Property("Lanercoast",0x4000,0x7200,0x2000,LAND); 
    game_map[15] = new Property("Erast",0x2000,0x4100,0x800,LAND); 
    game_map[17] = new Property("Airedale",0x3000,0x5700,0x1400,LAND); 
    game_map[18] = new Property("Wallowdale",0x2000,0x4100,0x900,LAND);  
    game_map[19] = new Property("Limesvilles",0x3000,0x5700,0x1300,LAND); 
    game_map[20] = new Property("Greenflower",0x2000,0x4200,0x800,LAND); 
    game_map[21] = new Property("Landow",0x1000,0x2200,0x600,LAND); 
    game_map[23] = new Property("Falkirk",0x1000,0x2300,0x600,LAND); 
    game_map[24] = new Property("Rotherham",0x1000,0x2200,0x600,LAND); 
    game_map[25] = new Property("Windrip",0x3000,0x5800,0x1300,LAND); 
    game_map[27] = new Property("Ilragorn",0x2000,0x4100,0x900,LAND); 
    game_map[28] = new Property("Worcester",0x4000,0x7100,0x1900,LAND); 
    game_map[29] = new Property("Drumnacanvy",0x1000,0x2200,0x600,LAND); 
    game_map[30] = new Property("Mirefield",0x1000,0x2100,0x600,LAND); 
    game_map[31] = new Property("Langdale",0x6000,0xb000,0x3000,LAND); 
    game_map[33] = new Property("Hadleigh",0x3000,0x5600,0x1500,LAND); 
    game_map[34] = new Property("Astrakane",0x1000,0x2200,0x600,LAND); 
    game_map[35] = new Property("Aempleforth",0x6000,0xb000,0x3200,LAND); 
    game_map[36] = new Property("Braedwardith",0x3000,0x5600,0x1400,LAND); 
    game_map[37] = new Property("Lerwick",0x1000,0x2000,0x600,LAND); 
    game_map[38] = new Property("Rutherglen",0x1000,0x2000,0x600,LAND); 
    game_map[39] = new Property("Northpass",0x5000,0x8900,0x2d00,LAND); 
    game_map[41] = new Property("Sarton",0x5000,0x8a00,0x2c00,LAND); 
    game_map[42] = new Property("Helmfirth",0x2000,0x4200,0x800,LAND); 
    game_map[43] = new Property("Moressley",0x5000,0x8900,0x2d00,LAND); 
    game_map[44] = new Property("Halivaara",0x4000,0x7300,0x1b00,LAND); 
    game_map[45] = new Property("Burnsley",0x2000,0x4000,0x800,LAND); 
    game_map[46] = new Property("Farncombe",0x1000,0x2200,0x600,LAND); 
    game_map[47] = new Property("Berxley",0x5000,0x8900,0x2c00,LAND); 
    game_map[49] = new Property("Timeston",0x1000,0x2100,0x600,LAND); 
    game_map[50] = new Property("Eastcliff",0x5000,0x8c00,0x2d00,LAND); 
    game_map[52] = new Property("Claethorpes",0x3000,0x5700,0x1400,LAND); 
    game_map[53] = new Property("Penzance",0x5000,0x8a00,0x2d00,LAND); 
    game_map[54] = new Property("Dalmellington",0x5000,0x8900,0x2d00,LAND); 
    game_map[55] = new Property("Ramshorn",0x1000,0x2200,0x600,LAND); 
    game_map[57] = new Property("Barnemouth",0x5000,0x8900,0x2c00,LAND); 
    game_map[58] = new Property("Sherfield",0x1000,0x2200,0x600,LAND); 
    game_map[59] = new Property("Lakeshore",0x3000,0x5800,0x1400,LAND); 
    game_map[60] = new Property("Tottenham",0x1000,0x2100,0x600,LAND); 
    game_map[61] = new Property("Easthaven",0x1000,0x2300,0x600,LAND); 
    game_map[62] = new Property("Nancledra",0x3000,0x5600,0x1600,LAND); 
    game_map[63] = new Property("Deathfall",0x4000,0x7000,0x1900,LAND); 
}


void handler(int sig){
	puts("time out!");
	_exit(2);
}



void init(void){
    setvbuf(stdin,NULL,_IONBF,0);
    setvbuf(stdout,NULL,_IONBF,0);
    setvbuf(stderr,NULL,_IONBF,0);
    signal(SIGALRM, handler);
	alarm(0x100);
}

void banner(){
    puts(
    "  _____   _____ _______ ______   ___   ___ ___  __ \n"
    " |  __ \\ / ____|__   __|  ____| |__ \\ / _ \\__ \\/_ |\n"
    " | |__) | |       | |  | |__       ) | | | | ) || |\n"
    " |  _  /| |       | |  |  __|     / /| | | |/ / | |\n"
    " | | \\ \\| |____   | |  | |       / /_| |_| / /_ | |\n"
    " |_|  \\_\\\\_____|  |_|  |_|      |____|\\___/____||_|\n"
    );
    puts("Welcome to play the game!");
    puts("You need to play the game to become multimillionaire in hard level.");
    puts("Here is your money: $20w");
}

void get_flag(){
    int fd = open("/flag",O_RDONLY);
    char buf[0x100] = {0};
    int file_length = read(fd,buf,0x100);
    if(file_length < 0){
        printf("Ops! please contact admin!");
        exit(-1);
    }
    write(1,buf,file_length);
    puts("");
    return;
}

uint32_t read_n(char* buf,uint32_t len){
    int i;
    for(i = 0;i < len;i++){
        read(0,buf+i,1);
        if(buf[i] == '\n'){
            break;
        }
    }
    return i;
}

uint32_t get_int(){
    char buf[0x10] = {0};
    read_n(buf,0xf);
    return atoi(buf);
}


uint8_t randU8(){
    uint8_t res;

    if(use_srand == 1){
        res = rand() & 0xff;
        return res;
    }

    if(r.idx + 1 > r.size){
        init_random(); 
    }
    res = *(uint8_t*)(&r.randBytes[r.idx]);
    r.idx += 4;
    return res;
}


void game_menu(){
    puts("1. easy level");
    puts("2. medium level");
    puts("3. hard level!!!!");
    puts("4. I don't want play");
    printf("input your choice>>");
}

void land_menu(){
    puts("1. sale your property");
    puts("2. pass (you need pay toll road)");
    puts("3. buy this property");
    puts("4. I don't want play");
    printf("input your choice>>");
}

void sale_property(uint32_t idx){
    player.money += game_map[idx]->mortgage;        // 不值钱
    game_map[idx]->reset();                         // 重新变成nobody所属
    for(int i = idx;i < player.property_count - 1;i++){ // 向前移动一格
        player.property_idx[i] = player.property_idx[i+1];
    }
    if(player.property_count > 0)
        player.property_count--;
}

// 返回值：0表示正常，1表示有误，2表示破产
uint32_t land_op(uint32_t choice){
    uint32_t property_idx = 0;
    uint32_t price = 0;

    switch(choice)
    {
        case 1:
            player.show_property();
            puts("property idx>>");
            property_idx = get_int();
            if(property_idx >= player.property_count){
                puts("invalid idx!");
                return 1;
            }
            sale_property(player.property_idx[property_idx]);
            break;
        case 2:                
            if(game_map[player.location]->owner != nobody && game_map[player.location]->owner != player.name){
                price = game_map[player.location]->price[game_map[player.location]->count];
                player.money -= price;
                game_map[player.location]->owner_ptr->money += price;
            }else{
                player.money -= game_map[player.location]->toll_road;
            }
            if(player.money <= 0){
                puts("you lose all money!");
                return 2;
            }
            break;
        case 3:
            if(game_map[player.location]->owner == nobody){
                player.money -= game_map[player.location]->worth;
                player.property_idx[player.property_count] = player.location;
                player.property_count++;
                game_map[player.location]->owner = player.name;
                game_map[player.location]->owner_ptr = &player;
                game_map[player.location]->count++;
            }else if(game_map[player.location]->owner == player.name){
                player.money -= game_map[player.location]->worth;
                game_map[player.location]->count++;
            }else{
                puts("this property already saled!");
                return 1;
            }
            break;
        default:
            puts("invalid idx");
            return 1;
    }
    return 0;
}

void chance(uint32_t player_type){
    // 1 代表玩家， 2代表ai
    uint8_t rand_num = randU8();
    puts("chance! win or lose?");
    Player* user = player_type == 1? (&player) : (&ai);
    if(rand_num < 10){
        puts("you lose half of your money!");
        user->money /=  2;
    }else if(rand_num < 50){
        puts("you lose 30000");
        user->money -= 30000;
    }else if(rand_num < 100){
        puts("you lose 20000");
        user->money -= 20000;
    }else if(rand_num < 160){
        puts("you lose 10000");
        user->money -= 10000;
    }else if(rand_num < 190){
        puts("you win 10000");
        user->money += 10000;
    }else if(rand_num < 210){
        puts("you win 20000");
        user->money += 20000;
    }else if(rand_num < 240){
        puts("you win 30000");
        user->money += 30000;
    }else{
        if(level == HARD_LEVEL){
            puts("wow! double your money!");
            user->money *= 2;
        }
    }
}

// 返回值：0表示正常，1表示输了或者不想玩了
uint32_t player_round(){
    uint32_t step = (randU8() % 12) + 1;
    uint32_t property_type;
    uint32_t choice;
    uint32_t res = 0;


    player.location += step;
    player.location %= 64;
    property_type = game_map[player.location]->type;

    printf("your money: %d\n",player.money);
    printf("%s throw %d, now location: %d, %s\n",player.name.c_str(),step,player.location,game_map[player.location]->name.c_str());


    switch (property_type)
    {
        case LAND:
            game_map[player.location]->show();
            while(1){
                land_menu();
                choice = get_int();
                if(choice == 4){        // 不玩了
                    return 1;
                }
                res = land_op(choice);
                if(res == 2){
                    // 输了
                    return 1;
                }else if(res == 0){
                    // 操作正常返回
                    break;
                }
            }
            break;
        case FREE_PARKING:
            puts("This is a free parking!");
            // nothing to do
            break;
        case CHANCE:
            chance(1);
            break;
        default:
            break;
    }
    return 0;
}

uint32_t ai_round(){
    uint32_t step = (randU8() % 12) + 1;
    uint32_t property_type;
    uint32_t choice;
    uint32_t price = 0;
    uint32_t res = 0;

    ai.location += step;
    ai.location %= 64;
    property_type = game_map[ai.location]->type;

    printf("ai money: %d\n",ai.money);
    printf("%s throw %d, now location: %d, %s\n",ai.name.c_str(),step,ai.location,game_map[ai.location]->name.c_str());

    switch (property_type)
    {
        case LAND:
            game_map[ai.location]->show();
            // ai就只会买和过
            if(game_map[ai.location]->owner == nobody){
                // 买
                ai.money -= game_map[ai.location]->worth;
                ai.property_idx[ai.property_count] = ai.location;
                ai.property_count++;
                game_map[ai.location]->owner = ai.name;
                game_map[ai.location]->owner_ptr = &ai;
                game_map[ai.location]->count++;
            }else if(game_map[ai.location]->owner == ai.name){
                ai.money -= game_map[ai.location]->worth;
                game_map[ai.location]->count++;
            }else{
                // 过
                price = game_map[ai.location]->price[game_map[ai.location]->count];
                ai.money -= price;
                game_map[ai.location]->owner_ptr->money += price;
            }
            break;
        case FREE_PARKING:
            puts("This is a free parking!");
            // nothing to do
            break;
        case CHANCE:
            chance(2);
            break;
        default:
            break;
    }
    return 0;
}

void easy_level(){
    
    uint32_t res;

    level = EASE_LEVEL;
    ai.money += 1000000;

    puts("you choice easy level, the game begins now!");


    while(1){
        res = player_round();
        if(res == 1){       // 输了或者不玩了
            puts("ok!");
            return;
        }
        ai_round();
        if((player.money > 0 && ai.money < 0) || 
            (player.money > 5000000)){
                puts("you win!");
                return;
        }
        if(ai.money > 5000000){
            puts("you lose");
            return;
        }
    }
}

void medium_level(){
    uint32_t res;

    level = MEDIUM_LEVEL;
    ai.money += 2000000;

    puts("you choice medium level, the game begins now!");


    while(1){
        res = player_round();
        if(res == 1){       // 输了或者不玩了
            puts("ok!");
            return;
        }
        ai_round();
        if((player.money > 0 && ai.money < 0) || 
            (player.money > 5000000)){
                puts("you win!");
                return;
        }
        if(ai.money > 10000000){
            puts("you lose");
            return;
        }
    }
}

void hard_level(){
    uint32_t res;
    uint32_t seed;

    level = HARD_LEVEL;
    ai.money = 100000000;

    puts("you choice hard level, you can choice a seed to help you win the game!");
    printf("seed>>");
    seed = get_int();
    srand(seed);
    use_srand = 1;
    

    while(1){
        res = player_round();
        if(res == 1){       // 输了或者不玩了
            puts("ok!");
            return;
        }
        ai_round();
        if((player.money > 0 && ai.money < 0) || 
            (player.money > 10000000)){
                puts("you win! here is your flag:");
                get_flag();
                return;
        }
    }
}



int main(){
    init();
    banner();
    init_map();
    
    player.init();
    ai.init(1);

    uint32_t choice = 0;
    while(choice != 4){
        game_menu();
        choice = get_int();
        switch(choice)
        {
            case 1:
                easy_level();
                ai.reset();
                player.reset();
                break;
            case 2:
                medium_level();
                ai.reset();
                player.reset();
                break;
            case 3:
                hard_level();
                break;
            case 4:
                puts("good bye!");
                break;
        }
        init_map();
        if(player.money < 0){
            puts("you lose all money!");
            break;
        }
    }
    return 0;
}