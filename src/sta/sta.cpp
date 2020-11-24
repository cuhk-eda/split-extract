#include "sta.h"
using namespace sta;

#include "../db/db.h"
using namespace db;

#include <experimental/iterator>

STALibrary rlib;

void STALibraryLUT::clear() {
    indexX.clear();
    indexY.clear();
    indexZ.clear();
    values.clear();
}

void STALibraryLUT::print() {
    cout << "index_1 : ";
    copy(indexX.begin(), indexX.end(), experimental::make_ostream_joiner(cout, " "));
    cout << endl;

    cout << "index_2 : ";
    copy(indexY.begin(), indexY.end(), experimental::make_ostream_joiner(cout, " "));
    cout << endl;

    cout << "index_3 : ";
    copy(indexZ.begin(), indexZ.end(), experimental::make_ostream_joiner(cout, " "));
    cout << endl;

    for (const vector<vector<double>>& value : values) {
        for (const vector<double>& val : value) {
            copy(val.begin(), val.end(), experimental::make_ostream_joiner(cout, " "));
            cout << endl;
        }
        cout << endl;
    }
}

double STALibraryOPin::default_max_capacitance = 0;
double STALibraryIPin::default_capacitance = 0;

void STALibrary::postLoad() {
    for (STALibraryCell& libcell : cells) {
        unsigned nOPins = libcell.opins.size();
        unsigned nIPins = libcell.ipins.size();
        libcell.timingArcs.resize(nIPins, vector<int>(nOPins, -1));
        for (unsigned op = 0; op != nOPins; ++op) {
            unsigned nTimings = libcell.opins[op].timings.size();
            for (unsigned t = 0; t != nTimings; ++t) {
                STALibraryTiming& timing = libcell.opins[op].timings[t];
                bool ipFound = false;
                for (unsigned ip = 0; ip != nIPins; ++ip) {
                    if (libcell.ipins[ip].name() == timing.relatedPinName) {
                        timing.relatedPin = ip;
                        if (libcell.timingArcs[ip][op] != -1) {
                            //  cerr << "timing arc duplicated!" << endl;
                        }
                        libcell.timingArcs[ip][op] = t;
                        ipFound = true;
                        break;
                    }
                }
                if (!ipFound) {
                    cout << "related pin (" << timing.relatedPinName << ") not found for pin "
                         << libcell.opins[op].name() << endl;
                }
            }
        }
    }
}

unsigned STALibrary::addCell(const string& name) {
    cells.emplace_back(name);
    return cells.size() - 1;
}
