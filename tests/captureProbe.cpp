// Finite desktop harness. Heap allocation is confined to the host, not effect DSP.
#include "Patch.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <span>
#include <string>
#include <vector>

struct Event
{
    std::size_t frame;
    std::string kind;
    int index;
    float value;
};

int main(int argc, char** argv)
{
    if (argc != 8)
    {
        std::fprintf(stderr, "usage: captureProbe input.f32 output.f32 blockSize events.txt param0 param1 param2\n");
        return EXIT_FAILURE;
    }
    const int blockSize = std::atoi(argv[3]);
    if (blockSize < 1 || blockSize > 65536) return EXIT_FAILURE;
    std::ifstream input(argv[1], std::ios::binary | std::ios::ate);
    if (!input || input.tellg() <= 0 || input.tellg() % (2 * sizeof(float)) != 0) return EXIT_FAILURE;
    const std::size_t floatCount = static_cast<std::size_t>(input.tellg()) / sizeof(float);
    const std::size_t frames = floatCount / 2;
    std::vector<float> source(floatCount), output(floatCount);
    input.seekg(0);
    input.read(reinterpret_cast<char*>(source.data()), static_cast<std::streamsize>(floatCount * sizeof(float)));
    if (!input) return EXIT_FAILURE;
    std::vector<Event> events;
    std::ifstream eventFile(argv[4]);
    Event event{};
    while (eventFile >> event.frame >> event.kind >> event.index >> event.value)
    {
        if (event.frame >= frames || (!events.empty() && event.frame < events.back().frame)) return EXIT_FAILURE;
        if ((event.kind != "parameter" && event.kind != "action") || !std::isfinite(event.value)) return EXIT_FAILURE;
        events.push_back(event);
    }
    if (!eventFile.eof()) return EXIT_FAILURE;
    Patch* const patch = Patch::getInstance();
    std::vector<float> storage(Patch::kWorkingBufferSize, 0.0f);
    patch->setWorkingBuffer(std::span<float, Patch::kWorkingBufferSize>(storage.data(), storage.size()));
    patch->init();
    std::array<float, 3> parameters{};
    for (int index = 0; index < 3; index += 1)
    {
        parameters[index] = std::string(argv[index + 5]) == "default"
            ? patch->getParameterMetadata(index).defaultValue : std::strtof(argv[index + 5], nullptr);
        patch->setParamValue(index, parameters[index]);
    }
    std::vector<float> left(blockSize), right(blockSize);
    std::size_t offset = 0, nextEvent = 0;
    while (offset < frames)
    {
        while (nextEvent < events.size() && events[nextEvent].frame == offset)
        {
            const Event& scheduled = events[nextEvent];
            if (scheduled.kind == "action") patch->handleAction(scheduled.index);
            else patch->setParamValue(scheduled.index, scheduled.value);
            nextEvent += 1;
        }
        std::size_t count = std::min(static_cast<std::size_t>(blockSize), frames - offset);
        if (nextEvent < events.size()) count = std::min(count, events[nextEvent].frame - offset);
        for (std::size_t index = 0; index < count; index += 1)
        {
            left[index] = source[(offset + index) * 2];
            right[index] = source[(offset + index) * 2 + 1];
        }
        patch->processAudio(std::span<float>(left.data(), count), std::span<float>(right.data(), count));
        for (std::size_t index = 0; index < count; index += 1)
        {
            if (!std::isfinite(left[index]) || !std::isfinite(right[index]))
            {
                std::fprintf(stderr, "Nonfinite effect output at frame %zu\n", offset + index);
                return EXIT_FAILURE;
            }
            output[(offset + index) * 2] = left[index];
            output[(offset + index) * 2 + 1] = right[index];
        }
        offset += count;
    }
    std::ofstream destination(argv[2], std::ios::binary);
    destination.write(reinterpret_cast<const char*>(output.data()), static_cast<std::streamsize>(floatCount * sizeof(float)));
    if (!destination) return EXIT_FAILURE;
    std::printf("{\"sampleRate\":%d,\"parameters\":[%.9g,%.9g,%.9g]}\n", Patch::kSampleRate,
                static_cast<double>(parameters[0]), static_cast<double>(parameters[1]), static_cast<double>(parameters[2]));
    return EXIT_SUCCESS;
}
