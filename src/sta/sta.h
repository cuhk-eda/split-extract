#ifndef _STA_LIB_H_
#define _STA_LIB_H_

#include "../global.h"
//#include "../db/db.h"

namespace sta {

/*****************
  Timing Library
*****************/
class STALibraryLUT {
public:
    bool isScalar = false;
    double value;
    vector<double> indexX;
    vector<double> indexY;
    vector<double> indexZ;
    double minX;
    double maxX;
    double minY;
    double maxY;
    double minZ;
    double maxZ;
    vector<vector<vector<double>>> values;

    STALibraryLUT() {}
    STALibraryLUT(double scalar) : isScalar(true), value(scalar) {}

    void clear();
    //  double get(double x = 0, double y = 0, double z = 0);
    void print();
};

class STALibraryTiming {
public:
    STALibraryLUT delayRise;
    STALibraryLUT delayFall;
    STALibraryLUT slewRise;
    STALibraryLUT slewFall;
    int relatedPin;
    string relatedPinName;
    char timingSense;  //'+' / '-' / 'x'
};

class STALibraryOPin {
private:
    string _name = "";

public:
    int pin = -1;
    double min_capacitance = 0.0;
    double max_capacitance = 0.0;
    vector<STALibraryTiming> timings;

    static double default_max_capacitance;

    STALibraryOPin(const string& name = "") : _name(name) {}
    STALibraryOPin(const STALibraryOPin& opin)
        : _name(opin._name),
          min_capacitance(opin.min_capacitance),
          max_capacitance(opin.max_capacitance),
          timings(opin.timings) {}

    const string& name() const { return _name; }
    void name(const string& s) { _name = s; }
};

class STALibraryIPin {
private:
    string _name = "";

public:
    int pin = -1;
    double capacitance = 0.0;

    static double default_capacitance;

    STALibraryIPin(const string& name = "") : _name(name) {}
    STALibraryIPin(const STALibraryIPin& ipin) : _name(ipin._name), capacitance(ipin.capacitance) {}

    const string& name() const { return _name; }
    void name(const string& s) { _name = s; }
};

class STALibraryCell {
private:
    string _name = "";
    unsigned _drive_strength = 0;

public:
    vector<int> pins;
    // map from FCell pin index to STALibraryCell index
    vector<STALibraryOPin> opins;
    vector<STALibraryIPin> ipins;
    vector<vector<int>> timingArcs;

    STALibraryCell(const string& name = "", const unsigned drive_strength = 0) : _name(name), _drive_strength(drive_strength) {}

    const string& name() const { return _name; }
    unsigned drive_strength() const { return _drive_strength; }

    void name(const string& s) { _name = s; }
    void drive_strength(const unsigned u) { _drive_strength = u; }

    void addOPin(const string& s) { opins.emplace_back(s); }
    void addIPin(const string& s) { ipins.emplace_back(s); }
};

class STALibrary {
public:
    string name;
    vector<STALibraryCell> cells;

    void postLoad();
    unsigned addCell(const string& name);
};
}  // namespace sta

extern sta::STALibrary rlib;

#endif
