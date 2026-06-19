#pragma once

#include "Library.h"
#include <CesiumRasterOverlays/CreateRasterOverlayTileProviderParameters.h>
#include <CesiumRasterOverlays/RasterOverlay.h>

#include <CesiumAsync/IAssetRequest.h>
#include <CesiumGeospatial/Ellipsoid.h>

#include <functional>
#include <memory>

namespace CesiumHoloveser {

/**
 * @brief A {@link RasterOverlay} that obtains imagery data from Cesium ion.
 */
class CESIUMHOLOVESER_API HoloIonRasterOverlay final : public CesiumRasterOverlays::RasterOverlay {
public:
  /**
   * @brief Creates a new instance.
   *
   * The tiles that are provided by this instance will contain
   * imagery data that was obtained from the Cesium ion asset
   * with the given ID, accessed with the given access token.
   *
   * @param name The user-given name of this overlay layer.
   * @param ionAssetID The asset ID.
   * @param ionAccessToken The access token.
   * @param overlayOptions The {@link RasterOverlayOptions} for this instance.
   */
  HoloIonRasterOverlay(
      const std::string& name,
      int64_t ionAssetID,
      const std::string& ionAccessToken,
      bool debug,
      const CesiumRasterOverlays::RasterOverlayOptions& overlayOptions = {},
      const std::string& ionAssetEndpointUrl = "https://api.cesium.com/");
  virtual ~HoloIonRasterOverlay() override;

  virtual CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
      const CesiumRasterOverlays::CreateRasterOverlayTileProviderParameters&
          parameters) const override;

private:
  int64_t _ionAssetID;
  //HoloveserMapType _mapType;
  std::string _ionAccessToken;
  std::string _ionAssetEndpointUrl;
  bool _isdebug=false;

  struct AssetEndpointAttribution {
    std::string html;
    bool collapsible = true;
  };

  struct ExternalAssetEndpoint {
    std::string externalType;
    std::string url;
    std::string mapStyle;
    std::string key;
    std::string culture;
    std::string accessToken;
    std::vector<AssetEndpointAttribution> attributions;
  };

  static std::unordered_map<std::string, ExternalAssetEndpoint> endpointCache;

  CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
      const ExternalAssetEndpoint& endpoint,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
      const std::shared_ptr<CesiumRasterOverlays::IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner) const;
};

} // namespace CesiumHoloveser
