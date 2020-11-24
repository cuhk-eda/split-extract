#ifndef _DB_TD_H_
#define _DB_TD_H_

namespace db{
    class TDBins{
        private:
            int nx, ny;
            double **tdBins;
        public:
            int binL, binR;
            int binB, binT;
            int binStepX, binStepY;
            int binNX, binNY;

            TDBins();

            void initTDBins(int nx, int ny);
            void setTDBin(int x, int y, double density);
            double getTDBin(int x, int y);
            void setRegionDensity(int lx, int ly, int hx, int hy, double density);
    };
}

#endif

