//tetris genetic alg ai

#include <chrono>
#include <poll.h> //macOS only
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <random>
#include <queue>
#include <algorithm>
#include <vector>
#include <string>
//#include <windows.h> // windows only

//keys
const int UP_KEY   ='w';
const int DOWN_KEY ='s';
const int LEFT_KEY ='a';
const int RIGHT_KEY='d';
const int DROP_KEY =' ';
const int HOLD_KEY ='h';

//game board
const int height=20+5, width=10+2;
bool board[height][width];

//game variables
bool gameOver=0;
int can_hold=-1;
bool block_lock=0;
int posX, posY, rotation, block_num, hold_block_num;
int total_line, total_blocks;
int block_order[195000];//yeet
int order_pointer = 0;
int lines_clear_at_once[5];

//render variables
const int RPS=10;//amount of keys can be detected in a second
int rend=0;
const int sleep_microseconds = 1000000/RPS;
const int drop_freq=0.5 * RPS;
int frame = 1;

//AI variables
double current_cost=0;
double best_cost=99999;
int best_pos=0;
int best_rot=0;
int best_hld=0;

// Define the population size and the number of genes for each player
const int POPULATION_SIZE = 100;
const int NUM_GENES = 7;
    double hole_cost   = 12;
    double peak_cost   = 17;
    double posX_cost   = -3;
    double bumpy_cost  = 6;
    double tth_cost    = 5;
    double pits_cost   = 13;
    double line_cost   = -5;

const int GENERATION_TARGET = 100;

// Define the mutation rate
const double MUTATION_RATE = 0.02;
int generation_times=0;

//Temp vector for genes
std::vector<double> tmpv(NUM_GENES);


struct THE_POPULATION{
    THE_POPULATION() : genes(NUM_GENES) {}
    int fitness;
    std::vector<double> genes;
};
// Generate the initial population of players
std::vector<THE_POPULATION> population(POPULATION_SIZE);

const std::string debug_board[20]={
    "0..........|",
    "9..........|",
    "8..........|",
    "7..........|",
    "6..........|",
    "5..........|",
    "4###.##.###|",
    "3.##..#####|",
    "2.##..#####|",
    "1.##.######|",
    "0.#...##..#|",
    "9.##.#.##.#|",
    "8####.#.###|",
    "7.#.###..##|",
    "6####.##.##|",
    "5.#########|",
    "4.##.##..##|",
    "3##..#.###.|",
    "2#..#####.#|",
    "1####.#.#..|"
};

//macOS functions
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
    void debug_board_init(){
        for(int i=0 ; i<=21 ; i++){
            for(int j=1 ; j<width-1 ; j++){
                if(debug_board[i][j] == '#'){
                    board[i+4][j]=1;
                }
            }
        }
    }
    void init() {
        //printf("intializing...\n");
        // Initialize the game state and set up the playfield
        gameOver=0;
        posX=0; posY=0; rotation=0;
        can_hold=-1;
        hold_block_num=-1;
        total_blocks=0;
        total_line=0;
        order_pointer=0;
        for(int i=0 ; i<5 ; i++){
            lines_clear_at_once[i]=0;
        }
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
        debug_board_init();
        new_block();
        hold_block();//for ai
        //render();
        //printf("initialize complete\n");
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

        //printf("\x1b[2J"); //clear the screen
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
        printf("single: %d\ndouble: %d\ntriple: %d\ntetris: %d\n", lines_clear_at_once[1], lines_clear_at_once[2], lines_clear_at_once[3], lines_clear_at_once[4] );
        printf("total line: %d\ntotal blocks: %d\n", total_line, total_blocks);
        
    }

private:
    // Game state variables, such as the playfield and the current position of the falling tetromino
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
    int check_line(){
        int c_line_count=0;
        for(int line_num=height-2 ; line_num>=0 ; line_num--){
            bool isline = 1;
            for(int j=1 ; j<width-1 && isline ; j++){
                isline&=board[line_num][j];
            }
            if(isline){
                c_line_count++;
                total_line++;
                all_move_down(line_num);
                line_num++;
            }
        }
        return c_line_count;
    }
   
    void check_game_over(){
        for(int i=1 ; i<width-1 ; i++){
            gameOver |= board[3][i];
        }
    }

};

