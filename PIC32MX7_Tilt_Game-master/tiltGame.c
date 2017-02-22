#include "tiltGame.h"

static int board[4][32];
static int ballStatus;
static int curx;
static int cury;
static int tilt;


int getX(){
  return curx/2;
}


int getY(){
	return cury/2;
}


int getBallStatus(){
	return ballStatus;
}


int wasTilted(){
	if(tilt == 1){
		tilt = 0;
		return 1;
	}
	return 0;
}


void initTiltGame(){
	tilt = 0;

	int i,j;
	for(i = 0; i < 4; i++){
		for(j = 0; j < 32; j++){
			board[i][j]=0;
		}
	}

	board[2][0] = 2; //2 = hole
	board[3][0] = 2;
	board[2][1] = 2;
	board[3][1] = 2;

	curx=31;
	cury=0;
	board[cury][curx] = 1;

	ballStatus = 2; //ball 2 at start
}


void tilted(int x, int y){

	if(x < 0){
		if(curx < 31){
			board[curx][cury] = 0;
			curx++;
			board[curx][cury] = 1;

			if(ballStatus == 1){
				ballStatus = 2;
			}else if(ballStatus == 2){
				ballStatus = 1;
			}else if(ballStatus == 3){
				ballStatus = 4;
			}else if(ballStatus == 4){
				ballStatus = 3;
			}

			tilt=1;
		}
	}else if(x > 0){
		if(curx > 0){
			board[curx][cury] = 0;
			curx--;
			board[curx][cury] = 1;

			if(ballStatus == 1){
				ballStatus = 2;
			}else if(ballStatus == 2){
				ballStatus = 1;
			}else if(ballStatus == 3){
				ballStatus = 4;
			}else if(ballStatus == 4){
				ballStatus = 3;
			}

			tilt=1;
		}
	}

	if(y < 0){
		if(cury > 0){
			board[curx][cury] = 0;
			cury--;
			board[curx][cury] = 1;

			if(ballStatus == 1){
				ballStatus = 3;
			}else if(ballStatus == 3){
				ballStatus = 1;
			}else if(ballStatus == 2){
				ballStatus = 4;
			}else if(ballStatus == 4){
				ballStatus = 2;
			}

			tilt=1;
		}
	}if(y > 0){
		if(cury < 3){
			board[curx][cury] = 0;
			cury++;
			board[curx][cury] = 1;

			if(ballStatus == 1){
				ballStatus = 3;
			}else if(ballStatus == 3){
				ballStatus = 1;
			}else if(ballStatus == 2){
				ballStatus = 4;
			}else if(ballStatus == 4){
				ballStatus = 2;
			}

			tilt=1;
		}
	}
}


int hasWon(){
	if(curx/2 == 0 && cury/2 == 1){
		board[cury][curx] = 2;
		curx=31;
		cury=0;
		board[cury][curx] = 1;
		return 1;
	}
	return 0;
}
