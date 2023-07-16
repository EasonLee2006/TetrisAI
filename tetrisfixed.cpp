#include <chrono>
#include <poll.h> //macOS only
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <random>
#include <queue>
#include <algorithm>
#include <vector>
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
using namespace std;

int gamemode = 0;
bool to_render = 1;
double sleep_time = 0;//sec
const int Max_Block = ( 10000 / 7 ) * 7;
const int Max_Line  = 2000000;
const int predict_blocks = 1;
const int num_of_block_queues = 1;
int playing_queue = 0;

//game board
const int height=18+5, width=10+2;
bool current_board[height][width];
bool temp_boards[predict_blocks+2][height][width];
bool another_temp_board[height][width];
bool temp_hold_block[predict_blocks+2];

//pics
const int board_start_x = 150;//354;
const int board_start_y = 300;//320;
const int per_block_width = 24;

//keys
const int CW_ROT_KEY ='k';
const int CCW_ROT_KEY='j';
const int UP_KEY     ='w';//for debug
const int DOWN_KEY   ='s';
const int LEFT_KEY   ='a';
const int RIGHT_KEY  ='d';
const int DROP_KEY   =' ';
const int HOLD_KEY   ='h';

//constants
const int MAX_INT = 2147483647;
const int num_of_tetromino = 7;
const int num_of_rotations = 4;
const int num_of_offsets   = 5;
const int CW =  1;//clockwise
const int CCW= -1;//counter clockwise

//game variables
bool gameOver=0;
int  block_lock_counter= 0;
int  block_lock_limit  = 3;
bool hold_used = 0;
int  total_line=0, total_blocks=0;
int  lines_cleared = 0;
int  lines_clear_at_once[5];
int  temp_line_clear_arr[5];
std::string block_queue[ num_of_block_queues ];

//render variables
const int RPS=10;//amount of keys can be detected in a second
int rend=0;
const int sleep_microseconds = 1000000/RPS;
const int one_second   = 1000000;
const int drop_freq=0.5 * RPS;
int frame = 1;

//AI's variables
double current_cost=0;
int num_of_genes = 7;
int population_size = 20;
int combo = 0;
int current_cost_mode = 0;

struct movement{
    int posY;
    int rotation;
    int to_hold;
    double cost;
} best_move;

struct block_status{
    block_status() : rotation(0){};
    int piece, posX, posY, rotation, order;
};

struct THE_POPULATION{
    int id;
    int switch_to_attack;
    int switch_to_defense;
    double hole_weight[2] ;
    double peak_weight[2] ;
    double posX_weight[2] ;
    double bumpy_weight[2];
    double tth_weight[2]  ;
    double pits_weight[2] ;
    double line_weight[2] ;
    THE_POPULATION() : fitness(-1){};
    int fitness;
};

std::vector<THE_POPULATION> population( population_size );
THE_POPULATION the_yeet_one;

block_status current_block, holding_block;