Tetris T;

bool cmp_test(THE_POPULATION p1, THE_POPULATION p2){
    for(int i=0 ; i<4 ; i++){
        if( p1.genes[i]<p2.genes[i] ) return 1;
        if( p1.genes[i]>p2.genes[i] ) return 0;
    }
    return p1.fitness<p2.fitness;
}
bool cmp(THE_POPULATION p1, THE_POPULATION p2){
    return p1.fitness<p2.fitness;
}

class TetrisAI{
public:
    void test_block(int t_block, int is_hold){
        Tetris T;
        int tmp_t_block = block_num;
        block_num = t_block;
        for(int r=0 ; r<4 ; r++){    
            T.block_init();
            rotation=r;
            
            for(int i=-1 ; i<width ; i++){
                //printf("rotation: %d\n", rotation);
                T.remove_current_block_from_board();
                if( !T.block_can_move(0, i) ) continue;
                T.move_block( 0, i );
                T.hard_drop();

                current_cost=get_cost();
                if(current_cost<best_cost){
                    best_cost=current_cost;
                    best_pos=posY;
                    best_rot=rotation;
                    best_hld=is_hold;
                }

                //render();
                //printf("x: %d, y: %d\n", posX, posY);
                //printf("current cost: %d\nbest cost: %d\n", current_cost, best_cost );
                //T.remove_current_block_from_board();
                //render();
            }
        }
        T.remove_current_block_from_board();
        T.block_init();
        block_num = tmp_t_block;
        return;
    }
    void play(){
        Tetris T;
        best_cost=99999;
        best_rot=0;
        best_pos=0;
        best_hld=0;
        test_block(block_num, 0);
        if(can_hold>0){
            test_block(hold_block_num, 1);
        }

        if(best_hld)T.hold_block();
        T.block_init();
        rotation=best_rot;
        T.move_block(posX, best_pos);
        T.hard_drop();  
    }

    int evaluateFitness(THE_POPULATION ef){
        total_line=0;
        total_blocks=0;
        hole_cost   = ef.genes[0];
        peak_cost   = ef.genes[1];
        posX_cost   = ef.genes[2];
        bumpy_cost  = ef.genes[3];
        tth_cost    = ef.genes[4];
        pits_cost   = ef.genes[5];
        line_cost   = ef.genes[6];
        T.init();
        for(int i=0 ; i<2147483647 && !gameOver; i++){
            play();
            if(total_blocks>=1000)gameOver=1;
            T.update();
        }
        play();

        return total_line;
    }
    int rouletteWheelSelection(){    
        //return POPULATION_SIZE-1;
        return random_int(POPULATION_SIZE*0.3, POPULATION_SIZE-1);
    }
    std::vector<double> crossover( const std::vector<double> &cov1,  const std::vector<double> &cov2){
        for(int i=0 ; i<NUM_GENES ; i++){
            double random_num = random_double(0, 1000)/1000;
            if(random_num<0.334){
                tmpv[i]=cov1[i];
                continue;
            }else if(random_num<0.667){
                tmpv[i]=( cov1[i]+cov2[i] )/2;
                continue;
            }else{
                tmpv[i]=cov2[i];
            }
        }
        return tmpv;
    }
    void mutate(std::vector<double> &mv, double mr){
        for(int i=0 ; i<NUM_GENES ; i++){
            double random_num = random_double(0, 1);
            if(random_num < mr){
                mv[i]=random_double(-5000, 5000)/1000;
            }
            random_num = random_double(0, 1);
            if(random_num<0.33){
                mv[i]*=0.9;
            }else if(random_num<0.66){
                mv[i]*=1.11;
            }
        }
        return;
    }
  
