﻿#include <iostream>
#include <cstdlib>
#include <ctime>
#include <stdio.h>
#include <termios.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "colors.h"
#include "Matrix.h"

using namespace std;

/**************************************************************/
/**************** Linux System Functions **********************/
/**************************************************************/

char saved_key = 0;
int tty_raw(int fd);	/* put terminal into a raw mode */
int tty_reset(int fd);	/* restore terminal's mode */
  
/* Read 1 character - echo defines echo mode */
char getch() {
  char ch;
  int n;
  while (1) {
    tty_raw(0);
    n = read(0, &ch, 1);
    tty_reset(0);
    if (n > 0)
      break;
    else if (n < 0) {
      if (errno == EINTR) {
        if (saved_key != 0) {
          ch = saved_key;
          saved_key = 0;
          break;
        }
      }
    }
  }
  return ch;
}

void sigint_handler(int signo) {
  // cout << "SIGINT received!" << endl;
  // do nothing;
}

void sigalrm_handler(int signo) {
  alarm(1);
  saved_key = 's';
}

void registerInterrupt() {
  struct sigaction act, oact;
  act.sa_handler = sigint_handler;
  sigemptyset(&act.sa_mask);
#ifdef SA_INTERRUPT
  act.sa_flags = SA_INTERRUPT;
#else
  act.sa_flags = 0;
#endif
  if (sigaction(SIGINT, &act, &oact) < 0) {
    cerr << "sigaction error" << endl;
    exit(1);
  }
}

void registerAlarm() {
  struct sigaction act, oact;
  act.sa_handler = sigalrm_handler;
  sigemptyset(&act.sa_mask);
#ifdef SA_INTERRUPT
  act.sa_flags = SA_INTERRUPT;
#else
  act.sa_flags = 0;
#endif
  if (sigaction(SIGALRM, &act, &oact) < 0) {
    cerr << "sigaction error" << endl;
    exit(1);
  }
  alarm(1);
}

/**************************************************************/
/**************** Tetris Blocks Definitions *******************/
/**************************************************************/
#define MAX_BLK_TYPES 7
#define MAX_BLK_DEGREES 4

int T0D0[] = { 1, 1, 1, 1, -1 };
int T0D1[] = { 1, 1, 1, 1, -1 };
int T0D2[] = { 1, 1, 1, 1, -1 };
int T0D3[] = { 1, 1, 1, 1, -1 };

int T1D0[] = { 0, 1, 0, 1, 1, 1, 0, 0, 0, -1 };
int T1D1[] = { 0, 1, 0, 0, 1, 1, 0, 1, 0, -1 };
int T1D2[] = { 0, 0, 0, 1, 1, 1, 0, 1, 0, -1 };
int T1D3[] = { 0, 1, 0, 1, 1, 0, 0, 1, 0, -1 };

int T2D0[] = { 1, 0, 0, 1, 1, 1, 0, 0, 0, -1 };
int T2D1[] = { 0, 1, 1, 0, 1, 0, 0, 1, 0, -1 };
int T2D2[] = { 0, 0, 0, 1, 1, 1, 0, 0, 1, -1 };
int T2D3[] = { 0, 1, 0, 0, 1, 0, 1, 1, 0, -1 };

int T3D0[] = { 0, 0, 1, 1, 1, 1, 0, 0, 0, -1 };
int T3D1[] = { 0, 1, 0, 0, 1, 0, 0, 1, 1, -1 };
int T3D2[] = { 0, 0, 0, 1, 1, 1, 1, 0, 0, -1 };
int T3D3[] = { 1, 1, 0, 0, 1, 0, 0, 1, 0, -1 };

int T4D0[] = { 0, 1, 0, 1, 1, 0, 1, 0, 0, -1 };
int T4D1[] = { 1, 1, 0, 0, 1, 1, 0, 0, 0, -1 };
int T4D2[] = { 0, 1, 0, 1, 1, 0, 1, 0, 0, -1 };
int T4D3[] = { 1, 1, 0, 0, 1, 1, 0, 0, 0, -1 };

int T5D0[] = { 0, 1, 0, 0, 1, 1, 0, 0, 1, -1 };
int T5D1[] = { 0, 0, 0, 0, 1, 1, 1, 1, 0, -1 };
int T5D2[] = { 0, 1, 0, 0, 1, 1, 0, 0, 1, -1 };
int T5D3[] = { 0, 0, 0, 0, 1, 1, 1, 1, 0, -1 };

int T6D0[] = { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, -1 };
int T6D1[] = { 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -1 };
int T6D2[] = { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, -1 };
int T6D3[] = { 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -1 };
  
