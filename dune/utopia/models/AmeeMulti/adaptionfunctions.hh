#ifndef UTOPIA_MODELS_AGENT_STATE_HH
#define UTOPIA_MODELS_AGENT_STATE_HH
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
auto multi_notnormed = [](auto agent) -> std::vector<double> {
    auto& state = agent->state();
    auto start = state.start;
    auto end = state.end;
    const auto& celltrait = state.habitat->state().celltrait;
    const auto& trait = state.phenotype;
    std::vector<double> adaption(end - start, 0.);

    if (start >= 0 and end > 0 and end > start and trait.size() > 0 and
        (int) celltrait.size() > 0 and start < (int)trait.size() and
        start < (int)celltrait.size())
    {
        int i = start;
        int j = 0;
        for (; i < end && i < (int)celltrait.size() && i < (int)trait.size(); ++i, ++j)
        {
            adaption[j] = (trait[i] * celltrait[i]) /
                          (1. + (std::abs(trait[i] - celltrait[i])));

            if (std::isnan(adaption[j]) or (adaption[j] < 0.))
            {
                adaption[j] = 0.;
            }

            if (std::isinf(adaption[j]))
            {
                throw std::runtime_error("Inf found in adaption");
            }
        }
    }
    return adaption;
};

auto multi_normed = [](auto agent) -> std::vector<double> {
    auto& state = agent->state();
    auto start = state.start;
    auto end = state.end;
    const auto& celltrait = state.habitat->state().celltrait;
    const auto& trait = state.phenotype;

    if (start >= 0 and end > 0 and end > start and trait.size() > 0 and
        (int) celltrait.size() > 0 and start < (int)trait.size() and
        start < (int)celltrait.size())
    {
        std::vector<double> adaption(end - start, 0.);

        int i = start;
        int j = 0;
        for (; i < end && i < (int)celltrait.size() && i < (int)trait.size(); ++i, ++j)
        {
            adaption[j] = ((trait[i] * celltrait[i]) /
                           (1. + (std::abs(trait[i] - celltrait[i])))) /
                          (end - start);

            if (std::isnan(adaption[j]) or (adaption[j] < 0.))
            {
                adaption[j] = 0.;
            }

            if (std::isinf(adaption[j]))
            {
                throw std::runtime_error("Inf found in adaption");
            }
        }
        return adaption;
    }
    else
    {
        return std::vector<double>();
    }
};

auto simple_notnormed = [](auto agent) {
    auto& state = agent->state();
    auto start = state.start;
    auto end = state.end;
    const auto& celltrait = state.habitat->state().celltrait;
    const auto& trait = state.phenotype;

    if (start >= 0 and end > 0 and end > start and trait.size() > 0 and
        (int) celltrait.size() > 0 and start < (int)trait.size() and
        start < (int)celltrait.size())
    {
        std::vector<double> adaption(end - start, 0.);

        for (int i = start;
             i < end && i < (int)celltrait.size() && i < (int)trait.size(); ++i)
        {
            adaption[i] = 1. / (1. + (std::abs(trait[i] - celltrait[i])));

            if (std::isnan(adaption[i]) or (adaption[i] < 0.))
            {
                adaption[i] = 0.;
            }
            if (std::isinf(adaption[i]))
            {
                throw std::runtime_error("Inf found in adaption");
            }
        }
        return adaption;
    }
    else
    {
        return std::vector<double>();
    }
};

auto simple_normed = [](auto agent) {
    auto& state = agent->state();
    auto start = state.start;
    auto end = state.end;
    const auto& celltrait = state.habitat->state().celltrait;
    const auto& trait = state.phenotype;

    if (start >= 0 and end > 0 and end > start and trait.size() > 0 and
        (int) celltrait.size() > 0 and start < (int)trait.size() and
        start < (int)celltrait.size())
    {
        std::vector<double> adaption(end - start, 0.);

        for (int i = start;
             i < end && i < (int)celltrait.size() && i < (int)trait.size(); ++i)
        {
            adaption[i] = (1. / (1. + (std::abs(trait[i] - celltrait[i])))) / (end - start);

            if (std::isnan(adaption[i]) or (adaption[i] < 0.))
            {
                adaption[i] = 0.;
            }
            if (std::isinf(adaption[i]))
            {
                throw std::runtime_error("Inf found in adaption");
            }
        }
        return adaption;
    }
    else
    {
        return std::vector<double>();
    }
};

} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia
#endif