#include "mag_core.h"

#include <iostream>

int main()
{
    mag::RawVector raw { 0.31, 0.08, -0.42 };
    mag::Attitude attitude { 3.5, 8.0 };
    mag::Calibration calibration {};
    calibration.declinationDeg = 14.5;

    const mag::HeadingResult result = mag::computeHeading(raw, calibration, attitude);
    std::cout << "Heading: " << result.headingDeg << " deg\n";
    return 0;
}
