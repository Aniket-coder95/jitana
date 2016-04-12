/*
 * Copyright (c) 2016, Shakthi Bachala
 * Copyright (c) 2016, Yutaka Tsutano
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <iostream>
#include <vector>
#include <regex>
#include <algorithm>
#include <string>
#include <fstream>

#include <boost/graph/graphviz.hpp>

#include <jitana/jitana.hpp>
#include <jitana/analysis/call_graph.hpp>
#include <jitana/analysis/data_flow.hpp>
#include <jitana/analysis/intent_flow.hpp>

void write_graphs(const jitana::virtual_machine& vm);

void run_iac_analysis()
{
    jitana::virtual_machine vm;

    {
        const auto& filenames
                = {"../../../dex/framework/core.dex",
                   "../../../dex/framework/framework.dex",
                   "../../../dex/framework/framework2.dex",
                   "../../../dex/framework/ext.dex",
                   "../../../dex/framework/conscrypt.dex",
                   "../../../dex/framework/okhttp.dex",
                   "../../../dex/framework/core-junit.dex",
                   "../../../dex/framework/android.test.runner.dex",
                   "../../../dex/framework/android.policy.dex"};
        jitana::class_loader loader(0, "SystemLoader", begin(filenames),
                                    end(filenames));
        vm.add_loader(loader);
    }

    std::ifstream location_ifs("extracted/location.txt");
    std::string name;
    for (int loader_idx = 1; std::getline(location_ifs, name); ++loader_idx) {
        std::cout << "Loading " << loader_idx << " " << name << "..."
                  << std::endl;
        vm.add_apk(loader_idx, "extracted/" + name, 0);
        vm.load_all_classes(loader_idx);
    }

    // Compute the call graph.
    std::cout << "Computing the call graph..." << std::endl;
    jitana::add_call_graph_edges(vm);

    // Compute the data-flow.
    std::cout << "Computing the data-flow..." << std::endl;
    std::for_each(vertices(vm.methods()).first, vertices(vm.methods()).second,
                  [&](const jitana::method_vertex_descriptor& v) {
                      add_data_flow_edges(vm.methods()[v].insns);
                  });

    // Compute the intent-flow edges.
    std::cout << "Computing the intent-flow..." << std::endl;
#if 0
    jitana::add_intent_flow_edges(vm);
#else
    jitana::add_intent_flow_edges_from_strings(vm);
#endif

    std::cout << "Writing graphs..." << std::endl;
    write_graphs(vm);

    std::cout << "# of Loaders: " << num_vertices(vm.loaders()) << "\n";
    std::cout << "# of classes: " << num_vertices(vm.classes()) << "\n";
    std::cout << "# of methods: " << num_vertices(vm.methods()) << "\n";
    std::cout << "# of fields:  " << num_vertices(vm.fields()) << "\n";
}

void write_graphs(const jitana::virtual_machine& vm)
{
    {
        std::ofstream ofs("output/loader_graph.dot");
        auto g = jitana::
                make_edge_filtered_graph<jitana::loader_parent_edge_property>(
                        vm.loaders());
        write_graphviz_loader_graph(ofs, g);
    }

    {
        std::ofstream ofs("output/intent_graph.dot");
        auto g = jitana::
                make_edge_filtered_graph<jitana::intent_flow_edge_property>(
                        vm.loaders());
        write_graphviz_loader_graph(ofs, g);
    }

    {
        std::ofstream ofs("output/class_graph.dot");
        write_graphviz_class_graph(ofs, vm.classes());
    }
}

int main()
{
    run_iac_analysis();
}
