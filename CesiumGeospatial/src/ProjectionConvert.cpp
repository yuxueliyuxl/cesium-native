#include "ProjectionConvert.h"

#include <CesiumUtility/Math.h>

#include <cmath>

namespace CesiumGeospatial {

bool ProjectionConvert::out_of_china(double lng, double lat) noexcept {
  return !(lng > 73.66 && lng < 135.05 && lat > 3.86 && lat < 53.55);
}

double ProjectionConvert::transformLng(double lng, double lat) noexcept {
  double ret = 300.0 + lng + 2.0 * lat + 0.1 * lng * lng + 0.1 * lng * lat +
               0.1 * std::sqrt(std::abs(lng));
  ret += ((20.0 * std::sin(6.0 * lng * CesiumUtility::Math::OnePi) +
           20.0 * std::sin(2.0 * lng * CesiumUtility::Math::OnePi)) *
          2.0) /
         3.0;
  ret += ((20.0 * std::sin(lng * CesiumUtility::Math::OnePi) +
           40.0 * std::sin((lng / 3.0) * CesiumUtility::Math::OnePi)) *
          2.0) /
         3.0;
  ret += ((150.0 * std::sin((lng / 12.0) * CesiumUtility::Math::OnePi) +
           300.0 * std::sin((lng / 30.0) * CesiumUtility::Math::OnePi)) *
          2.0) /
         3.0;
  return ret;
}

double ProjectionConvert::transformLat(double lng, double lat) noexcept {
  double ret = -100.0 + 2.0 * lng + 3.0 * lat + 0.2 * lat * lat +
               0.1 * lng * lat + 0.2 * std::sqrt(std::abs(lng));
  ret += ((20.0 * std::sin(6.0 * lng * CesiumUtility::Math::OnePi) +
           20.0 * std::sin(2.0 * lng * CesiumUtility::Math::OnePi)) *
          2.0) /
         3.0;
  ret += ((20.0 * std::sin(lat * CesiumUtility::Math::OnePi) +
           40.0 * std::sin((lat / 3.0) * CesiumUtility::Math::OnePi)) *
          2.0) /
         3.0;
  ret += ((160.0 * std::sin((lat / 12.0) * CesiumUtility::Math::OnePi) +
           320.0 * std::sin((lat * CesiumUtility::Math::OnePi) / 30.0)) *
          2.0) /
         3.0;
  return ret;
}

glm::dvec2 ProjectionConvert::delta(double lng, double lat) noexcept {
  double dLng = transformLng(lng - 105.0, lat - 35.0);
  double dLat = transformLat(lng - 105.0, lat - 35.0);
  const double radLat = (lat / 180.0) * CesiumUtility::Math::OnePi;
  double magic = std::sin(radLat);
  magic = 1.0 - 0.00669342162296594323 * magic * magic;
  const double sqrtMagic = std::sqrt(magic);
  dLng =
      (dLng * 180.0) /
      ((6378245.0 / sqrtMagic) * std::cos(radLat) *
       CesiumUtility::Math::OnePi);
  dLat =
      (dLat * 180.0) /
      (((6378245.0 * (1.0 - 0.00669342162296594323)) /
        (magic * sqrtMagic)) *
       CesiumUtility::Math::OnePi);
  return glm::dvec2(dLng, dLat);
}

glm::dvec2
ProjectionConvert::WGS84ToGCJ02(double lng, double lat) noexcept {
  if (out_of_china(lng, lat)) {
    return glm::dvec2(lng, lat);
  }

  const glm::dvec2 offset = delta(lng, lat);
  return glm::dvec2(lng + offset.x, lat + offset.y);
}

glm::dvec2
ProjectionConvert::GCJ02ToWGS84(double lng, double lat) noexcept {
  if (out_of_china(lng, lat)) {
    return glm::dvec2(lng, lat);
  }

  const glm::dvec2 offset = delta(lng, lat);
  return glm::dvec2(lng - offset.x, lat - offset.y);
}

} // namespace CesiumGeospatial
