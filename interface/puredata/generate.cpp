#include "generate.h"
#include "../../common/error.hpp"

#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;
using nlohmann::json;

namespace arrp {
namespace puredata_io {

void generate(const options & opt, const nlohmann::json & report)
{
    string kernel_file_name = report["cpp"]["filename"];
    string kernel_namespace = report["cpp"]["namespace"];

    int periodFrames = 0;
    int inputChannels = 0;
    int outputChannels = 0;

    auto & inputs = report["inputs"];
    auto & outputs = report["outputs"];

    for (auto & channel : inputs)
    {
        if (not channel["is_stream"])
            throw stream::error("Input is not a stream: " + string(channel["name"]));
        if (string(channel["type"]) != "real32")
            throw stream::error("Input does not have type real32: " + string(channel["name"]));
        if (channel.count("dimensions") and channel["dimensions"].size() > 1)
            throw stream::error("Input has too many dimensions: " + string(channel["name"]));
    }

    for (auto & channel : outputs)
    {
        if (not channel["is_stream"])
            throw stream::error("Output is not a stream: " + string(channel["name"]));
        if (string(channel["type"]) != "real32")
            throw stream::error("Output does not have type real32: " + string(channel["name"]));
        if (channel.count("dimensions") and channel["dimensions"].size() > 1)
            throw stream::error("Output has too many dimensions: " + string(channel["name"]));
    }

    json input, output;

    if (inputs.size() > 1)
    {
        throw stream::error("Too many inputs (only one supported).");
    }
    else if (inputs.size())
    {
        input = inputs[0];
    }

    if (outputs.size() != 1)
    {
        throw stream::error("Exactly one output required.");
    }

    {
        output = outputs[0];
        periodFrames = output["period_count"];
        outputChannels = output["dimensions"].size() ? int(output["dimensions"][0]) : 1;
    }

    if (!input.is_null())
    {
        if (input["period_count"] != periodFrames)
        {
            throw stream::error("Input and output rates are different.");
        }
        inputChannels = input["dimensions"].size() ? int(input["dimensions"][0]) : 1;
    }

    ostringstream io_text;

    io_text << "#include \"" << kernel_file_name << "\"" << endl;
    io_text << "#include <arrp/puredata_io/interface.h>" << endl;
    io_text << "#include <memory>" << endl;

    io_text << "namespace arrp { namespace puredata_io {" << endl;

    io_text << "class IO : public Abstract_IO {" << endl;

    io_text << "using Kernel = " << kernel_namespace << "::program<IO>;" << endl;
    io_text << "std::unique_ptr<Kernel> kernel;" << endl;

    io_text << "public:" << endl;
    io_text << "IO(): "
            << "Abstract_IO(" << inputChannels << ", " << outputChannels << ")"
            << "{}" << endl;

    io_text << "void prologue() override {" << endl;
    io_text << "kernel = std::make_unique<Kernel>();" << endl;
    io_text << "kernel->io = this;" << endl;
    io_text << "kernel->prelude();" << endl;
    io_text << "}" << endl;

    io_text << "void period() override {" << endl;
    io_text << "kernel->period();" << endl;
    io_text << "}" << endl;

    if (!input.is_null())
    {
        string name = input["name"];
        int size = input["size"];

        io_text << "void input_" << name;
        if (size == 1)
            io_text << "(float & value)";
        else
            io_text << "(float (&value)[" << size << "])";
        io_text << " { input(value); }" << endl;
    }

    {
        string name = output["name"];
        int size = output["size"];

        io_text << "void output_" << name;
        if (size == 1)
            io_text << "(float & value)";
        else
            io_text << "(float (&value)[" << size << "])";
        io_text << " { output(value); }" << endl;
    }

    io_text << "};" << endl; // class

    io_text << "Abstract_IO * create_kernel() { return new IO; }" << endl;

    io_text << "void library_setup(const char * name);" << endl;

    io_text << "}}" << endl; // namespace

    io_text << "extern \"C\" {" << endl;

    io_text << "void " << opt.pd_object_name << "_tilde_setup()"
            << " { arrp::puredata_io::library_setup(\"" << opt.pd_object_name << "~\"); }"
            << endl;

    io_text << "}" << endl; // extern C

    {
        string filename = opt.base_file_name + "-pd-interface.cpp";
        cerr << "Writing to " << filename << endl;
        ofstream io_file(filename);
        io_file << io_text.str();
    }
}

}
}