const bool tetrominos[ num_of_tetromino ][ num_of_rotations ][5][5]={//0I 1J 2L 3O 4S 5T 6Z 
    {//I 0
        {{0, 0, 0, 0, 0},{0, 0, 0, 0, 0},{0, 1, 1, 1, 1},{0, 0, 0, 0, 0},{0, 0, 0, 0, 0}},
        {{0, 0, 1, 0, 0},{0, 0, 1, 0, 0},{0, 0, 1, 0, 0},{0, 0, 1, 0, 0},{0, 0, 0, 0, 0}},
        {{0, 0, 0, 0, 0},{0, 0, 0, 0, 0},{1, 1, 1, 1, 0},{0, 0, 0, 0, 0},{0, 0, 0, 0, 0}},
        {{0, 0, 0, 0, 0},{0, 0, 1, 0, 0},{0, 0, 1, 0, 0},{0, 0, 1, 0, 0},{0, 0, 1, 0, 0}}
    },
    {//J 1
        {{0, 0, 0},{1, 1, 1},{1, 0, 0}},
        {{0, 1, 0},{0, 1, 0},{0, 1, 1}},
        {{0, 0, 1},{1, 1, 1},{0, 0, 0}},
        {{1, 1, 0},{0, 1, 0},{0, 1, 0}}
    },
    {//L 2
        {{0, 0, 0},{1, 1, 1},{0, 0, 1}},
        {{0, 1, 1},{0, 1, 0},{0, 1, 0}},
        {{1, 0, 0},{1, 1, 1},{0, 0, 0}},
        {{0, 1, 0},{0, 1, 0},{1, 1, 0}}
    },
    {//O 3
        {{0, 0, 0},{0, 1, 1},{0, 1, 1}},
        {{0, 1, 1},{0, 1, 1},{0, 0, 0}},
        {{1, 1, 0},{1, 1, 0},{0, 0, 0}},
        {{0, 0, 0},{1, 1, 0},{1, 1, 0}}
    },
    {//S 4
        {{0, 0, 0},{1, 1, 0},{0, 1, 1}},
        {{0, 0, 1},{0, 1, 1},{0, 1, 0}},
        {{1, 1, 0},{0, 1, 1},{0, 0, 0}},
        {{0, 1, 0},{1, 1, 0},{1, 0, 0}}
    },
    {//T 5
        {{0, 0, 0},{1, 1, 1},{0, 1, 0}},
        {{0, 1, 0},{0, 1, 1},{0, 1, 0}},
        {{0, 1, 0},{1, 1, 1},{0, 0, 0}},
        {{0, 1, 0},{1, 1, 0},{0, 1, 0}}
    },
    {//Z 6
        {{0, 0, 0},{0, 1, 1},{1, 1, 0}},
        {{0, 1, 0},{0, 1, 1},{0, 0, 1}},
        {{0, 1, 1},{1, 1, 0},{0, 0, 0}},
        {{1, 0, 0},{1, 1, 0},{0, 1, 0}}
    }
};
const char tetromino_shape[ num_of_tetromino ] = {'I', 'J', 'L', 'O', 'S', 'T', 'Z'};
const int tetromino_width[ num_of_tetromino ]={5, 3, 3, 3, 3, 3, 3};
const int tetromino_color[ num_of_tetromino+2 ]={14326528, 13180981, 17906, 40683, 48154, 8913087, 2228457, 6974058, 10066329};
const int rotation_offset[ num_of_tetromino ][ num_of_rotations ][ num_of_offsets ][2] = {
    //offsets: num_of_blocks, num_of_rotations, num_of_offsets, 2
    {//I
        {/*0*/{0, 0}, {-1, 0}, {2, 0}, {-1, 0}, {2, 0}},
        {/*R*/{-1, 0}, {0, 0}, {0, 0}, {0, 1}, {0, -2}},
        {/*2*/{-1, 1}, {1, 1}, {-2, 1}, {1, 0}, {-2, 0}},
        {/*L*/{0, 1}, {0, 1}, {0, 1}, {0, -1}, {0, 2}}
    },
    {//J, L, S, T, Z
        {/*0*/{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
        {/*R*/{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}},
        {/*2*/{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
        {/*L*/{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}
    },
    {//L
        {/*0*/{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
        {/*R*/{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}},
        {/*2*/{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
        {/*L*/{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}
    },
    {//O
        {/*0*/{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
        {/*R*/{0, -1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
        {/*2*/{-1, -1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
        {/*L*/{-1, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}
    },
    {//S
        {/*0*/{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
        {/*R*/{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}},
        {/*2*/{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
        {/*L*/{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}
    },
    {//T
        {/*0*/{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
        {/*R*/{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}},
        {/*2*/{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
        {/*L*/{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}
    },
    {//Z
        {/*0*/{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
        {/*R*/{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}},
        {/*2*/{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
        {/*L*/{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}
    }
};

const string debug_board[20]={
    "0..........|",
    "9..........|",
    "8..........|",
    "7..........|",
    "6..........|",
    "5..........|",
    "4..........|",
    "3..........|",
    "2..........|",
    "1..........|",
    "0..........|",
    "9..........|",
    "8..........|",
    "7.........#|",
    "6..##....##|",
    "5#####...##|",
    "4######.###|",
    "3######.###|",
    "2######.###|",
    "1######.###|"
};
class debug{
public:

    void print_block_queue(){
        for(int j=0 ; j<num_of_block_queues ; j++){
            for(int i=0 ; i<Max_Block ; i++){
                if(i%7==0)printf("\n");
                printf("%c", block_queue[j][i]+48);
            }
            printf("\n");
        }
    }
    void init_the_yeet(){
        double temp_array[7] = {1.868625, 1.748223, 3.743000, 2.180668, 2.406600, 0.943628, -3.624588};
        double temp_array2[7] = {1.133100, 4.481100, 1.324800, -3.657000, 4.316000, -0.301000, -0.756000};
        //1.221000, 4.209000, 3.609000, -0.860000, 3.404700, 1.343925, -0.541000
        //2.560050, 1.015200, 3.937500, 1.048000, 2.651400, 2.540160, -2.493000
        //1.868625, 1.748223, 3.743000, 2.180668, 2.406600, 0.943628, -3.624588
        the_yeet_one.hole_weight[0] =temp_array[0];
        the_yeet_one.peak_weight[0] =temp_array[1];
        the_yeet_one.posX_weight[0] =temp_array[2];
        the_yeet_one.bumpy_weight[0]=temp_array[3];
        the_yeet_one.tth_weight[0]  =temp_array[4];
        the_yeet_one.pits_weight[0] =temp_array[5];
        the_yeet_one.line_weight[0] =temp_array[6];
        
        the_yeet_one.hole_weight[1] =temp_array2[0];
        the_yeet_one.peak_weight[1] =temp_array2[1];
        the_yeet_one.posX_weight[1] =temp_array2[2];
        the_yeet_one.bumpy_weight[1]=temp_array2[3];
        the_yeet_one.tth_weight[1]  =temp_array2[4];
        the_yeet_one.pits_weight[1] =temp_array2[5];
        the_yeet_one.line_weight[1] =temp_array2[6];
    }
    void debug_board_init(bool (*board)[width]){
        for(int i=1 ; i<height-4 ; i++){
            for(int j=1 ; j<width-1 ; j++){
                if(i>20)return;
                if(debug_board[20-i][j]=='#'){
                    board[i][j]=1;
                }
            }
        }
    }
    void test_render(bool (*board)[width], block_status this_block) {
        // Render the game on the screen

        // Print the playfield and the falling tetromino to the console
        //printf("   \033[H"); // send cursor to home position
        frame++;
        printf("\n");
        for(int i=height-1 ; i>0 ; i--){//
            printf("\033[%d;3H", height-i+1);
            for(int j=1 ; j<width-1; j++){
                
                if( board[i][j] && 0<=i-this_block.posX && i-this_block.posX<5 && 0<=j-this_block.posY && j-this_block.posY<5 && tetrominos[ this_block.piece ][ this_block.rotation ][ i-this_block.posX ][ j-this_block.posY ]){
                    printf("\033[33m▩ \033[0m");
                    continue;
                }
                if(board[i][j]){
                    printf("▩ ");
                    continue;
                }

                if(i == height-4){
                    printf("\033[31m_ \033[0m");
                    continue;
                }
                else printf("  ");
                
            }//printf("\n");
        }
        printf("\033[%d;1H", height+3);
        
        //printf("hold: %d, next: %d  \n", holding_block.piece, block_queue[0][ current_block.order+1 ]);
        printf("total line: %d\ntotal blocks: %d \n", total_line, this_block.order+1);
        printf("current order: %d\n", current_block.order);
        printf("ccm %d\n", current_cost_mode);
        //print_block_queue();
        usleep(0*one_second);
        return;
    }
    void print_gameboard(bool (*board)[width]){
        printf("      \033[H"); // send cursor to home position
        printf("\n");
        for(int i=height-1 ; i>=0 ; i--){//
            for(int j=0 ; j<width; j++){
                if(i==0 && j==0){
                    printf("╚═");
                    continue;
                }
                if(i==0 && j==width-1 ){
                    printf("╝");
                    continue;
                }
                if( j==0 || j==width-1 ){
                    printf("%d ", i%10);//║
                    continue;
                }
                if(i==0){
                    printf("%d ", j%10);//══
                    continue;
                }
                if(board[i][j]){
                    printf("▩ ");
                    continue;
                }

                if(i == height-4){
                    printf("\033[31m_ \033[0m");
                    continue;
                }
                else printf("  ");
                
            }printf("\n");
        }printf("\n");
        
    }
    bool is_block_color(int colorCode){
        for(int i=0 ; i<num_of_tetromino+2 ; i++){
            if (colorCode== tetromino_color[i])return 1;
        }
        return 0;
    }
    int get_pixel_color(int x, int y){
        CGDirectDisplayID displayId = kCGDirectMainDisplay;
        CGImageRef image = CGDisplayCreateImageForRect(displayId, CGRectMake(x, y, 1, 1));
        CGDataProviderRef provider = CGImageGetDataProvider(image);
        CFDataRef data = CGDataProviderCopyData(provider);
        const UInt8* pixelData = CFDataGetBytePtr(data);

        // Color components
        int red = pixelData[0];
        int green = pixelData[1];
        int blue = pixelData[2];
        int alpha = pixelData[3];
        int colorCode = (red << 16) | (green << 8) | blue;

        // Release resources
        CFRelease(data);
        CGImageRelease(image);
        //printf("%d " , colorCode);
        return colorCode;
    }
    void get_gameboard( bool (*board)[width] ){
        int pixel_x = board_start_x;
        int pixel_y = board_start_y;
        for(int i=height-5 ; i>0 ; i--){
            for(int j=1 ; j<width-1 ; j++){
                //printf("%d, %d  ", pixel_x, pixel_y);
                board[i][j]=is_block_color( get_pixel_color(pixel_x, pixel_y) );
                pixel_x+=per_block_width;
            }
            //printf("\n");
            pixel_x = board_start_x;
            pixel_y+=per_block_width;
        }
    }
    int scan_nextPiece(){
        int this_color = get_pixel_color( board_start_x+12 * per_block_width, board_start_y );
        for(int i=1 ; i<num_of_tetromino ; i++){
            if(this_color == tetromino_color[i] )
            return i;
        }
        return 0;
    }

    double random_double(double randA, double randB){
        std::random_device rd;
        std::mt19937 yee(rd());
        std::uniform_int_distribution<> distr(randA*1000, randB*1000);
        return ( distr(yee)/1000.0 );// random number from 0~6
    }
    void pressKey(unsigned short keyCode)
    {
        CGEventRef keyDownEvent = CGEventCreateKeyboardEvent(NULL, keyCode, true);
        CGEventRef keyUpEvent = CGEventCreateKeyboardEvent(NULL, keyCode, false);

        // Set the virtual key state
        CGEventSetType(keyDownEvent, kCGEventKeyDown);
        CGEventSetType(keyUpEvent, kCGEventKeyUp);

        // Post the key events
        CGEventPost(kCGHIDEventTap, keyDownEvent);
        CGEventPost(kCGHIDEventTap, keyUpEvent);

        // Release the events
        CFRelease(keyDownEvent);
        CFRelease(keyUpEvent);

        usleep( random_double(0.05, 0.07) * one_second );
    }
};

class Tetris{
private:
    int start_x, start_y, end_x, end_y;
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
    bool pos_valid(int check_x, int check_y){
        if(check_x<0 || check_x>=height || check_y<0 || check_y>=width){
            return 0;
        }
        return 1;
    }
    void locate(block_status locating_block, int locate_x, int locate_y){
        start_x = locate_x,
        end_x   = locate_x + tetromino_width[ locating_block.piece ], 
        start_y = locate_y, 
        end_y   = locate_y + tetromino_width[ locating_block.piece ];
    }
    void move_all_down(bool (*board)[width], int line_index){
        for(int i=line_index ; i<height-2 ; i++){
            for(int j=1 ; j<width-1 ; j++){
                board[i][j] = board[i+1][j];
            }
        }
    }

    void clear_screen(){
        printf("\x1b[2J");
        printf("\x1b[H");
        //printf("screen cleared\n");
    }
    void board_init(bool (*board)[width]){
        for(int i=0 ; i<height ; i++){
            for(int j=0 ; j<width ; j++){
                board[i][j]=1;
            }
        }
        for(int i=1 ; i<height ; i++){
            for(int j=1 ; j<width-1 ; j++){
                board[i][j]=0;
            }
        }
    }
    
    void hold_block_init(){
        holding_block.piece = -1;
        hold_used = 0;
    }

public:
    void create_block_queue(int block_queue_num, int block_queue_length){
        for(int k=0 ; k<block_queue_num ; k++){
            for(int i=0 ; i<block_queue_length ; i++){
                block_queue[ k ]+=(char)((i%7));
                if(i%7==6){
                    std::random_device rd;
                    std::mt19937 yee(rd());
                    std::shuffle(block_queue[k].end()-7, block_queue[k].end() , yee );
                }
                
            }
        }
        return;
    }
    bool is_game_over(bool (*board)[width]){
        for(int j=1 ; j<width-1 ; j++){
            if( board[height-4][j] )return 1;
        }
        return 0;
    }
    void remove_block_from_board(bool (*board)[width], block_status removing_block){
        locate( removing_block, removing_block.posX, removing_block.posY);
        for(int i=start_x ; i<end_x ; i++){
            for(int j=start_y ; j<end_y ; j++ ){
                board[i][j] = board[i][j] & !tetrominos[ removing_block.piece ][ removing_block.rotation ][ i-start_x ][ j-start_y ];
            }//11->0 10->1 01->0 00->0
        }
    }
    bool block_can_move(bool (*board)[width], block_status &checking_block , int checking_X, int checking_Y){
        locate( checking_block, checking_X, checking_Y);
        for(int i=start_x ; i<end_x ; i++){
            for(int j=start_y ; j<end_y ; j++ ){
                if( !pos_valid(i, j) ){
                    if( tetrominos[ checking_block.piece ][ checking_block.rotation ][ i-start_x ][ j-start_y ] ){
                        return 0;
                    }
                    else continue;
                }
                if( board[i][j] & tetrominos[ checking_block.piece ][ checking_block.rotation ][ i-start_x ][ j-start_y ] ){
                    return 0;
                }
            }
        }
        return 1;
    }
    bool move_block(bool (*board)[width], block_status &moving_block, int move_to_x, int move_to_y){
        remove_block_from_board( board, moving_block );
        if( !block_can_move( board, moving_block, move_to_x, move_to_y) ){
            move_block( board, moving_block, moving_block.posX, moving_block.posY );
            return 0;
        }
        locate( moving_block , move_to_x, move_to_y);
        for(int i=start_x ; i<end_x ; i++){
            for(int j=start_y ; j<end_y ; j++ ){
                if( !pos_valid(i, j) ){
                    continue;
                }
                board[i][j] |= tetrominos[ moving_block.piece ][ moving_block.rotation ][ i-start_x ][ j-start_y ]; 
            }
        }
        moving_block.posX=move_to_x;
        moving_block.posY=move_to_y;
        return 1;
    }
    void hard_drop(bool (*board)[width], block_status &dropping_block){
        remove_block_from_board( board, dropping_block );
        while( block_can_move( board, dropping_block, dropping_block.posX-1, dropping_block.posY) ){
            dropping_block.posX--;
        }
        move_block( board, dropping_block, dropping_block.posX, dropping_block.posY );
        block_lock_counter=block_lock_limit;
        return;
    }
    void rotate_block(bool (*board)[width], block_status &rotating_block, int direction){
        remove_block_from_board( board, rotating_block );
        block_status rot_temp_block = rotating_block;
        int new_rotation = (rotating_block.rotation + direction + 4) % 4;
        rot_temp_block.rotation = new_rotation;
        int x_offset = 0;
        int y_offset = 0;
        //printf("\n%d -> %d\n", rotating_block.rotation, new_rotation);//debug
        for(int i=0 ; i<num_of_offsets ; i++){
            x_offset = rotation_offset[ rot_temp_block.piece ][ rotating_block.rotation ][i][1] - rotation_offset[ rot_temp_block.piece ][ new_rotation ][i][1];
            y_offset = rotation_offset[ rot_temp_block.piece ][ rotating_block.rotation ][i][0] - rotation_offset[ rot_temp_block.piece ][ new_rotation ][i][0];
            //printf("\nx: %d, y: %d", x_offset, y_offset);//debug
            if( !block_can_move( board, rot_temp_block, rot_temp_block.posX + x_offset, rot_temp_block.posY + y_offset) ){
                continue;
            }else{
                rotating_block.posX += x_offset;
                rotating_block.posY += y_offset;
                rotating_block.rotation = new_rotation;
                break;
            }
        }
        move_block( board, rotating_block, rotating_block.posX, rotating_block.posY );
        return;
    }
    void block_pos_init(bool (*board)[width], block_status &target_block){
        target_block.posX = height-4;
        target_block.posY = ((width-tetromino_width[target_block.piece])/2);
        target_block.rotation = 0;
        move_block(board, target_block, target_block.posX, target_block.posY);
    }
    void new_block(bool (*board)[width], block_status &target_block, int queue_index, int order_index){
        //if(order_index>=Max_Block)gameOver=1;
        target_block.order = order_index % Max_Block;
        target_block.piece = block_queue[ queue_index ][ order_index ];
        block_pos_init(board, target_block);    
        return;
    }
    
    void hold_block(bool (*board)[width], block_status &to_hold_block, int &this_holding_block ){
        if( hold_used )return;
        remove_block_from_board( board, to_hold_block );
        if( this_holding_block==-1 ){
            this_holding_block = to_hold_block.piece;
            new_block(board, to_hold_block, 0, to_hold_block.order+1);
        }else{
            int temp_block_piece;
            temp_block_piece = this_holding_block;
            this_holding_block = to_hold_block.piece;
            to_hold_block.piece = temp_block_piece;
            block_pos_init(board, to_hold_block);
        }
        move_block( board, to_hold_block, to_hold_block.posX, to_hold_block.posY );
        hold_used = 1;
        return;
    }
    
    int  check_lines(bool (*board)[width], block_status &this_block){
        int line_count = 0;
        bool isline = 1;
        for(int i=this_block.posX ; i<this_block.posX+5 ; i++){
            if(i<1 || i>=height)continue;
            isline=1;
            for(int j=1 ; j<width-1 ; j++){
                isline &= board[i][j];
            }
            if( isline ){
                line_count++;
                move_all_down(board, i);
                i--;
            }
        }
        return line_count;
    }
    
    void handleInput() {
        // Check for user input and update the game state accordingly
        if (kbhit()) {
            int key = getch();
            switch (key) {
                case LEFT_KEY:
                    move_block( current_board, current_block, current_block.posX, current_block.posY-1);
                    break;
                case RIGHT_KEY:
                    move_block( current_board, current_block, current_block.posX, current_block.posY+1);
                    break;
                case UP_KEY:
                    //move_block( current_block, current_block.posX+1, current_block.posY);
                    break;
                case DOWN_KEY:
                    move_block( current_board, current_block, current_block.posX-1, current_block.posY);
                    break;
                case CW_ROT_KEY:                   // Rotate the tetromino
                    rotate_block( current_board, current_block, CW);
                    break;
                case CCW_ROT_KEY:                   // Rotate the tetromino
                    rotate_block( current_board, current_block, CCW);
                    break;
                case HOLD_KEY:
                    hold_block( current_board, current_block , holding_block.piece);
                    break;
                case DROP_KEY:
                    hard_drop( current_board, current_block );
                    update(current_board, current_block );
                    break;
            }
            render(current_board, current_block);
        }
    }
    void init(){
        clear_screen();
        create_block_queue( num_of_block_queues, Max_Block );
        board_init( current_board );
        //debug_board_init();
        hold_block_init();
        new_block(current_board, current_block, 0, 0);
        //render( current_board, current_block );
    }

    void render(bool (*board)[width], block_status this_block) {
        // Render the game on the screen

        // Print the playfield and the falling tetromino to the console
        printf("      \033[H"); // send cursor to home position
        frame++;
        printf("\n");
        for(int i=height-1 ; i>=0 ; i--){//
            for(int j=0 ; j<width; j++){
                if(i==0 && j==0){
                    printf("╚═");
                    continue;
                }
                if(i==0 && j==width-1 ){
                    printf("╝");
                    continue;
                }
                if( j==0 || j==width-1 ){
                    printf("║ ");//║
                    continue;
                }
                if(i==0){
                    printf("══");//══
                    continue;
                }
                if( board[i][j] && 0<=i-this_block.posX && i-this_block.posX<5 && 0<=j-this_block.posY && j-this_block.posY<5 && tetrominos[ this_block.piece ][ this_block.rotation ][ i-this_block.posX ][ j-this_block.posY ]){
                    printf("\033[33m▩ \033[0m");
                    continue;
                }
                if(board[i][j]){
                    printf("▩ ");
                    continue;
                }

                if(i == height-4){
                    printf("\033[31m_ \033[0m");
                    continue;
                }
                else printf("  ");
                
            }printf("\n");
        }printf("\n");

        printf("hold:\t\tnext:\n");
        for(int i=0 ; i<5 ; i++){
            for(int j=0 ; j<5 ; j++){
                if(holding_block.piece==-1){
                    printf("        ");
                    break;
                }
                if(tetrominos[holding_block.piece][0][i][j]) printf("  ");
                else printf("  ");
            }
            printf("\t");
            for(int j=0 ; j<5 ; j++){
                if( tetrominos[ block_queue[0][ current_block.order+1 ] ][0][i][j] ) printf("▩ ");
                else printf("  ");
            }
            printf("\n");
        }printf("\n");
        
        //printf("next: %c  \n",  tetromino_shape[ block_queue[0][ current_block.order+1 ] ]);
        printf("total line: %d\ntotal blocks: %d\n", total_line, this_block.order+1);
        usleep(0*one_second);
        return;
    }
    void update(bool (*board)[width], block_status &this_block){
        //printf("ttl3: %d\n", total_line);
        if( !move_block(board, this_block, this_block.posX-1, this_block.posY) ){
            block_lock_counter++;
        }else{
            block_lock_counter=0;
        }
        if( block_lock_counter>=block_lock_limit ){
            //printf("ttl2: %d\n", total_line);
            lines_cleared = check_lines(board, this_block);
            lines_clear_at_once[ lines_cleared ]++;
            total_line += lines_cleared;
            //printf("ttl1: %d\n", total_line);
            gameOver = is_game_over(board);
            if( gameOver )return;
            hold_used=0;
            new_block(board, this_block, 0, this_block.order+1);
        }
        //render(board, this_block);
        return; 
    }
};

class TetrisAI{
private:
    Tetris T;
    int ABS(int n){
        return (n<0)? -n : n;
    }
    int get_hole(bool (*board)[width]){//checked
        int attribute_1_score=0;
        for(int i=1 ; i<width-1 ; i++){
            int j=height-4;
            for( ; j>0 ; j--){
                if(board[j][i])break;
            }
            j--;
            int checklimit = j-4;
            for( ; j>0 ; j--){
                if(!board[j][i]){
                    attribute_1_score++;
                }
            }
        }
        return attribute_1_score;
    }
    int get_peak(bool (*board)[width]){//checked
        for(int i=height-1 ; i>0 ; i--){
            for(int j=1 ; j<width-1 ; j++){
                if(board[i][j]){
                    if(i>=height-4)return 1000;
                    if(i >= 8) return 8;
                    else return i;
                }
            }
        }
        return 1000;
    }
    int get_bumpy(bool (*board)[width]){//checked (can still  improve)
        int bumpy = 0, b_left=0, b_right=0;
        for(int i=1 ; i<width-1 ; i++){
            b_left=b_right;
            for(int j=height-1 ; j>=0 ; j--){
                if(board[j][i]){
                    b_right=j;
                    break;
                }
            }
            if(i==1)continue;
            bumpy+=ABS(b_right-b_left);
        }
        return bumpy;
    }
    int get_total_h(bool (*board)[width]){//checked
        int total_h=0;
        for(int i=1 ; i<width-1 ; i++){
            for(int j=height-1 ; j>=0 ; j--){
                if(board[j][i]){
                    total_h+=j;
                    break;
                }
            }
        }
        return total_h;
    }
    int get_pits(bool (*board)[width]){//checked
        int pits=width-2;
        for(int i=1 ; i<width-1 ; i++){
            for(int j=height-1 ; j>0 ; j--){
                if(board[j][i]){
                    pits--;
                    break;
                }
            }
        }
        return pits;
    }

    double get_cost(bool (*board)[width], THE_POPULATION choosen_one, block_status &this_block, int lines){
        double total_cost=0;
        // if(get_peak( board ) >= 12){
        //     current_cost_mode = 1;
        // }
        // if(get_peak(board) <=4){
        //     current_cost_mode = 0;
        // }
        total_cost = 
            get_hole( board )   * choosen_one.hole_weight[current_cost_mode] +
            get_peak( board )   * choosen_one.peak_weight[current_cost_mode] +
            this_block.posX     * choosen_one.posX_weight[current_cost_mode] +
            get_bumpy( board )  * choosen_one.bumpy_weight[current_cost_mode]+
            get_total_h( board )* choosen_one.tth_weight[current_cost_mode]  +
            get_pits( board )   * choosen_one.pits_weight[current_cost_mode] +
            (1+lines) * choosen_one.line_weight[0];
        return total_cost;
    }
public:
    debug dd;
    void array_copy_paste( int* copy_arr, int* paste_arr, int arr_length ){
        for(int i=0 ; i<arr_length ; i++){
            paste_arr[i]=copy_arr[i];
        }
    }
    void board_copy_paste( bool (*copy_board)[width], bool (*paste_board)[width] ){
        for(int i=0 ; i<height ; i++){
            for(int j=0 ; j<width ; j++){
                paste_board[i][j] = copy_board[i][j];
            }
        }
        return;
    }
    double ai_decide(block_status this_block, int predict_block_index, int this_lines, int this_combo){
        //printf("  ttl: %d pbi: %d\n", total_line, predict_block_index);
        double cost = MAX_INT;
        double cur_best_cost = MAX_INT;
        int cur_line = 0;
        bool (*deciding_board)[width] = temp_boards[ predict_block_index ];

        if(predict_block_index == 1){
            best_move.cost = MAX_INT;
        }
        block_status next_block = this_block;
        next_block.order++;
        next_block.order%=Max_Block;
        for(int r=0 ; r<4 ; r++){
            board_copy_paste( temp_boards[predict_block_index-1], deciding_board );
            temp_hold_block[predict_block_index] = temp_hold_block[predict_block_index-1];
            T.new_block(deciding_board, this_block, playing_queue, this_block.order);
            if(r>=1 && this_block.piece == 3)return cur_best_cost;
            if(r>=2 && (this_block.piece == 0 || this_block.piece == 4 || this_block.piece == 6) )return cur_best_cost;
            for(int i=0 ; i<r ; i++){
                T.rotate_block(deciding_board, this_block, CW);
            }
            while( T.move_block( deciding_board, this_block, this_block.posX, this_block.posY-1 ) );
            // ^ move to the left

            for(int y=this_block.posY ; y<width ; y++){
                //printf("im here\n");
                board_copy_paste( temp_boards[predict_block_index-1], deciding_board );
                
                if( !T.block_can_move( deciding_board, this_block, height-4, y ) )break;
                T.move_block(deciding_board, this_block, height-4, y);
                
                T.hard_drop(deciding_board, this_block);
                cur_line = T.check_lines(deciding_board, this_block);
                if( get_peak( deciding_board )>=height-4 ){//over height
                    cost = get_cost( deciding_board , the_yeet_one, this_block, cur_line+this_lines )+10000*(predict_blocks-predict_block_index);
                }else if(predict_block_index<=predict_blocks){
                    cost = ai_decide( next_block, (predict_block_index+1) % Max_Block, cur_line+this_lines, this_combo);
                }else{
                    int the_peak = get_peak( deciding_board );
                    if( the_peak>=12 ){
                        for(int i=the_peak-6 ; i<height ; i++){
                            for(int j=1 ; j<width-1 ; j++){
                                another_temp_board[i-5][j] = deciding_board[i][j];
                            }
                        }
                        cost = get_cost( another_temp_board , the_yeet_one, this_block, cur_line+this_lines);
                    }
                    else{
                        cost = get_cost( deciding_board , the_yeet_one, this_block, cur_line+this_lines);
                    }
                }

                if(cost < cur_best_cost){
                    cur_best_cost = cost;   
                }
                if(predict_block_index == 1 && cur_best_cost<best_move.cost){
                    best_move.cost = cur_best_cost;
                    best_move.rotation = this_block.rotation;
                    best_move.posY = this_block.posY;
                    //printf("best: cost %f, rot %d, Y %d\n", best_move.cost, best_move.rotation, best_move.posY);
                }
            }
        }
        return cur_best_cost;
    }
    void ai_move_block(block_status &play_block){
        while(play_block.rotation != best_move.rotation){
            T.rotate_block( current_board, play_block, CW );
        }
        T.move_block(current_board, play_block, play_block.posX, best_move.posY);
        T.hard_drop( current_board, play_block );
    }
    
    void control_move_block( block_status &play_block ){
        int some_index = 0;
        while(play_block.rotation != best_move.rotation && some_index<25){
            T.rotate_block( current_board, play_block, CW );
            dd.pressKey(kVK_UpArrow);
            some_index++;
        }
        while(play_block.posY > best_move.posY && some_index<25){
            T.move_block(current_board, play_block, play_block.posX, play_block.posY-1);
            //T.render( current_board, current_block );
            dd.pressKey(kVK_LeftArrow);
            some_index++;
        }
        // for(int i=0 ;i<5 && some_index<25 ; i++){
        //     T.move_block(current_board, play_block, play_block.posX, play_block.posY-1);
        //     dd.pressKey(kVK_LeftArrow);
        //     some_index++;
        // }
        while(play_block.posY < best_move.posY && some_index<25){
            T.move_block(current_board, play_block, play_block.posX, play_block.posY+1);
            //T.render( current_board, current_block );
            dd.pressKey(kVK_RightArrow);
            some_index++;
        }
        if(some_index>=25){
            gameOver=1;
            return;
        }
        //usleep( 0.1 * one_second );
        dd.pressKey(kVK_Space);
        T.hard_drop( current_board, play_block );
    }
    
    void ai_play(){
        debug dd;
        int some_index = 1;
        clock_t time_1;
        clock_t time_2;
        time_1 = clock();
        for(int i=0 ; i<MAX_INT && !gameOver ; i++){
            array_copy_paste( lines_clear_at_once, temp_line_clear_arr, 5 );
            T.remove_block_from_board(current_board, current_block);
            board_copy_paste(current_board, temp_boards[0] );//yee
            temp_hold_block[0] = holding_block.piece;
            int temp_total_ttl = total_line;
            ai_decide( current_block, 1 , 0, 0);//must be 1 //yee
            total_line = temp_total_ttl;
            array_copy_paste( temp_line_clear_arr, lines_clear_at_once, 5 );
            T.move_block( current_board, current_block, current_block.posX, current_block.posY ); //yee
            //printf("best move: cost: %f, Y %d\n", best_move.cost, best_move.posY);
            ai_move_block( current_block );// yee
            T.update( current_board, current_block ); //yee
            //dd.test_render(current_board, current_block); //yee
            T.render(current_board, current_block);
            usleep(0.05 * one_second);
            if(total_line>=500 * some_index){
                time_2 = clock();
                printf("\ntotal lines: %d, %f sec(s) from start\n", 500 * some_index, (time_2-time_1)/1000000.0);
                some_index++;
            }
            if(total_line>Max_Line)gameOver=1;
        }     
    }
};


void player_mode(){
    Tetris game;
    debug yeet;
    debug dd;
    //game.create_block_queue( num_of_block_queues, Max_Block );
    
    game.init();
    //dd.debug_board_init( current_board );
    for(int i=0 ; i<MAX_INT && !gameOver ; i++){
        game.handleInput();
        if(i%5==0){
            game.update( current_board, current_block );
            game.render( current_board, current_block);
        }
        usleep(0.1*one_second);
    }
    printf("game over");
}
void ai_play_mode(){
    debug dd;
    Tetris T;
    TetrisAI AI;
    clock_t time_a, time_b;
    printf("initializing...\n");
    usleep(one_second);
    T.init();
    //dd.print_block_queue();
    dd.init_the_yeet();
    dd.debug_board_init(current_board);
    printf("initialize complete\n");
    printf("start\n");
    printf("predict %d block(s)\n", predict_blocks);
    time_a = clock();
    printf("timer start\n");
    T.render( current_board, current_block );
    AI.ai_play();
    time_b = clock();
    printf("timer stop\n");
    printf("predict %d block(s)\n", predict_blocks);
    printf("total time: %f sec\n", (time_b-time_a)/1000000.0);
    printf("total lines: %d lines\n", total_line);
    printf("end\n");
    return;
}
void ai_control_mode(){
    debug dd;
    Tetris T;
    TetrisAI AI;
    printf("initializing...\n");
    usleep(1*one_second);
    T.init();
    T.remove_block_from_board( current_board, current_block );
    dd.init_the_yeet();
    block_queue[0][0] = dd.scan_nextPiece();
    current_block.piece = block_queue[0][0];
    dd.pressKey( kVK_ANSI_C );
    T.render( current_board, current_block );

    //AI.ai_play();
    for(int i=0 ; i<10000 && !gameOver ; i++){
        //dd.get_gameboard(current_board);
        //usleep(one_second);
        
        //usleep(0.2 * one_second);
        block_queue[0][ (current_block.order+1)% Max_Block ] = dd.scan_nextPiece();
        T.remove_block_from_board(current_board, current_block);
        AI.board_copy_paste(current_board, temp_boards[0] );//yee
        AI.ai_decide( current_block, 1 , 0, 0);//must be 1 //yee
        //printf("\n%d, %d\n", best_move.posY, current_block.posY);
        T.move_block( current_board, current_block, current_block.posX, current_block.posY ); //yee
        //printf("\n%d, %d\n", best_move.posY, current_block.posY);
        AI.control_move_block( current_block );// yee
        //printf("\n%d, %d\n", best_move.posY, current_block.posY);
        T.update( current_board, current_block ); //yee
        dd.test_render(current_board, current_block); //yee
        if(i%40 == 0){
            dd.get_gameboard( current_board );
        }
        usleep(0.01 * one_second);
    }
        

    printf(" end\n");
    return;
}
void debug_mode(){//free mode //order
    debug dd;
    Tetris T;
    T.init();
    T.remove_block_from_board(current_board, current_block);
    for(int i=0 ; i<1000 ; i++){
        dd.get_gameboard(current_board);
        T.render( current_board, current_block );
        usleep(0.1 * one_second);
    } 
}

int main(){
    printf("yeet\n");
    //usleep(3 * one_second);
    //debug_mode();
    //player_mode();
    //ai_control_mode();
    ai_play_mode();
    printf(" YEEEEE\n");
    return 0;
}
/*
Taking a break

usleep

cd "/Users/wanda/Desktop/Eason_C++_學習/c++code/project/tetrisA/" && g++ tetrisfixed.cpp -o tetrisfixed -framework CoreFoundation -framework CoreGraphics && "/Users/wanda/Desktop/Eason_C++_學習/c++code/project/tetrisA/"tetrisfixed
*/