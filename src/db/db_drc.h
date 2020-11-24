#ifndef _DB_DRC_H_
#define _DB_DRC_H_

namespace db{
    class WireRule {
        private:
            Layer* _layer = nullptr;
        public:
            int width = 0;
            int space = 0;

            WireRule(Layer *layer = nullptr, int width = 0, int space = 0);

            const Layer* layer() const { return _layer; }
    };

    class NDR {
        private:
            string _name = "";
            bool _hardSpacing = false;
        public:
            vector<WireRule> rules;
            vector<ViaType*> vias;

            NDR(const string& name = "", const bool hardSpacing = false);

            const string& name() const { return _name; }
            bool hardSpacing() const { return _hardSpacing; }
    };

    class EdgeTypes {
        public:
            vector<string> types;
            vector<vector<int>> distTable;

            EdgeTypes() {}

            int getEdgeType(const string& name) const;
            int getEdgeSpace(const int edge1, const int edge2) const;
    };
}

#endif