    void geneticAlgorithmTest(){
        // Generate the initial population of players
        for (int i = 0; i < POPULATION_SIZE; i++) {
            for (int j = 0; j < NUM_GENES; j++) {
                double random_num=random_double(-5000, 5000)/1000;
                population[i].genes[j] = random_num; // Each gene can take values from 0 to 3
            }
        }
        std::vector< std::vector<double> > parents(POPULATION_SIZE/2, std::vector<double>(NUM_GENES));
        std::vector< std::vector<double> > nextGeneration(POPULATION_SIZE, std::vector<double>(NUM_GENES));
        // Main loop of the genetic algorithm
        T.generate_block_order(190000);
        while (generation_times < GENERATION_TARGET) {
            //printf("\n\nGeneration %d (out of %d):\n", generation_times, GENERATION_TARGET);
            generation_times++;
            
            // Evaluate the fitness of each player

            for (int i = 0; i < POPULATION_SIZE; i++) {
                T.init();
                population[i].fitness = evaluateFitness(population[i]); // TODO: Implement the fitness function
            }

            // Select the best players to be the parents of the next generation
            std::sort(population.begin(), population.end(), cmp);
            for(int i=POPULATION_SIZE-10 ; i<POPULATION_SIZE ; i++){
                // for(int j=0 ; j<NUM_GENES ; j++){
                //     printf("%f, ", population[i].genes[j] );
                // }
                printf("%d \n", population[i].fitness );
            }
            for (int i = 0; i < POPULATION_SIZE/2; i++) {
                int parent1 = rouletteWheelSelection();
                int parent2 = rouletteWheelSelection();
                copy_paste(
                    crossover(population[parent1].genes, population[parent2].genes),
                    parents[i]
                );
            }

            // Apply mutation to the offspring
            for (int i = 0; i < POPULATION_SIZE/2; i++) {
                mutate(parents[i], MUTATION_RATE); // TODO: Implement the mutation function
            }

            // Create the next generation by combining the offspring with the best players from the previous generation
            for (int i = 0; i < POPULATION_SIZE/2; i++) {
                copy_paste(
                    population[POPULATION_SIZE-i-1].genes,
                    nextGeneration[i]
                );
                // printf("%d", POPULATION_SIZE-i-1);
                // for(int j=0 ; j<5 ; j++){
                //     printf(" %f,", population[POPULATION_SIZE-i-1].genes[j]);
                // }printf("\n");
                // for(int j=0 ; j<5 ; j++){
                //     printf("%f, ", nextGeneration[i][j]);
                // }printf("\n");
                copy_paste(
                    parents[i],
                    nextGeneration[i+POPULATION_SIZE/2]
                );
            }

            
            // for(int i=0 ; i<POPULATION_SIZE ; i++){
            //     printf("nxt %f, %f, %f, %f, %f\n", nextGeneration[i][0], nextGeneration[i][1], nextGeneration[i][2], nextGeneration[i][3], nextGeneration[i][4]);
            // }

            // Replace the current population with the next generation
            for(int i=0 ; i<POPULATION_SIZE ; i++){
                copy_paste(
                    nextGeneration[i],
                    population[i].genes
                );
            }
            printf("\n");
            // for(int i=0 ; i<POPULATION_SIZE ; i++){
            //     printf("%f, %f, %f, %f, %f\n", population[i].genes[0], population[i].genes[1], population[i].genes[2], population[i].genes[3], population[i].genes[4]);
            // }
        }
        return;
    }
private:
    double random_double(double rda, double rdb){
        std::random_device rd;
        std::mt19937 yee( rd() );
        std::uniform_int_distribution<double> dis(rda, rdb);
        return dis(yee);
    }
    int random_int(int ria, int rib){
        std::random_device rd;
        std::mt19937 yee( rd() );
        std::uniform_int_distribution<int> dis(ria, rib);
        return dis(yee);
    }
    int ABS(int ABS_n){
        return ABS_n<0? -ABS_n:ABS_n; 
    }
    int get_hole(){
        int attribute_1_score=0;
        for(int i=1 ; i<width-1 ; i++){
            int j=1;
            for( ; j<height ; j++){
                if(board[j][i])break;
            }
            j++;
            for( ; j<height ; j++){
                if(!board[j][i]){
                    attribute_1_score++;
                }
            }
        }
        return attribute_1_score;
    }
    int get_peak(){
        int cur_p = height+1000;
        for(int i=3 ; i<height ; i++){
            for(int j=1 ; j<width-1 ; j++){
                if(board[i][j]){
                    return cur_p;
                }     
            }
            cur_p = height-i-2;
        }
        return cur_p;
    }
    int get_bumpy(){
        int bumpy = 0, b_left=0, b_right=0;
        for(int i=1 ; i<width-1 ; i++){
            b_left=b_right;
            for(int j=0 ; j<height-1 ; i++){
                if(board[i][j]){
                    b_right=j;
                    break;
                }
            }
            if(i==1)continue;
            bumpy+=ABS(b_right-b_left);
        }
        return bumpy;
    }
    int get_total_h(){
        int total_h=0, cur_h=0;
        for(int i=1 ; i<width-1 ; i++){
            for(int j=3 ; j<height ; j++){
                if(board[j][i]){
                    cur_h = height-j-1;
                    break;
                }
            }
            total_h+=cur_h;
        }
        return total_h;
    }
    int get_pits(){
        int pits=width-1;
        for(int i=1 ; i<width-1 ; i++){
            for(int j=3 ; j<height-1 ; j++){
                if(board[j][i]){
                    pits--;
                    break;
                }
            }
            
        }
        return pits;
    }
    int get_lines(){
        bool isline=1;
        int line_count=0;
        for(int i=posX ; i<posX+5 && i<height-1 ; i++){
            isline=1;
            for(int j=1 ; j<width-1 ; j++){
                isline&=board[i][j];
            }
            line_count +=isline;
        }
        return line_count;
    }

