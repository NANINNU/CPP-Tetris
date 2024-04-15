#include <iostream>
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
#define MAX_BLK_TYPES 7 //7가지 블록 타입
#define MAX_BLK_DEGREES 4 

int T0D0[] = { 1, 1, 1, 1, -1 };//-1은 배열의 끝을 표시. 2X2 행렬임.
int T0D1[] = { 1, 1, 1, 1, -1 };//90도 회전
int T0D2[] = { 1, 1, 1, 1, -1 };//180도 회전
int T0D3[] = { 1, 1, 1, 1, -1 };//270도 회전

int T1D0[] = { 0, 1, 0, 1, 1, 1, 0, 0, 0, -1 };//3X3 행렬임.
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
  
int *setOfBlockArrays[] = { //7가지 블록에서 회전한것을 포함한 28가지 블록모양 1차원
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

  for (int y = 0; y < dy - dw + 3; y++) {
    for (int x = dw - 3; x < dx - dw + 3; x++) {
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
#define SCREEN_DW  3

#define ARRAY_DY (SCREEN_DY + SCREEN_DW)
#define ARRAY_DX (SCREEN_DX + 2*SCREEN_DW)

int arrayScreen[ARRAY_DY][ARRAY_DX] = {
  {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
  {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
  {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
  {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
  {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
  {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
  {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
  {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
  {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
  {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1} 
};

int arrayBlk[3][3] = {
  { 0, 1, 0 },
  { 1, 1, 1 },
  { 0, 0, 0 },
};



void deleteFullLines(Matrix *oScreen, Matrix *iScreen){
    for(int y=0;y<SCREEN_DY;y++)
    {
      int top = y;
      int left = SCREEN_DW;

      Matrix *new_tempBlk = iScreen->clip(top,left,top+1,left+SCREEN_DX);
      
      if(new_tempBlk->sum() == 10)
      {       
        Matrix *new_tempBlk2 = iScreen->clip(0,left,top,left+SCREEN_DX);
        iScreen->paste(new_tempBlk2,1,left);
        delete new_tempBlk2;
      }
      delete new_tempBlk;
    }
    return;
}


int main(int argc, char *argv[]) {
  char key;
  int top = 0, left = 6;

  //Matrix A((int*) arrayBlk,3,3);
  //Matrix B(A);
  //Matrix C(A);
  //Matrix D;
  //D=A+B+C;
  //cout<<D<<endl;
  //return 0;


  int idxBlockDegree = 0;
  srand((unsigned int)time(NULL));
  Matrix *setOfBlockObjects[7][4] = {}; //7x4 배열 2차원
  int count(0); //현재 블록 유형의 회전 상태를 나타냄. count를 0으로 초기화 시킨거.
  

  for(int j=0; j<4; j++){
    setOfBlockObjects[0][j] = new Matrix(setOfBlockArrays[count],2,2); //2x2 행렬
    count++; //setOfBlockObjects와 setOfBlockArrays의 차원이 달라서 count++
  }

  for(int i=1; i<6; i++){
    for(int j=0; j<4; j++){
      setOfBlockObjects[i][j] = new Matrix(setOfBlockArrays[count],3,3); //3x3 행렬
      count++;
    }
  }

  for(int j=0; j<4; j++){
    setOfBlockObjects[6][j] = new Matrix(setOfBlockArrays[count],4,4); //4x4 행렬
    count++;
  }
  unsigned int blkType = rand() % MAX_BLK_TYPES; //0부터 6사이의 임의의 정수 저장
  

  Matrix *iScreen = new Matrix((int *) arrayScreen, ARRAY_DY, ARRAY_DX);
  Matrix *currBlk = setOfBlockObjects[blkType][idxBlockDegree];//setOfBlockObjects[blkType][idxBlockDegree] 계속 회전해야하므로 idxBlockDegree를 넣어줌.
  Matrix *tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
  Matrix *tempBlk2 = tempBlk->add(currBlk); //Matrix * tempBlk2로 바꿔줘야 충돌이 안생김.
  delete tempBlk; //tempBlk 쓰임을 다해서 삭제.(코드 누수방지) + 좌표 숫자를 맞추기 위해서 삭제.

  Matrix *oScreen = new Matrix(iScreen);
  oScreen->paste(tempBlk2, top, left);
  delete tempBlk2; //tempBlk2 쓰임을 다해서 삭제. + 좌표 숫자를 맞추기 위해서 삭제.
  drawScreen(oScreen, SCREEN_DW);
  delete oScreen; //oScreen 쓰임을 다해서 삭제. + 좌표 숫자를 맞추기 위해서 삭제.

  while ((key = getch()) != 'q') {
    switch (key) {
      case 'a': left--; break; //왼쪽으로 가려면 left 감소
      case 'd': left++; break; //오른쪽으로 가려면 left 증가
      case 's': top++; break; 
      case 'w':
      { 
        idxBlockDegree = (idxBlockDegree + 1) % 4;
        currBlk = setOfBlockObjects[blkType][idxBlockDegree];//currBlk을 갈아끼우는 과정
        break;
      }
      case ' ': //스페이스바 누르면 currblk 밑으로 추락
      {
        while(true){
          top++;
          tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
          tempBlk2 = tempBlk->add(currBlk);
          delete tempBlk;

          if(tempBlk2->anyGreaterThan(1)){
            delete tempBlk2;
            top--;
            tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
            tempBlk2 = tempBlk->add(currBlk);
            delete tempBlk;
            break;
          }
          delete tempBlk2;
        }
        blkType = rand() % MAX_BLK_TYPES;
        idxBlockDegree = 0;
        currBlk = setOfBlockObjects[blkType][idxBlockDegree];
        iScreen -> paste(tempBlk2, top, left);
        delete tempBlk2;
        deleteFullLines(oScreen, iScreen);
        top = 0;
        left = 6;
        break;
      }
      default: cout << "wrong key input" << endl;
    }
    tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
    tempBlk2 = tempBlk->add(currBlk);
    delete tempBlk; //tempBlk 쓰임을 다해서 삭제.(코드 누수방지) + 좌표 숫자를 맞추기 위해서 삭제.

    if (tempBlk2->anyGreaterThan(1)){ //tempBlk2에 tempBlk가 들어갔는데 1보다 크면, 원래 상태로 바꿔야해서 left, top 값 반대로 바꾸기.
      delete tempBlk2; //tempBlk2 값이 이미 위에서 1보다 크다는 것을 알았으므로 삭제.
      switch (key) {
        case 'a': left++; break; //벽과의 충돌 방지
        case 'd': left--; break; //벽과의 충돌 방지
        case 's': top--; break; //벽과의 충돌 방지
        case 'w': //벽과의 충돌을 막기위해 반대방향으로 돌려서 블럭이 돌아가지 않게 함.
        {
          idxBlockDegree = (idxBlockDegree + 3) % 4;
          currBlk = setOfBlockObjects[blkType][idxBlockDegree];
          break;
        }
        case ' ': break;
        //if(oScreen -> anyGreaterThan(1))
            //return 0; 종료조건.
        default: cout << "wrong key input" << endl;
    }
    tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
    tempBlk2 = tempBlk->add(currBlk);
    delete tempBlk; //tempBlk 쓰임을 다해서 삭제.(코드 누수방지) + 좌표 숫자를 맞추기 위해서 삭제.
  }
  oScreen = new Matrix(iScreen);
  oScreen->paste(tempBlk2, top, left);
  delete tempBlk2; //tempBlk2 쓰임을 다해서 삭제. + 좌표 숫자를 맞추기 위해서 삭제.
  drawScreen(oScreen, SCREEN_DW);
  delete oScreen; //oScreen 쓰임을 다해서 삭제. + 좌표 숫자를 맞추기 위해서 삭제.
  }
  delete iScreen;
  for(int i=0; i<7; i++){ //7x4 배열 2차원 matrix 객체 삭제.
    for (int j=0; j<4; j++){
      delete setOfBlockObjects[i][j];
    }
  }

  cout << "(nAlloc, nFree) = (" << Matrix::get_nAlloc() << ',' << Matrix::get_nFree() << ")" << endl;  
  cout << "Program terminated!" << endl;

  return 0;
}