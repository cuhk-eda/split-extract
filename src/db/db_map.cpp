#include "db.h"
using namespace db;

#include "../ut/utils.h"

/***** SiteMap *****/

void SiteMap::initSiteMap(unsigned nx, unsigned ny) {
    this->nx = nx;
    this->ny = ny;

    sites = (unsigned char**)malloc(sizeof(unsigned char*) * nx);
    sites[0] = (unsigned char*)calloc(nx * ny, sizeof(unsigned char));
    for(unsigned x=1; x<nx; x++){
        sites[x] = sites[x-1] + ny;
    }

    regions = (unsigned char**)malloc(sizeof(unsigned char*) * nx);
    regions[0] = (unsigned char*)calloc(nx * ny, sizeof(unsigned char));
    for(unsigned x=1; x<nx; x++){
        regions[x] = regions[x-1] + ny;
    }
}

void SiteMap::getSiteBound(int x, int y, int &lx, int &ly, int &hx, int &hy) const {
    lx = siteL + x * siteStepX;
    ly = siteB + y * siteStepY;
    hx = lx + siteStepX;
    hy = ly + siteStepY;
}

void SiteMap::blockRegion(int x, int y) {
    regions[x][y] = Region::InvalidRegion;
}

void SiteMap::setSites(int lx, int ly, int hx, int hy, unsigned char property) {
    unsigned slx = binOverlappedL(lx, siteL, siteR, siteStepX);
    unsigned sly = binOverlappedL(ly, siteB, siteT, siteStepY);
    unsigned shx = binOverlappedR(hx, siteL, siteR, siteStepX);
    unsigned shy = binOverlappedR(hy, siteB, siteT, siteStepY);
    for (unsigned x = slx; x <= shx; ++x) {
        for (unsigned y = sly; y <= shy; ++y) {
            setSiteMap(x, y, property);
        }
    }
}

void SiteMap::unsetSites(int lx, int ly, int hx, int hy, unsigned char property) {
    unsigned slx = binOverlappedL(lx, siteL, siteR, siteStepX);
    unsigned sly = binOverlappedL(ly, siteB, siteT, siteStepY);
    unsigned shx = binOverlappedR(hx, siteL, siteR, siteStepX);
    unsigned shy = binOverlappedR(hy, siteB, siteT, siteStepY);
    for (unsigned x = slx; x <= shx; ++x) {
        for(unsigned y = sly; y <= shy; ++y) {
            unsetSiteMap(x, y, property);
        }
    }
}

void SiteMap::blockRegion(int lx, int ly, int hx, int hy) {
    unsigned slx = binOverlappedL(lx, siteL, siteR, siteStepX);
    unsigned sly = binOverlappedL(ly, siteB, siteT, siteStepY);
    unsigned shx = binOverlappedR(hx, siteL, siteR, siteStepX);
    unsigned shy = binOverlappedR(hy, siteB, siteT, siteStepY);
    for (unsigned x = slx; x <= shx; ++x) {
        for (unsigned y = sly; y <= shy; ++y) {
            setRegion(x, y, Region::InvalidRegion);
        }
    }
}

void SiteMap::setRegion(int lx, int ly, int hx, int hy, unsigned char region) {
    unsigned slx = binContainedL(lx, siteL, siteR, siteStepX);
    unsigned sly = binContainedL(ly, siteB, siteT, siteStepY);
    unsigned shx = binContainedR(hx, siteL, siteR, siteStepX);
    unsigned shy = binContainedR(hy, siteB, siteT, siteStepY);
    for (unsigned x = slx; x <= shx; ++x) {
        for (unsigned y = sly; y <= shy; ++y) {
            setRegion(x, y, region);
        }
    }
}

