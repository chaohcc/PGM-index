// This file is part of PGM-index <https://github.com/gvinciguerra/PGM-index>.
// Copyright (c) 2018 Giorgio Vinciguerra.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstring>
#include "args.hxx"
#include "tuner.hpp"

int main(int argc, char **argv) {
    using namespace args;
    ArgumentParser p("Space-time trade-off tuner for the PGM-index.",
                     "This program lets you specify a maximum space and get the PGM-index minimising the query time "
                     "within that space.  Or, it lets you specify a maximum query time and get the PGM-index "
                     "minimising the space.");
    HelpFlag help(p, "help", "Display this help menu", {'h', "help"});
    Group g(p, "Operation modes:", args::Group::Validators::Xor, args::Options::Required);
    ValueFlag<size_t> time(g, "ns", "Specify a time to minimise the space", {'t', "time"});
    ValueFlag<size_t> space(g, "bytes", "Specify a space to minimise the time", {'s', "space"});
    ValueFlag<double> tol(p, "float", "Tolerance between 0 and 1 on the constraint (default 0.01)", {'o', "tol"}, 0.01);
    Flag verbose(p, "verbose", "Show additional logging info", {'v', "verbose"});
    Group t(p, "File type:", args::Group::Validators::Xor, args::Options::Required);
    Flag bin(t, "binary", "The input file is a binary file containing 32-bit integers", {'b', "binary"});
    Flag csv(t, "csv", "The input file is a csv file containing integers separated by a newline", {'c', "csv"});
    Positional<std::string> file(p, "file", "The file containing the input data", args::Options::Required);
    CompletionFlag completion(p, {"complete"});

    try {
        p.ParseCLI(argc, argv);
    }
    catch (args::Help &) {
        std::cout << p;
        return 0;
    }
    catch (args::Error &e) {
        std::cerr << p;
        return 1;
    }

    std::vector<int64_t> data;
    try {
        data = bin.Get() ? read_data_binary<int32_t, int64_t>(file.Get()) : read_data_csv(file.Get());
    } catch (std::ios_base::failure &e) {
        std::cerr << e.what() << std::endl;
        std::cerr << std::strerror(errno) << std::endl;
        exit(1);
    }

    std::sort(data.begin(), data.end());
    auto lo_eps = 2 * cache_line_size() / sizeof(int64_t);
    auto hi_eps = data.size() / 2;
    auto minimize_space = time.Matched();

    printf("Dataset: %zu entries\n", data.size());
    if (minimize_space)
        printf("Max time: %zu±%.0f ns\n", time.Get(), time.Get() * tol.Get());
    else
        printf("Max space: %zu±%.0f KiB\n", space.Get() / (1ul << 10ul), space.Get() * tol.Get() / (1ul << 10ul));

    printf("%s\n", std::string(80, '-').c_str());
    printf("%-19s %-19s %-19s %-19s\n", "Epsilon", "Construction (s)", "Space (KiB)", "Query (ns)");
    printf("%s\n", std::string(80, '-').c_str());

    if (minimize_space)
        minimize_space_given_time(time.Get(), tol.Get(), data, lo_eps, hi_eps, verbose.Get());
    else
        minimize_time_given_space(space.Get(), tol.Get(), data, lo_eps, hi_eps, verbose.Get());
}