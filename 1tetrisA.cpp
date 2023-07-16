//方塊下落、隨機方塊
#include<iostream>
#include<unistd.h>//usleep
//#include<stdlib.h>//
#include<random>//generate random num
#include<stdio.h>

//#include<ncurses.h>

using namespace std;

const int ROW=20+2;// height
const int COL=10;// width
const int FPS=24;
const int microseconds=1000000/FPS;
const bool blocks[7][4][4]={// 7 kind of blocks
    {//I
        {0, 0, 0, 0},
        {1, 1, 1, 1},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    },
    {//O
        {0, 0, 0, 0},
        {0, 1, 1, 0},
        {0, 1, 1, 0},
        {0, 0, 0, 0}
    },
    {//L
        {0, 0, 0, 0},
        {0, 0, 0, 1},
        {0, 1, 1, 1},
        {0, 0, 0, 0}
    },
    {//J
        {0, 0, 0, 0},
        {1, 0, 0, 0},
        {1, 1, 1, 0},
        {0, 0, 0, 0}
    },
    {//T
        {0, 1, 0, 0},
        {1, 1, 1, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    },
    {//Z
        {0, 0, 0, 0},
        {0, 1, 1, 0},
        {0, 0, 1, 1},
        {0, 0, 0, 0}
    },
    {//S
        {0, 0, 1, 1},
        {0, 1, 1, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    }
};
bool cur_block[4][4]={
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
};
bool tmp_block[4][4]={
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
};
int posX=0, posY=0;

bool board[ROW+2][COL+2];

void debug(){cout << "hello world\n";return;}
void print_board(){
    system("clear");
    cout << "\n";
    for(int i=3 ; i<ROW+2 ; i++){
        for(int j=0 ; j<COL+2; j++){
            if(board[i][j]) cout <<"0 ";
            else cout << "  ";
            
        }cout << "\n";
    }cout << "\n";
}
void print_cur_block(){// debug
    cout << "\ncur:\n";
    for(int i=0 ; i<4 ; i++){
        for(int j=0 ; j<4 ; j++){
            cout << cur_block[i][j]<<" ";
        }cout << "\n";
    }cout << "\n";
}
void print_tmp_block(){// debug
    cout << "\ntmp:\n";
    for(int i=0 ; i<4 ; i++){
        for(int j=0 ; j<4 ; j++){
            cout << tmp_block[i][j]<<" ";
        }cout << "\n";
    }cout << "\n";
}
// to check if the position is valid
// 0<= posX <= ROW && 1<= posY <= COL
bool position_valid(){
    return (0<=posX && posX<=ROW && 1<=posY && posY<=COL)? 1: 0;
}

void initialize(){
    for(int i=0 ; i<=ROW+1 ; i++){
        for(int j=0 ; j<COL+2 ; j++){
            board[i][j]=1;
        }
    }
    for(int i=0 ; i<=ROW ; i++){
        for(int j=1 ; j<=COL ; j++){
            board[i][j]=0;
        }
    }
}
void block_copy(){
    for(int i=0 ; i<4 ; i++){
        for(int j=0 ; j<4 ; j++){ if( !position_valid() )continue;
            tmp_block[i][j]=cur_block[i][j];
        }
    }
}

bool can_move(int x, int y){
    for(int i=0 ; i<4 ; i++){
        for(int j=0 ; j<4 ; j++){ if( !position_valid() )continue;
            if(board[i+x][j+y] && tmp_block[i][j]){
                return 0;
            }
        }
    }return 1;
}

//move block to x, y or rotate the block
bool move_b(int x, int y){
    // copy cur_block to tmp_block
    block_copy();

    //delete current block on the board
    for(int i=0 ; i<4 ; i++){
        for(int j=0 ; j<4 ; j++){ if( !position_valid() )continue;
            board[i+posX][j+posY]=board[i+posX][j+posY] & ( board[i+posX][j+posY] ^ cur_block[i][j] );
        }
    }
    //cout << "debug:\n";//debug
    //cout <<"x: "<< x << ", y: " << y<<"\n";
    //print_board();
    //print_tmp_block();
    //print_cur_block();
    
    //check if the block can move
    if( !can_move(x, y) ){
        
        //fill the block back
        for(int k1=0 ; k1<4 ; k1++){
            for(int k2=0 ; k2<4 ; k2++){
                board[k1+posX][k2+posY]=board[k1+posX][k2+posY] | cur_block[k1][k2];
            }
        }
        cout << "\n";
        return 0;
    }

    //fill the moved block
    for(int i=0 ; i<4 ; i++){
        for(int j=0 ; j<4 ; j++){ if( !position_valid() )continue;
            board[i+x][j+y]=board[i+x][j+y] | tmp_block[i][j];
        }
    }
    posX=x;posY=y;
    return 1;
}

bool new_block(){
    random_device rd; 
    mt19937 yee(rd()); 
    uniform_int_distribution<> distr(1, 7);
    int n = distr(yee) - 1;// random number from 0~6

    posX=0;
    posY=(COL/2)-1;// middle of width
    for(int i=0 ; i<4 ; i++){
        for(int j=0 ; j<4 ; j++){
            cur_block[i][j]=blocks[n][i][j];
        }
    }
    if( !can_move(posX, posY) ){
        return 0;
    }
    else{
        move_b(posX, posY);
        return 1;
    }
}

int main(){
    initialize(); 
    new_block();
    
    char c;
    for(int frame=0 ;  ; frame++){
        print_board();
        
        //if the block can't drop, then game over
        if( !move_b(posX+1, posY) ){
            if( !new_block() ){
                cout << "GAME OVER\n";
                return 0;
            };
        }
        
        /*
        system("stty raw");
        c=getchar();
        system("stty cooked");
        if ( (int)c == 27 ) {
            getchar(); // skip the [
            switch(getchar()) { // the real value
                case 'A':
                    // code for arrow up
                    //printf("up\n\r");

                    break;
                case 'B':
                    // code for arrow down
                    //printf("down\n\r");
                    move_b(posX+1, posY);
                    break;
                case 'C':
                    // code for arrow right
                    //printf("right\n\r");
                    move_b(posX, posY+1);
                    break;
                case 'D':
                    // code for arrow left
                    //printf("left\n\r");
                    move_b(posX, posY-1);
                    break;
                default: break;
            }
        }
        */
    }
    print_board();

/*
    char c;
    while(1){
        system("stty raw");
        c=getchar();
        system("stty cooked");
        cout << "--" << c << "--\n";
    }
*/
    return 0;
}