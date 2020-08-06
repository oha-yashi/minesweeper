#include <ncurses.h>
#include <algorithm>
#include <vector>
template<typename T> using vector = std::vector<T>;
using vi = vector<int>;
using vvi = vector<vi>;
#include <random>
#include <chrono>
namespace chrono = std::chrono;

#define RATIO 8 //難易度を調整する
//#define SELF //自分で地雷の数を決める

int window_height, window_width; //ウィンドウのサイズ
int size_of_field; //縦横幅
int mine_num; //地雷数
int closing_num; //閉鎖されているマス数
int flag_num; //フラグ立てたマス数
int H = 0, W = 0; //現在地

vvi field; //f>=0 数字 f<0 爆弾
vvi cover; //1:隠蔽 0:開放 -1:閉鎖

const int dh[8] = {0,0,1,-1, 1,1,-1,-1}; //縦横, 斜め
const int dw[8] = {1,-1,0,0, 1,-1,1,-1};

namespace /* in_range.cpp */ {
  template<class T>
  bool is_range_in(T l, T x, T r){
    /* l <= x < r */
    return l <= x && x < r;
  }
}

int select_difficulty(){
  mvaddstr(0,0,"start game! select difficulty!");
  //window_height-3 で引いているのは説明行の分
  int can_select = std::min({ (window_height-3)/RATIO ,window_width/(RATIO*3), 4 });

  if(can_select == 0){
    mvaddstr(window_height-2,0,"window is too small!\n");
    return 0;
  }
  mvprintw(1,0,"you can select 1 ~ %d", can_select);
  mvprintw(2,0,"1. small  - %d*%d = %d", RATIO, RATIO, RATIO*RATIO);
  mvprintw(3,0,"2. normal - %d*%d = %d", RATIO*2, RATIO*2, RATIO*2*RATIO*2);
  mvprintw(4,0,"3. large  - %d*%d = %d", RATIO*3, RATIO*3, RATIO*3*RATIO*3);
  mvprintw(5,0,"4. extra  - %d*%d = %d", RATIO*4, RATIO*4, RATIO*4*RATIO*4);
  char ch = getch();
  //mvprintw(5,0,"ch = %c\n", ch);
  while(!is_range_in('1', ch, (char)('1'+can_select))){

    mvprintw(5,0,"select 1 ~ %d!!", can_select);
    //mvprintw(5,0,"ch = %c\n", ch);
    ch = getch();
  }

  return ch - '0';
}

int set_mine(int diff){
  std::random_device rand;
  //地雷数決定
  int num = size_of_field*size_of_field/9;
  switch(diff){
    case 1: break;
    case 2: num += rand()%4; break;
    case 3: num += rand()%8; break;
    case 4: num += rand()%16; break;
  }

  #ifdef SELF
    mvprintw(6,0, "how many Mines? -> ");
    echo();
    
    while(scanw("%d", &num)!=1){
      clrtoeol();
    }
    noecho();
  #endif

  int q = num;

  while(q){
    int h = rand()%size_of_field, w = rand()%size_of_field;
    if(field[h][w]<0)continue;

    field[h][w] = -99;
    for(int k=0; k<8; k++){
      int nh = h + dh[k], nw = w + dw[k];
      if(!is_range_in(0,nh,size_of_field) || !is_range_in(0,nw,size_of_field))continue;
      field[nh][nw]++;
    }
    q--;
  }

  return num;
}

void show_field(int now_H, int now_W){
  for(int i=0; i<size_of_field; i++){
    for(int j=0; j<size_of_field; j++){
      bool isNow = (i==now_H && j==now_W);
      move(i, j*3);
      if(cover[i][j] == 1){ //隠蔽
        printw(isNow ? "$#$" : " # ");
      }else if(cover[i][j] == 0){ //開放

        if(field[i][j]<0){ //爆発
          attron(COLOR_PAIR(2));
          printw(">X<");
          attron(COLOR_PAIR(1));
        }else if(field[i][j]==0)printw(isNow ? "$.$" : " . "); //空き
        else printw(isNow ? "$%d$" : " %d ", field[i][j]); //数字

      }else if(cover[i][j]==-1){ //フラグ
        attron(COLOR_PAIR(2));
        printw(isNow ? "$F$" : " F ");
        attron(COLOR_PAIR(1));
      }
    }
  }
}

