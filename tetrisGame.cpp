//tetris game

#include <chrono>
#include <poll.h> //macOS only
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <random>
#include <queue>
#include <algorithm>
//#include <windows.h> // windows only

//keys
const int UP_KEY   ='w';
const int DOWN_KEY ='s';
const int LEFT_KEY ='a';
const int RIGHT_KEY='d';
const int DROP_KEY =' ';
const int HOLD_KEY ='h';

//game board
const int height=16+3, width=10+2;
bool board[height][width];

//game variables
bool gameOver=0;
int can_hold=-1;
bool block_lock=0;
int posX, posY, rotation, block_num, hold_block_num;
int total_line, total_blocks;
int block_order[10000];
int order_pointer = 0;
//std::queue<int> block_queue;

//render variables
const int RPS=30;//amount of keys can be detected in a second
int rend=0;
const int sleep_microseconds = 1000000/RPS;
const int drop_freq=0.25 * RPS;
int frame = 1;

int kbhit() {//check if any key is pressed
    struct pollfd fds;
    fds.fd = 0;
    fds.events = POLLIN;
    system("stty raw");
    int ret = poll(&fds, 1, 0);
    system("stty cooked");
    return ret;
}
int getch() {//get the input from the usersd
    int ch;
    struct termios t_old, t_new;

    tcgetattr(STDIN_FILENO, &t_old);
    t_new = t_old;
    t_new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
    return ch;
}

const bool blocks[7][4][4][4]={// 7 kind of blocks, 4 rotations
    {//I
        {{0, 0, 0, 0},{1, 1, 1, 1},{0, 0, 0, 0},{0, 0, 0, 0}},
        {{0, 0, 1, 0},{0, 0, 1, 0},{0, 0, 1, 0},{0, 0, 1, 0}},
        {{0, 0, 0, 0},{0, 0, 0, 0},{1, 1, 1, 1},{0, 0, 0, 0}},
        {{0, 1, 0, 0},{0, 1, 0, 0},{0, 1, 0, 0},{0, 1, 0, 0}}
    },
    {//O
        {{0, 0, 0, 0},{0, 1, 1, 0},{0, 1, 1, 0},{0, 0, 0, 0}},
        {{0, 0, 0, 0},{0, 1, 1, 0},{0, 1, 1, 0},{0, 0, 0, 0}},
        {{0, 0, 0, 0},{0, 1, 1, 0},{0, 1, 1, 0},{0, 0, 0, 0}},
        {{0, 0, 0, 0},{0, 1, 1, 0},{0, 1, 1, 0},{0, 0, 0, 0}}
    },
    {//L
        {{0, 0, 0, 0},{0, 0, 0, 1},{0, 1, 1, 1},{0, 0, 0, 0}},
        {{0, 0, 1, 0},{0, 0, 1, 0},{0, 0, 1, 1},{0, 0, 0, 0}},
        {{0, 0, 0, 0},{0, 1, 1, 1},{0, 1, 0, 0},{0, 0, 0, 0}},
        {{0, 1, 1, 0},{0, 0, 1, 0},{0, 0, 1, 0},{0, 0, 0, 0}}
    },
    {//J
        {{0, 0, 0, 0},{1, 0, 0, 0},{1, 1, 1, 0},{0, 0, 0, 0}},
        {{0, 1, 1, 0},{0, 1, 0, 0},{0, 1, 0, 0},{0, 0, 0, 0}},
        {{0, 0, 0, 0},{1, 1, 1, 0},{0, 0, 1, 0},{0, 0, 0, 0}},
        {{0, 1, 0, 0},{0, 1, 0, 0},{1, 1, 0, 0},{0, 0, 0, 0}}
    },
    {//T
        {{0, 1, 0, 0},{1, 1, 1, 0},{0, 0, 0, 0},{0, 0, 0, 0}},
        {{0, 1, 0, 0},{0, 1, 1, 0},{0, 1, 0, 0},{0, 0, 0, 0}},
        {{0, 0, 0, 0},{1, 1, 1, 0},{0, 1, 0, 0},{0, 0, 0, 0}},
        {{0, 1, 0, 0},{1, 1, 0, 0},{0, 1, 0, 0},{0, 0, 0, 0}},
    },
    {//Z
        {{0, 0, 0, 0},{0, 1, 1, 0},{0, 0, 1, 1},{0, 0, 0, 0}},
        {{0, 0, 0, 0},{0, 0, 0, 1},{0, 0, 1, 1},{0, 0, 1, 0}},
        {{0, 0, 0, 0},{0, 0, 0, 0},{0, 1, 1, 0},{0, 0, 1, 1}},
        {{0, 0, 0, 0},{0, 0, 1, 0},{0, 1, 1, 0},{0, 1, 0, 0}},
    },
    {//S
        {{0, 0, 1, 1},{0, 1, 1, 0},{0, 0, 0, 0},{0, 0, 0, 0}},
        {{0, 0, 1, 0},{0, 0, 1, 1},{0, 0, 0, 1},{0, 0, 0, 0}},
        {{0, 0, 0, 0},{0, 0, 1, 1},{0, 1, 1, 0},{0, 0, 0, 0}},
        {{0, 1, 0, 0},{0, 1, 1, 0},{0, 0, 1, 0},{0, 0, 0, 0}},
    }
};

