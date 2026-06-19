#pragma once

#include <glm/vec2.hpp>

namespace CesiumGeospatial {

class ProjectionConvert {
public:
  static bool out_of_china(double lng, double lat) noexcept;
  static double transformLng(double lng, double lat) noexcept;
  static double transformLat(double lng, double lat) noexcept;
  static glm::dvec2 delta(double lng, double lat) noexcept;
  static glm::dvec2 WGS84ToGCJ02(double lng, double lat) noexcept;
  static glm::dvec2 GCJ02ToWGS84(double lng, double lat) noexcept;
};

} // namespace CesiumGeospatial