void open_bfs(int h, int w){
  for(int k=0; k<8; k++){
    int nh = h + dh[k], nw = w + dw[k];
    if(!is_range_in(0,nh,size_of_field)||!is_range_in(0,nw,size_of_field))continue;
    if(cover[nh][nw]==0)continue;

    cover[nh][nw] = 0;
    closing_num--;
    if(field[nh][nw]==0){
      open_bfs(nh, nw);
    }
  }
}

int game(int &H, int &W){
  show_field(H, W);

  char ch = getch();

  int rtn = 0;
  switch(ch){
    case 'a' : if(W!=0) W--; break;
    case 'w' : if(H!=0) H--; break;
    case 's' : if(H!=size_of_field-1) H++; break;
    case 'd' : if(W!=size_of_field-1) W++; break;

    case 'j' : //開放
      if(cover[H][W] == 0 || cover[H][W] == -1)break; //開放済みor閉鎖

      cover[H][W] = 0; closing_num--;
      if(field[H][W] == 0){//空白
        open_bfs(H,W);
        if(closing_num == mine_num)rtn = 1;
      }else if(field[H][W] > 0){//数字
        if(closing_num == mine_num)rtn = 1;
      }else{//地雷
        rtn = -1;
      }
    break;

    case 'k' : //フラグ操作
      if(cover[H][W] == 1){
        flag_num++;
        cover[H][W] = -1;
      }else if(cover[H][W] == -1){
        flag_num--;
        cover[H][W] = 1;
      }
    break;

    case '@' : //終了
      rtn = -1;
  }
  return rtn;
}

int main(void){
  initscr(); cbreak(); noecho();
  start_color();
  init_pair(1, COLOR_BLACK, COLOR_WHITE);
  init_pair(2, COLOR_RED,   COLOR_WHITE);
  bkgd(COLOR_PAIR(1));
  chrono::system_clock::time_point start_time, end_time;

START:
  clear();
  H = 0; W = 0; flag_num = 0;
  getmaxyx(stdscr, window_height, window_width);

  int diff = select_difficulty();
  if(diff == 0)goto END;
  size_of_field = diff * RATIO;
  closing_num = size_of_field * size_of_field;

  field.assign(size_of_field, vi(size_of_field, 0));
  cover.assign(size_of_field, vi(size_of_field, 1));

  //地雷セット
  mine_num = set_mine(diff);

  //操作説明
  clear();
  mvprintw(window_height-1, 0, "[<A][W^][Sv][D>] [J:open][K:flag][@:quit]");
  
  //ゲームループ
  start_time = chrono::system_clock::now();
  //初めの一手自動開放
  while(true){
    int h = rand()%size_of_field, w = rand()%size_of_field;
    if(field[h][w]!=0)continue;
    cover[h][w] = 0; closing_num--;
    open_bfs(h,w);
    break;
  }
  init_pair(2, COLOR_RED, COLOR_WHITE);
  while(true){
    end_time = chrono::system_clock::now();
    mvprintw(window_height-3, 0, "Time = %.1lf s", chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count()/1000.0);
    mvprintw(window_height-2, 0, "Mines:%d Flaged:%d All:%d", mine_num, flag_num, size_of_field*size_of_field);
    int g = game(H, W);
    if(g == 0){
      continue;
    }else if(g == 1){
      mvprintw(window_height-2, 0, "good end!");clrtoeol();
      init_pair(2, COLOR_GREEN, COLOR_WHITE);
      //good_end();
      break;
    }else if(g == -1){
      mvprintw(window_height-2, 0, "bad end!");clrtoeol();
      //bad_end();
      break;
    }
  }
  
  cover.assign(size_of_field, vi(size_of_field, 0)); //全マス開く
  show_field(H, W);
  

END:
  end_time = chrono::system_clock::now();
  mvprintw(window_height-3, 0, "Time = %.1lf s", chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count()/1000.0);
  mvprintw(window_height-1, 0, "push R to retry, push E to end.");clrtoeol();

HOW_NEXT:
  char ch = getch();
  if(ch == 'r')goto START;
  else if(ch != 'e')goto HOW_NEXT;

  endwin();
}