    double get_cost(){
        double total_cost=0;
        //printf("ls: %d\n", get_lines() );
        total_cost = 
            get_hole()   * hole_cost +
            get_peak()   * peak_cost +
            posX         * posX_cost +
            get_bumpy()  * bumpy_cost+
            get_total_h()* tth_cost  +
            get_pits()   * pits_cost +
            get_lines()  * line_cost;
        return total_cost;
    }

    void copy_paste(const std::vector<double> &v1, std::vector<double> &v2){
        for(int i=0 ; i<v1.size() && i<v2.size() ; i++){
            v2[i]=v1[i];
        }
        return;
    }
};

int main(){
    Tetris game;
    TetrisAI AI;
    //game.generate_block_order(49000);
    game.init();
    printf("\x1b[2J"); //clear the screen
    AI.geneticAlgorithmTest();

    return 0;
}
/* 17,000,000 updates per sec (without ai, height 10000)*/

/*
▩
═

deep rl
演化計算 evolutionary algorithm
ieee cog (?)

3.158500, 3.395625, -1.584000, 4.327000, 3.339000
2.077000, 3.866000, -1.584000, -4.526000, 4.351000
2.153000, 1.862000, -1.853000, -1.700000, 4.511000 fts: 1035
3.157000, 1.761000, -1.822000, -0.691000, 2.830000, 1.183000, -3.265000, ft: 832
4.790000, 3.196800, -3.514500, 2.125800, 4.350600, 3.052000, -1.225000, ft: 2484

1.221000, 4.209000, -3.609000, -0.860000, 3.404700, 1.343925, -0.541000, ft: 19195
1.221000, 4.209000, -3.609000, -0.860000, 3.404700, 1.343925, -0.541000, ft: 19199
1.868625, 1.748223, -3.743000, 2.180668, 2.406600, 0.943628, -3.624588, ft: 14745
2.560050, 1.015200, -3.937500, 1.048000, 2.651400, 2.540160, -2.493000, ft: 19199

4.504000, 0.131000, -3.677000, 0.031000, 1.658000, -0.111000, -2.217000, 409 
4.443000, 3.078000, -4.032180, 3.863700, -0.287000, 3.094200, -4.944000, 410 
1.234000, 1.642000, -3.716000, -1.341000, 4.048000, 1.106000, -1.215000, 410 
*/