#pragma once

#include <array>

namespace mag {

struct RawVector {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct Attitude {
    double pitchDeg = 0.0;
    double rollDeg = 0.0;
};

struct Calibration {
    std::array<double, 3> bias { 0.0, 0.0, 0.0 };
    std::array<double, 9> softIronMatrix {
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    };
    double declinationDeg = 0.0;
};

struct HeadingResult {
    RawVector calibrated {};
    double headingDeg = 0.0;
};

HeadingResult computeHeading(const RawVector &raw, const Calibration &calibration, const Attitude &attitude);

} // namespace mag