class Tetris {

public:
//block movements
    bool block_can_move(int x, int y){
        //printf("checking if block can move...\n");
        for(int i=0 ; i<4 ; i++){
            for(int j=0 ; j<4 ; j++){ 
                if( !pos_valid(i+x, j+y) && blocks[block_num][rotation][i][j] )return 0;;
                if( !pos_valid(i+x, j+y) )continue;
                if(board[i+x][j+y] && blocks[block_num][rotation][i][j]){
                    //printf("  block cannot move\n");
                    return 0;
                }
            }
        }
        //printf("block can move\n");
        return 1;
    }
    void remove_current_block_from_board(){
        //printf("removing current block from the board...\n");
        for(int i=posX ; i<posX+4 ; i++){
            for(int j=posY ; j<posY+4 ; j++){
                if( !pos_valid(i, j) )continue;
                //printf("%d %d %d %d\n", block_num, rotation, i-posX, j-posY);
                board[i][j]=board[i][j] & ( board[i][j] ^ blocks[block_num][rotation][i-posX][j-posY] );
            }
        }
        //printf("removing current block complete\n");
    }
    void place_current_block(){
        for(int i=0 ; i<4 ; i++){
            for(int j=0 ; j<4 ; j++){
                if( !pos_valid(i+posX, j+posY) )continue;
                board[i+posX][j+posY]=board[i+posX][j+posY] | blocks[block_num][rotation][i][j];
            }
        }
    }
    bool move_block(int newX, int newY){
        //x+1 => down;  y+1 => right ; y-1 => left
        remove_current_block_from_board();
        //render();
        if( !block_can_move(newX, newY) ){
            place_current_block();
            //printf("moving block complete: fail\n");
            return 0;
        }else{
            posX=newX;posY=newY;
            place_current_block();
            //printf("moving block complete: complete\n");
            return 1;
        }
    }
    void rotate_block(){
        remove_current_block_from_board();
        for(int i=0 ; i<4 ; i++){
            rotation++;
            rotation%=4;
            if( block_can_move(posX, posY) ){
                //printf("yes rotate\n");
                place_current_block();
                return;
            }
            //printf("no rotate\n");
        }
        return;
    }
    void hard_drop(){
        remove_current_block_from_board();
        while( block_can_move(posX+1, posY) ){
            posX++;
        }
        place_current_block();
    }
    void block_init(){
        posX=0;
        posY=(width/2)-2;// middle of width
        rotation=0;
        block_lock=0;
        return;
    }  
    void new_block(){
        //printf("new block generating...\n");
        block_num = block_order[order_pointer];
        order_pointer++;
        //block_num = 0;
        block_init();
        move_block(posX, posY);
        total_blocks++;
        return;
    }
    void hold_block(){
        if( !can_hold ){
            printf("cannot hold\n");
            return;
        }
        if( can_hold<0 ){
            remove_current_block_from_board();
            hold_block_num=block_num;
            can_hold=0;
            new_block();
            return;
        }
        remove_current_block_from_board();
        swap(hold_block_num, block_num);
        block_init();
        can_hold=0;
        return;
    }

//game running
    void generate_block_order(int gbon){
        int tmpptr=0;
        gbon/=7;
        gbon++;
        for(int i=0 ; i<gbon ; i++){
            for(int j=0 ; j<7 ; j++, tmpptr++){
                block_order[tmpptr]=j;
            }
            std::random_device rd;
            std::mt19937 yee(rd());
            std::shuffle(block_order+(tmpptr-7), block_order+(tmpptr) , yee );
        }
        return;
    }
    void init() {
        printf("intializing...\n");
        // Initialize the game state and set up the playfield
        gameOver=0;
        posX=0; posY=0; rotation=0;
        can_hold=-1;
        hold_block_num=-1;
        total_blocks=-2;
        total_line=0;
        //printf("\tposX: %d, posY: %d, rotation: %d\n", posX, posY, rotation);
        for(int i=0 ; i<height ; i++){
            for(int j=0 ; j<width ; j++){
                board[i][j]=1;
            }
        }
        for(int i=0 ; i<height-1 ; i++){
            for(int j=1 ; j<width-1 ; j++){
                board[i][j]=0;
            }
        }
        
        new_block();
        //hold_block();//for ai
        printf("initialize complete\n");
        return;
    }
    void handleInput() {
        // Check for user input and update the game state accordingly
        if (kbhit()) {
            int key = getch();
            switch (key) {
                case LEFT_KEY:
                    // Move the tetromino left
                    move_block(posX, posY-1);
                    break;
                case RIGHT_KEY:
                    // Move the tetromino right
                    move_block(posX, posY+1);
                    break;
                case DOWN_KEY:
                    // Move the tetromino down
                    move_block(posX+1, posY);
                    break;
                case UP_KEY:
                    // Rotate the tetromino
                    rotate_block();
                    //printf("yee\n");
                    break;
                case HOLD_KEY:
                    //hold block
                    hold_block();
                    break;
                case DROP_KEY:
                    //hard drop
                    hard_drop();
                    update();
                    break;
            }
            render();
        }
    }
    void update() {
        //printf("before update \tposX: %d, posY: %d\n", posX, posY);
        block_lock=!move_block(posX+1, posY);
        if( block_lock ){
            check_line();
            check_game_over();
            if(gameOver)return;
            can_hold|=1;
            new_block();
        }
        //printf("after update \tposX: %d, posY: %d\n", posX, posY);
        return; 
    }
    void render() {
        // Render the game on the screen

        // Print the playfield and the falling tetromino to the console
        //system("clear");
        printf("\x1b[2J"); //clear the screen
        printf("\x1b[H"); // send cursor to home position
        //printf("frame %d\nposX: %d, posY: %d\n", frame, posX, posY);
        //printf("now holding: %d, can hold: %d\n", hold_block_num, can_hold);
        frame++;
        printf("\n");
        for(int i=0 ; i<height ; i++){//game: 2 ~ height
            for(int j=0 ; j<width; j++){
                if(i==height-1 && j==0){
                    printf("╚═");
                    continue;
                }
                if(i==height-1 && j==width-1 ){
                    printf("╝");
                    continue;
                }
                if( j==0 || j==width-1 ){
                    printf("║ ");
                    continue;
                }
                if(i==height-1){
                    printf("══");
                    continue;
                }
                if(board[i][j]) printf("▩ ");
                else printf("  ");
                
            }printf("\n");
        }printf("\n");
        
        printf("hold:\t\tnext:\n");
        for(int i=0 ; i<4 ; i++){
            for(int j=0 ; j<4 ; j++){
                if(hold_block_num==-1){
                    printf("        ");
                    break;
                }
                if(blocks[hold_block_num][0][i][j]) printf("▩ ");
                else printf("  ");
            }
            printf("\t");
            for(int j=0 ; j<4 ; j++){
                if(blocks[ block_order[order_pointer] ][0][i][j]) printf("▩ ");
                else printf("  ");
            }
            printf("\n");
        }printf("\n");
        printf("total line: %d\ntotal blocks: %d\n", total_line, total_blocks);
        
    }

private:
    // Game state variables, such as the playfield and the current position of the falling tetromino
    
