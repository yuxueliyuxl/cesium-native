#pragma once

#include "Library.h"

#include <CesiumRasterOverlays/CreateRasterOverlayTileProviderParameters.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>

#include <memory>

namespace CesiumHoloveser {

class CreditSystem;

/**
 * @brief Options for Web Map Service (WMS) overlays.
 */
struct UrlTemplateRasterOverlayOptions {

  /**
   * @brief The Web Map Service version. The default is "1.3.0".
   */
  std::string version = "1.3.0";

  /**
   * @brief Comma separated Web Map Service layer names to request.
   */
  std::string layers;

  /**
   * @brief The image format to request, expressed as a MIME type to be given to
   * the server. The default is "image/png".
   */
  std::string format = "image/png";

  /**
   * @brief A credit for the data source, which is displayed on the canvas.
   */
  std::optional<std::string> credit;

  /**
   * @brief The minimum level-of-detail supported by the imagery provider.
   *
   * Take care when specifying this that the number of tiles at the minimum
   * level is small, such as four or less. A larger number is likely to
   * result in rendering problems.
   */
  int32_t minimumLevel = 0;

  /**
   * @brief The maximum level-of-detail supported by the imagery provider.
   */
  int32_t maximumLevel = 18;

  /**
   * @brief Pixel width of image tiles.
   */
  int32_t tileWidth = 256;

  /**
   * @brief Pixel height of image tiles.
   */
  int32_t tileHeight = 256;

    /**
   *  是否调试. by zzt
   */
  bool debug = false;

    /**
   * @brief The subdomains to use for the <code>{s}</code> placeholder in the URL template.
   *
   * If this parameter is a single string, each character in the string is a
   * subdomain. If it is an array, each element in the array is a subdomain.
   */
  std::vector<std::string> subdomains;

    /**
   * @brief The {@link CesiumGeospatial::Projection} that is used.
   */
  std::optional<CesiumGeospatial::Projection> projection;

  /**
   * @brief The {@link CesiumGeometry::QuadtreeTilingScheme} specifying how
   * the ellipsoidal surface is broken into tiles.
   */
  std::optional<CesiumGeometry::QuadtreeTilingScheme> tilingScheme;
    /**
   * @brief The {@link CesiumGeometry::Rectangle}, in radians, covered by the
   * image.
   */
  std::optional<CesiumGeometry::Rectangle> coverageRectangle;
};

/**
 * @brief A {@link RasterOverlay} accessing images from a Web Map Service (WMS) server.
 */
class CESIUMHOLOVESER_API UrlTemplateRasterOverlay final
    : public CesiumRasterOverlays::RasterOverlay {
public:
  /**
   * @brief Creates a new instance.
   *
   * @param name The user-given name of this overlay layer.
   * @param url The base URL.
   * @param headers The headers. This is a list of pairs of strings of the
   * form (Key,Value) that will be inserted as request headers internally.
   * @param wmsOptions The {@link WebMapServiceRasterOverlayOptions}.
   * @param overlayOptions The {@link RasterOverlayOptions} for this instance.
   */
  UrlTemplateRasterOverlay(
      const std::string& name,
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers = {},
      const UrlTemplateRasterOverlayOptions& wmsOptions = {},
      const CesiumRasterOverlays::RasterOverlayOptions& overlayOptions = {});
  virtual ~UrlTemplateRasterOverlay() override;

  virtual CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
      const CesiumRasterOverlays::CreateRasterOverlayTileProviderParameters&
          parameters) const override;

private:
  std::string _baseUrl;
  std::vector<CesiumAsync::IAssetAccessor::THeader> _headers;
  UrlTemplateRasterOverlayOptions _options;
};

} // namespace CesiumRasterOverlays
