#ifndef _DB_MAP_H_
#define _DB_MAP_H_

namespace db {
class SiteMap {
private:
    int nx = 0;
    int ny = 0;
    unsigned char** sites = nullptr;
    unsigned char** regions = nullptr;

public:
    static const char SiteBlocked = 1; // nothing can be placed in the site
    static const char SiteM2Blocked = 2; // any part of it is blocked by M2 metal
    static const char SiteM3Blocked = 4; // any part of it is blocked by M3 metal
    int siteL, siteR;
    int siteB, siteT;
    int siteStepX, siteStepY;
    int siteNX, siteNY;

    long long nSites = 0;
    long long nPlaceable = 0;
    vector<long long> nRegionSites;

    void initSiteMap(unsigned nx, unsigned ny);

    void setSiteMap(const int x, const int y, const unsigned char property) { setBit(sites[x][y], property); }
    void unsetSiteMap(const int x, const int y, const unsigned char property) { unsetBit(sites[x][y], property); }
    bool getSiteMap(int x, int y, unsigned char property) const { return (getBit(sites[x][y], property) == property); }
    unsigned char getSiteMap(int x, int y) const { return sites[x][y]; }

    void getSiteBound(int x, int y, int& lx, int& ly, int& hx, int& hy) const;

    void blockRegion(int x, int y);
    void setRegion(const int x, const int y, const unsigned char region) { regions[x][y] = region; }
    unsigned char getRegion(int x, int y) const { return regions[x][y]; }

    void setSites(int lx, int ly, int hx, int hy, unsigned char property);
    void unsetSites(int lx, int ly, int hx, int hy, unsigned char property);
    void blockRegion(int lx, int ly, int hx, int hy);
    void setRegion(int lx, int ly, int hx, int hy, unsigned char region);
};
}

#endif

