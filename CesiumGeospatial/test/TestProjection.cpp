#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/ext/vector_double2.hpp>
#include <glm/geometric.hpp>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

TEST_CASE("WebMercatorProjection supports GCJ02") {
  const Cartographic beijing =
      Cartographic::fromDegrees(116.3913, 39.9075, 100.0);
  const WebMercatorProjection wgs84(Ellipsoid::WGS84, "WGS84");
  const WebMercatorProjection gcj02(Ellipsoid::WGS84, "GCJ02");

  const glm::dvec3 wgs84Projected = wgs84.project(beijing);
  const glm::dvec3 gcj02Projected = gcj02.project(beijing);

  CHECK(glm::distance(wgs84Projected, gcj02Projected) > 100.0);

  const Cartographic roundTrip = gcj02.unproject(gcj02Projected);
  CHECK(Math::equalsEpsilon(
      roundTrip.longitude,
      beijing.longitude,
      0.0,
      Math::degreesToRadians(0.0001)));
  CHECK(Math::equalsEpsilon(
      roundTrip.latitude,
      beijing.latitude,
      0.0,
      Math::degreesToRadians(0.0001)));
  CHECK(roundTrip.height == doctest::Approx(beijing.height));
}

TEST_CASE("computeProjectedRectangleSize") {
  SUBCASE("Entire Globe") {
    GeographicProjection projection(Ellipsoid::WGS84);
    Rectangle rectangle = projectRectangleSimple(
        projection,
        GlobeRectangle::fromDegrees(-180, -90, 180, 90));
    double maxHeight = 0.0;
    glm::dvec2 size = computeProjectedRectangleSize(
        projection,
        rectangle,
        maxHeight,
        Ellipsoid::WGS84);
    CHECK(size.x - Ellipsoid::WGS84.getMaximumRadius() * 2.0 > 0.0);
    CHECK(Math::equalsEpsilon(
        size.y,
        Ellipsoid::WGS84.getMinimumRadius() * 2.0,
        0.0,
        1.0));
  }

  SUBCASE("Hemispheres") {
    // A hemisphere should have approximately the diameter of the
    // globe.
    GeographicProjection projection(Ellipsoid::WGS84);
    Rectangle rectangle = projectRectangleSimple(
        projection,
        GlobeRectangle::fromDegrees(-180, -90, 0, 90));
    double maxHeight = 0.0;
    glm::dvec2 size = computeProjectedRectangleSize(
        projection,
        rectangle,
        maxHeight,
        Ellipsoid::WGS84);
    CHECK(Math::equalsEpsilon(
        size.x,
        Ellipsoid::WGS84.getMaximumRadius() * 2.0,
        0.0,
        1.0));
    CHECK(Math::equalsEpsilon(
        size.y,
        Ellipsoid::WGS84.getMinimumRadius() * 2.0,
        0.0,
        1.0));

    rectangle = projectRectangleSimple(
        projection,
        GlobeRectangle::fromDegrees(0, -90, 180, 90));
    size = computeProjectedRectangleSize(
        projection,
        rectangle,
        maxHeight,
        Ellipsoid::WGS84);
    CHECK(Math::equalsEpsilon(
        size.x,
        Ellipsoid::WGS84.getMaximumRadius() * 2.0,
        0.0,
        1.0));
    CHECK(Math::equalsEpsilon(
        size.y,
        Ellipsoid::WGS84.getMinimumRadius() * 2.0,
        0.0,
        1.0));
  }

  SUBCASE("Rectangle crossing the equator") {
    // For a rectangle that crosses the equator, the widest part is at the
    // equator.
    GeographicProjection projection(Ellipsoid::WGS84);
    Rectangle rectangle = projectRectangleSimple(
        projection,
        GlobeRectangle::fromDegrees(-100, -70, -80, 40));
    double maxHeight = 0.0;
    glm::dvec2 size = computeProjectedRectangleSize(
        projection,
        rectangle,
        maxHeight,
        Ellipsoid::WGS84);
    CHECK(Math::equalsEpsilon(
        size.x,
        glm::distance(
            Ellipsoid::WGS84.cartographicToCartesian(
                Cartographic::fromDegrees(-100.0, 0.0, 0.0)),
            Ellipsoid::WGS84.cartographicToCartesian(
                Cartographic::fromDegrees(-80.0, 0.0, 0.0))),
        0.0,
        1.0));
  }

  SUBCASE("Narrow band around the entire globe") {
    GeographicProjection projection(Ellipsoid::WGS84);
    Rectangle rectangle = projectRectangleSimple(
        projection,
        GlobeRectangle::fromDegrees(-180, 20, 180, 40));
    double maxHeight = 0.0;
    glm::dvec2 size = computeProjectedRectangleSize(
        projection,
        rectangle,
        maxHeight,
        Ellipsoid::WGS84);
    CHECK(size.x - Ellipsoid::WGS84.getMaximumRadius() * 2.0 > 0.0);
    CHECK(Math::equalsEpsilon(
        size.y,
        glm::distance(
            Ellipsoid::WGS84.cartographicToCartesian(
                Cartographic::fromDegrees(0.0, 20.0, 0.0)),
            Ellipsoid::WGS84.cartographicToCartesian(
                Cartographic::fromDegrees(0.0, 40.0, 0.0))),
        0.0,
        1.0));
  }
}
