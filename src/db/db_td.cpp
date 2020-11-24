#include "db.h"
using namespace db;

/***** TDBins *****/
TDBins::TDBins(){
    nx = ny = 0;
    tdBins = NULL;
}

void TDBins::initTDBins(int nx, int ny){
    this->nx = nx;
    this->ny = ny;

    tdBins = (double**)malloc(sizeof(double*) * nx);
    tdBins[0] = (double*)malloc(sizeof(double) * nx * ny);
    for(int x=1; x<nx; x++){
        tdBins[x] = tdBins[x-1] + ny;
    }
    for(int x=0; x<nx; x++){
        for(int y=0; y<ny; y++){
            tdBins[x][y] = 1.0;
        }
    }
}

void TDBins::setTDBin(int x, int y, double density){
    tdBins[x][y] = density;
}

double TDBins::getTDBin(int x, int y){
    return tdBins[x][y];
}

void TDBins::setRegionDensity(int lx, int ly, int hx, int hy, double density){
    int halfBinX = binStepX / 2;
    int halfBinY = binStepY / 2;
    int startX = (lx - binL + halfBinX) / binStepX;
    int startY = (ly - binB + halfBinY) / binStepY;
    int endX = (hx - binL + halfBinX - 1) / binStepX;
    int endY = (hy - binB + halfBinY - 1) / binStepY;
    for(int x=startX; x<=endX; x++){
        for(int y=startY; y<=endY; y++){
            setTDBin(x, y, density);
        }
    }
}
