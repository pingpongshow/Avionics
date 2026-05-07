#include "mag_core.h"

#include <cmath>

namespace mag {

namespace {

constexpr double kPi = 3.14159265358979323846;

double toRadians(const double degrees)
{
    return degrees * kPi / 180.0;
}

double normalizeHeading(double headingDeg)
{
    while (headingDeg < 0.0) {
        headingDeg += 360.0;
    }
    while (headingDeg >= 360.0) {
        headingDeg -= 360.0;
    }
    return headingDeg;
}

} // namespace

HeadingResult computeHeading(const RawVector &raw, const Calibration &calibration, const Attitude &attitude)
{
    const double xBiasRemoved = raw.x - calibration.bias[0];
    const double yBiasRemoved = raw.y - calibration.bias[1];
    const double zBiasRemoved = raw.z - calibration.bias[2];

    HeadingResult result {};
    result.calibrated.x = calibration.softIronMatrix[0] * xBiasRemoved
        + calibration.softIronMatrix[1] * yBiasRemoved
        + calibration.softIronMatrix[2] * zBiasRemoved;
    result.calibrated.y = calibration.softIronMatrix[3] * xBiasRemoved
        + calibration.softIronMatrix[4] * yBiasRemoved
        + calibration.softIronMatrix[5] * zBiasRemoved;
    result.calibrated.z = calibration.softIronMatrix[6] * xBiasRemoved
        + calibration.softIronMatrix[7] * yBiasRemoved
        + calibration.softIronMatrix[8] * zBiasRemoved;

    const double pitchRad = toRadians(attitude.pitchDeg);
    const double rollRad = toRadians(attitude.rollDeg);

    const double xh = result.calibrated.x * std::cos(pitchRad)
        + result.calibrated.z * std::sin(pitchRad);
    const double yh = result.calibrated.x * std::sin(rollRad) * std::sin(pitchRad)
        + result.calibrated.y * std::cos(rollRad)
        - result.calibrated.z * std::sin(rollRad) * std::cos(pitchRad);

    result.headingDeg = normalizeHeading(std::atan2(yh, xh) * 180.0 / kPi + calibration.declinationDeg);
    return result;
}

} // namespace mag

