#include <args/args.hxx>

#include "db/db.h"
#include "global.h"
#include "io/io.h"

int main(int argc, char** argv) {
    init_log(LOG_ALL ^ LOG_VERBOSE);

    args::ArgumentParser parser("This is a extraction program.", "This goes after the options.");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::CompletionFlag completion(parser, {"complete"});

    args::ValueFlag<string> cell(parser, "cell", "The cell lef flag", {'c', "cell_lef"});
    args::ValueFlag<string> flow(parser, "flow", "The flow flag", {'f', "flow"});
    args::ValueFlag<string> input(parser, "input", "The input def flag", {'i', "input_def"});
    args::ValueFlag<string> liberty(parser, "liberty", "The liberty flag", {'l', "liberty"});
    args::ValueFlag<string> metal(parser, "metal", "The metal flag", {'m', "metal"});
    args::ValueFlag<string> net(parser, "net", "The net detail flag", {'n', "net_detail"});
    args::ValueFlag<string> numCands(parser, "num cands", "The number of candidates flag", {'d', "num_cands"});
    args::ValueFlag<string> output(parser, "output", "The output flag", {'o', "output_csv"});
    args::ValueFlag<string> tech(parser, "tech", "The tech lef flag", {'c', "tech_lef"});
    args::ValueFlag<string> timePath(parser, "time path", "The time path flag", {'p', "time_path"});
    args::ValueFlag<string> timeUnconstrain(
        parser, "time unconstrain", "The time unconstrain flag", {'u', "time_unconstrain"});

    try {
        parser.ParseCLI(argc, argv);
    } catch (const args::Completion& e) {
        std::cout << e.what();
        return 0;
    } catch (const args::Help&) {
        std::cout << parser;
        return 0;
    } catch (const args::ParseError& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    } catch (const args::ValidationError& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    if (cell) {
        io::IOModule::LefCell = args::get(cell);
    }
    if (flow) {
        const string& sFlow = args::get(flow);
        if (sFlow == "extract") {
            io::IOModule::IOFlow = io::IOModule::Flow::extract;
        }
    }
    if (input) {
        io::IOModule::DefCell = args::get(input);
    }
    if (liberty) {
        io::IOModule::Liberty = args::get(liberty);
    }
    if (metal) { db::DBModule::Metal = atoi(args::get(metal).c_str()); }
    if (net) {
        io::IOModule::NetDetail = args::get(net);
    }
    if (numCands) { db::DBModule::NumCands = atoi(args::get(numCands).c_str()); }
    if (output) {
        io::IOModule::DefPlacement = args::get(output);
    }
    if (tech) {
        io::IOModule::LefTech = args::get(tech);
    }
    if (timePath) io::IOModule::TimePath = args::get(timePath);
    if (timeUnconstrain) io::IOModule::TimeUnconstrain = args::get(timeUnconstrain);

    io::IOModule::load();
    db::DBModule::setup();

    printlog(LOG_INFO, "Terminating...");

    return 0;
}