    // int random_num(int randA, int randB){
    //     std::random_device rd;
    //     std::mt19937 yee(rd());
    //     std::uniform_int_distribution<> distr(randA, randB);
    //     return (distr(yee) );// random number from 0~6
    // }
    
    void swap(int &swap_a, int &swap_b){
        int swap_tmp = swap_a;
        swap_a = swap_b;
        swap_b = swap_tmp;
        return;
    }
    bool pos_valid(int checkX, int checkY){
        //printf("position checking...\n");
        return (0<=checkX && checkX<height && 0<=checkY && checkY<width )? 1: 0;
    }
    void all_move_down(int line_num){
        for(int i=line_num ; i>0 ; i--){
            for(int j=1 ; j<width-1 ; j++){
                board[i][j]=board[i-1][j];
            }
        }
    }
    void check_line(){
        for(int line_num=height-2 ; line_num>=0 ; line_num--){
            bool isline = 1;
            for(int j=1 ; j<width-1 && isline ; j++){
                isline&=board[line_num][j];
            }
            if(isline){
                total_line++;
                all_move_down(line_num);
                line_num++;
            }
        }
    }
   
    void check_game_over(){
        for(int i=1 ; i<width-1 ; i++){
            gameOver |= board[3][i];
        }
    }

};

int main(){

    Tetris game;
    game.generate_block_order(1000);
    for(int i=0 ; i<100 ; i++){
        if(i%7==0)printf("\n");
        printf("%d ", block_order[i]);
    }
    
    //return 0;
    game.init();
/*
    while (!gameOver) {
        game.handleInput();
        if(rend%RPS == 0){
            AIcontrol(); // 
            game.update();
            //game.render();
        }
        //usleep(sleep_microseconds); // to control the game speed
    }
    printf("\nGAME OVER\n");
*/
    int update_time;
    for(int i=0 ; i<2147483647 ; i++, rend++){
        update_time=i;
        if(gameOver){
            printf("yeet");
            break;
        }
        game.handleInput();
        if( rend%drop_freq == 0){
            game.update();
            game.render();
        }
        usleep(2*sleep_microseconds); // to control the game speed
    }
    printf("\nupdate times: %d\nGAME OVER\n", update_time);

    return 0;
}
/* 17,000,000 updates per sec (without ai, height 10000)*/

/*
▩
═
*/