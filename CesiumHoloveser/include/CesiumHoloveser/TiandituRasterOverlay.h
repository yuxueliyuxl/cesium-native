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
 * @brief Options for 天地图 Web Map Tile Service (WMTS) overlays.
 */
struct TiandituRasterOverlayOptions {

  /**
   * @brief The Web Map Tile Service version. The default is "1.3.0".
   */
  std::string version = "1.3.0";

  /**
   * @brief Comma separated Web Map Tile Service layer names to request.
   */
  std::string layers;

  /**
   * @brief 层级阵列.
   */
  std::string tileMatrixSet;

  /**
   * @brief 图层风格.
   */

  std::string style;

  /**
   * @brief key,个人用户10000次/天，企业用户3000000次/天.
   */
  std::string key;

  /**
   * @brief 地图服务.
   */
  std::string mapStyle;

  /**
   * @brief 地图服务坐标系.
   */
  std::string epsg = "4490";

  /**
   * @brief 是否调试.
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
  uint32_t minimumLevel = 1;

  /**
   * @brief The maximum level-of-detail supported by the imagery provider.
   */
  uint32_t maximumLevel = 18;

  /**
   * @brief Pixel width of image tiles.
   */
  uint32_t tileWidth = 256;

  /**
   * @brief Pixel height of image tiles.
   */
  uint32_t tileHeight = 256;
};

/**
 * @brief A {@link RasterOverlay} accessing images from a Web Map Tile Service (WMTS) server.
 */
class CESIUMHOLOVESER_API TiandituRasterOverlay final
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
  TiandituRasterOverlay(
      const std::string& name,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers = {},
      const TiandituRasterOverlayOptions& wmtsOptions = {},
      const CesiumRasterOverlays::RasterOverlayOptions& overlayOptions = {});
  virtual ~TiandituRasterOverlay() override;

  virtual CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
      const CesiumRasterOverlays::CreateRasterOverlayTileProviderParameters&
          parameters) const override;

private:
  std::string _baseUrl;
  std::vector<CesiumAsync::IAssetAccessor::THeader> _headers;
  TiandituRasterOverlayOptions _options;
};

} // namespace CesiumHoloveser