int *setOfBlockArrays[] = {
  T0D0, T0D1, T0D2, T0D3,
  T1D0, T1D1, T1D2, T1D3,
  T2D0, T2D1, T2D2, T2D3,
  T3D0, T3D1, T3D2, T3D3,
  T4D0, T4D1, T4D2, T4D3,
  T5D0, T5D1, T5D2, T5D3,
  T6D0, T6D1, T6D2, T6D3,
};

void drawScreen(Matrix *screen, int wall_depth)
{
  int dy = screen->get_dy();
  int dx = screen->get_dx();
  int dw = wall_depth;
  int **array = screen->get_array();

  for (int y = 0; y < dy - dw + 1; y++) {
    for (int x = dw - 1; x < dx - dw + 1; x++) {
      if (array[y][x] == 0)
	      cout << "□ ";
      else if (array[y][x] == 1)
	      cout << "■ ";
      else if (array[y][x] == 10)
	      cout << "◈ ";
      else if (array[y][x] == 20)
	      cout << "★ ";
      else if (array[y][x] == 30)
	      cout << "● ";
      else if (array[y][x] == 40)
	      cout << "◆ ";
      else if (array[y][x] == 50)
	      cout << "▲ ";
      else if (array[y][x] == 60)
	      cout << "♣ ";
      else if (array[y][x] == 70)
	      cout << "♥ ";
      else
	      cout << "X ";
    }
    cout << endl;
  }
}
  
/**************************************************************/
/******************** Tetris Main Loop ************************/
/**************************************************************/

#define SCREEN_DY  10
#define SCREEN_DX  10
#define SCREEN_DW  4//벽의 두께

#define ARRAY_DY (SCREEN_DY + SCREEN_DW)
#define ARRAY_DX (SCREEN_DX + 2*SCREEN_DW)

int arrayScreen[ARRAY_DY][ARRAY_DX] = {
  { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },//0번째 줄
  { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },  
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },  
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },  
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },  
};

////4. Full line 삭제
////4-1. Down키 입력시 충돌있고, full line들이 발견되면 해당 line들을 배경화면에서 삭제.
////4-1-1. Matrix class의 sum, clip, paste 매서드를 이용하고, screen 배열의 검색범위를 줄이는 것의 유의
////how to do? full line이 생기면 해당 라인을 삭제하고, 위의 라인들을 밑으로 한 줄씩 밀기.

//Logic
/*for ARRAY_DY-4:
    테트리스 한 줄씩 검사
    if 줄이 모두 1인가?:
      해당 라인 삭제
      for 0 to lineNum:
        한 줄씩 복사해서 붙여넣기(삭제한 라인의 윗 라인들을 밑으로 한 줄씩 밀기)
      break;
    else:
      break;*/

void deleteFullLines(Matrix *oScreen, Matrix *iScreen){
  for(int y = 0; y < ARRAY_DY-4; y++){
    int top = y;
    int left = SCREEN_DW;
    Matrix *newOscreen = iScreen -> clip(top, left, top+1, left+SCREEN_DX);
    if(newOscreen -> sum() == 10){
      Matrix *newOscreen2 = iScreen -> clip(0,left, top+1, left+SCREEN_DW);
      iScreen -> paste(newOscreen2,1,left);
      delete newOscreen2;
    }
    delete newOscreen;
    // delete iScreen;(하면 안됨)
  }
  return;
}

