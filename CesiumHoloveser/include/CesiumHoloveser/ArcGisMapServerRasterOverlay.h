#pragma once

#include "Library.h"

#include <CesiumAsync/IAssetRequest.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumRasterOverlays/CreateRasterOverlayTileProviderParameters.h>
#include <CesiumRasterOverlays/RasterOverlay.h>

#include <functional>
#include <memory>

namespace CesiumHoloveser {

class CreditSystem;

/**
 * @brief Options for Web Map Service (WMS) overlays.
 */
struct ArcGisMapServerRasterOverlayOptions {

  /**
   * @brief The Web Map Service version. The default is "1.3.0".
   */
  std::string version = "1.0.0";

  /**
   * @zzt 投影坐标系.GCJ02或WGS84
   */
  std::string layers="WGS84";

  std::string tileMatrixSet;

  std::string style="default";

  std::string key;

  std::string epsg="3857";

    /**
   * @zzt 是否调试.
   */
  bool debug = false;
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
};

/**
 * @brief A {@link RasterOverlay} accessing images from a Web Map Service (WMS) server.
 */
class CESIUMHOLOVESER_API ArcGisMapServerRasterOverlay final
    : public CesiumRasterOverlays::RasterOverlay {
public:
  /**
   * @brief Creates a new instance.
   *
   * @param name The user-given name of this overlay layer.
   * @param url The base URL.
   * @param headers The headers. This is a list of pairs of strings of the
   * form (Key,Value) that will be inserted as request headers internally.
   * @param wmtsOptions The {@link WebMapServiceRasterOverlayOptions}.
   * @param overlayOptions The {@link RasterOverlayOptions} for this instance.
   */
  ArcGisMapServerRasterOverlay(
      const std::string& name,
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers = {},
      const ArcGisMapServerRasterOverlayOptions& wmtsOptions = {},
      const CesiumRasterOverlays::RasterOverlayOptions& overlayOptions = {});
  virtual ~ArcGisMapServerRasterOverlay() override;

  virtual CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
      const CesiumRasterOverlays::CreateRasterOverlayTileProviderParameters&
          parameters) const override;

private:
  std::string _baseUrl;
  std::vector<CesiumAsync::IAssetAccessor::THeader> _headers;
  ArcGisMapServerRasterOverlayOptions _options;
};

} // namespace CesiumHoloveser