int main(int argc, char *argv[]) {
  char key;
  int top = 0, left = 8;//주인공 블럭 출발 좌표
  bool newBlockNeeded = false;
  Matrix *setOfBlockObjects[MAX_BLK_TYPES][MAX_BLK_DEGREES];

  setOfBlockObjects[0][0] = new Matrix(T0D0, 2, 2);
  setOfBlockObjects[0][1] = new Matrix(T0D1, 2, 2);
  setOfBlockObjects[0][2] = new Matrix(T0D2, 2, 2);
  setOfBlockObjects[0][3] = new Matrix(T0D3, 2, 2);

  setOfBlockObjects[1][0] = new Matrix(T1D0, 3, 3);
  setOfBlockObjects[1][1] = new Matrix(T1D1, 3, 3);
  setOfBlockObjects[1][2] = new Matrix(T1D2, 3, 3);
  setOfBlockObjects[1][3] = new Matrix(T1D3, 3, 3);
  
  setOfBlockObjects[2][0] = new Matrix(T2D0, 3, 3);
  setOfBlockObjects[2][1] = new Matrix(T2D1, 3, 3);
  setOfBlockObjects[2][2] = new Matrix(T2D2, 3, 3);
  setOfBlockObjects[2][3] = new Matrix(T2D3, 3, 3);

  setOfBlockObjects[3][0] = new Matrix(T3D0, 3, 3);
  setOfBlockObjects[3][1] = new Matrix(T3D1, 3, 3);
  setOfBlockObjects[3][2] = new Matrix(T3D2, 3, 3);
  setOfBlockObjects[3][3] = new Matrix(T3D3, 3, 3);

  setOfBlockObjects[4][0] = new Matrix(T4D0, 3, 3);
  setOfBlockObjects[4][1] = new Matrix(T4D1, 3, 3);
  setOfBlockObjects[4][2] = new Matrix(T4D2, 3, 3);
  setOfBlockObjects[4][3] = new Matrix(T4D3, 3, 3);

  setOfBlockObjects[5][0] = new Matrix(T5D0, 3, 3);
  setOfBlockObjects[5][1] = new Matrix(T5D1, 3, 3);
  setOfBlockObjects[5][2] = new Matrix(T5D2, 3, 3);
  setOfBlockObjects[5][3] = new Matrix(T5D3, 3, 3);

  setOfBlockObjects[6][0] = new Matrix(T6D0, 4, 4);
  setOfBlockObjects[6][1] = new Matrix(T6D1, 4, 4);
  setOfBlockObjects[6][2] = new Matrix(T6D2, 4, 4);
  setOfBlockObjects[6][3] = new Matrix(T6D3, 4, 4);

  srand((unsigned int)time(NULL));

  int blkType = 0;
  blkType = rand() % MAX_BLK_TYPES;
  int blkDegree = 0;
  Matrix *iScreen = new Matrix((int *) arrayScreen, ARRAY_DY, ARRAY_DX);
  Matrix *currBlk = setOfBlockObjects[blkType][blkDegree];
  Matrix *tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
  Matrix *tempBlk2 = tempBlk->add(currBlk);
  delete tempBlk;

  Matrix *oScreen = new Matrix(iScreen);
  oScreen->paste(tempBlk2, top, left);
  delete tempBlk2;
  drawScreen(oScreen, SCREEN_DW);
  delete oScreen;
//방향키
  while ((key = getch()) != 'q') {
    switch (key) {
      case 'a': left--; break;
      case 'd': left++; break;
      case 's': top++; break;
      case 'w': {
        //메모리 누수 X
        blkDegree = (blkDegree+1)%4;
        currBlk = setOfBlockObjects[blkType][blkDegree];   
        break;}
      case ' ': {
        while(1){
          top++;
          tempBlk = iScreen -> clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
          tempBlk2 = tempBlk->add(currBlk);
          delete tempBlk;
          if (tempBlk2 -> anyGreaterThan(1)){
            delete tempBlk2;
            top--;
            break;
          }
          }
          delete tempBlk2;
          break;
      }
      default: cout << "wrong key input" << endl;
    }

    tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
    tempBlk2 = tempBlk->add(currBlk);
    delete tempBlk;
    if (tempBlk2->anyGreaterThan(1)) {
      delete tempBlk2;
      switch (key) {
        case 'a': left++; break;
        case 'd': left--; break;
        case 's': {
          top--; 
          newBlockNeeded = true; 
          break;
        }
        case 'w': {
          blkDegree = (blkDegree+3)%4;//벽과 충돌하면 반대 방향으로 90도 회전.
          currBlk = setOfBlockObjects[blkType][blkDegree];
          break;
        }
        case ' ': break;
    }
    

    if (newBlockNeeded) {
      iScreen = new Matrix(oScreen);
      deleteFullLines(oScreen, iScreen);
      newBlockNeeded = false;
      top = 0;
      left = 8;  
      
      blkType = rand() % MAX_BLK_TYPES;
      blkDegree = 0;
      currBlk = setOfBlockObjects[blkType][blkDegree];
      tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
      tempBlk2 = tempBlk->add(currBlk);
      oScreen = new Matrix(iScreen);
      oScreen->paste(tempBlk2, top, left);
      drawScreen(oScreen, SCREEN_DW);
      delete oScreen;
      delete tempBlk;
      //delete iScreen;(하면안됨)
      delete tempBlk2;
      //delete currBlk;(하면 절대 안됨)
      
    }
  //end of while
    }
    //delete tempBlk2;
    tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
    tempBlk2 = tempBlk->add(currBlk);    
    delete tempBlk;
    oScreen = new Matrix(iScreen);
    oScreen->paste(tempBlk2, top, left);
    delete tempBlk2;
    drawScreen(oScreen, SCREEN_DW);
    // delete oScreen;
  }

  delete iScreen;
  delete tempBlk;
  delete tempBlk2;
  delete oScreen;//(하면 안되는 것 같음. 계속 segmentation Fault뜸)
  delete currBlk;

  //delete setOfBlockObjects
  for(int x=0;x<7;x++){
    for(int y=0;y<4;y++){
      delete setOfBlockObjects[x][y];
    }
  }
  

  cout << "end(nAlloc, nFree) = (" << Matrix::get_nAlloc() << ',' << Matrix::get_nFree() << ")" << endl;  
  cout << "Program terminated!" << endl;

  return 0;
}
