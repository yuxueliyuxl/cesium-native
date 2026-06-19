
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumHoloveser/UrlTemplateRasterOverlay.h>
#include <CesiumRasterOverlays/QuadtreeRasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/Uri.h>

#include <tinyxml2.h>

#include <cstddef>
#include <sstream>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace CesiumHoloveser {

static CesiumRasterOverlays::CreateRasterOverlayTileProviderParameters
makeTileProviderParameters(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
    const std::shared_ptr<
        CesiumRasterOverlays::IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const IntrusivePointer<const CesiumRasterOverlays::RasterOverlay>& pOwner) {
  return CesiumRasterOverlays::CreateRasterOverlayTileProviderParameters{
      CesiumRasterOverlays::RasterOverlayExternals{
          pAssetAccessor,
          pPrepareRendererResources,
          asyncSystem,
          pCreditSystem,
          pLogger},
      pOwner,
      nullptr};
}

class UrlTemplateTileProvider final
    : public CesiumRasterOverlays::QuadtreeRasterOverlayTileProvider {
public:
  UrlTemplateTileProvider(
      const IntrusivePointer<const CesiumRasterOverlays::RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
      std::optional<Credit> credit,
      const std::shared_ptr<
          CesiumRasterOverlays::IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const CesiumGeospatial::Projection& projection,
      const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
      const CesiumGeometry::Rectangle& coverageRectangle,
      const std::string& url,
      const std::vector<IAssetAccessor::THeader>& headers,
      const std::string& version,
      const std::string& layers,
      const std::string& format,
      uint32_t width,
      uint32_t height,
      uint32_t minimumLevel,
      uint32_t maximumLevel,
      bool debug,
      std::vector<std::string> subdomains)
      : QuadtreeRasterOverlayTileProvider(
            pOwner,
            makeTileProviderParameters(
                asyncSystem,
                pAssetAccessor,
                pCreditSystem,
                pPrepareRendererResources,
                pLogger,
                pOwner),
            projection,
            tilingScheme,
            coverageRectangle,
            minimumLevel,
            maximumLevel,
            width,
            height),
        _url(url),
        _headers(headers),
        _version(version),
        _layers(layers),
        _format(format),
        _debug(debug),
        _subdomains(subdomains),
        _asyncSystem(asyncSystem) {
    if (credit) {
      this->getCredits().emplace_back(*credit);
    }
  }

  virtual ~UrlTemplateTileProvider() = default;

protected:
  virtual CesiumAsync::Future<CesiumRasterOverlays::LoadedRasterOverlayImage>
  loadQuadtreeTileImage(
      const CesiumGeometry::QuadtreeTileID& tileID) const override {
    CesiumRasterOverlays::LoadTileImageFromUrlOptions options;
    options.rectangle = this->getTilingScheme().tileToRectangle(tileID);
    options.moreDetailAvailable = tileID.level < this->getMaximumLevel();

    const std::string urlTemplate = this->_url;
    std::map<std::string, std::string> urlTemplateMap = {
        {"x", std::to_string(tileID.x)},
        {"y", std::to_string(tileID.computeInvertedY(this->getTilingScheme()))},
        {"z", std::to_string(tileID.level)},
        {"X", std::to_string(tileID.x)},
        {"Y", std::to_string(tileID.computeInvertedY(this->getTilingScheme()))},
        {"Z", std::to_string(tileID.level)}};

    if (_subdomains.size() > 0) {
      urlTemplateMap.emplace(
          "s",
          _subdomains[static_cast<size_t>(rand()) % _subdomains.size()]);
    }

    std::string url = CesiumUtility::Uri::substituteTemplateParameters(
        urlTemplate,
        [&map = urlTemplateMap](const std::string& placeholder) {
          auto it = map.find(placeholder);
          return it == map.end() ? "{" + placeholder + "}"
                                 : Uri::escape(it->second);
        });
    // 低于getMinimumLevel返回空
    if (tileID.level < this->getMinimumLevel() ||
        tileID.level > this->getMaximumLevel()) {
      ErrorList errors;
      return _asyncSystem.createResolvedFuture(
          CesiumRasterOverlays::LoadedRasterOverlayImage{
              nullptr,
              options.rectangle,
              std::move(options.credits),
              std::move(errors),
              options.moreDetailAvailable});
    }

    if (_debug) {
      SPDLOG_LOGGER_INFO(this->getLogger(), url);
    }

    options.allowEmptyImages = true;
    return this->loadTileImageFromUrl(url, this->_headers, std::move(options));
  }

private:
  std::string _url;
  std::vector<IAssetAccessor::THeader> _headers;
  std::string _version;
  std::string _layers;
  std::string _format;
  bool _debug;
  std::vector<std::string> _subdomains;
  CesiumAsync::AsyncSystem _asyncSystem;
};

UrlTemplateRasterOverlay::UrlTemplateRasterOverlay(
    const std::string& name,
    const std::string& url,
    const std::vector<IAssetAccessor::THeader>& headers,
    const UrlTemplateRasterOverlayOptions& urlOptions,
    const CesiumRasterOverlays::RasterOverlayOptions& overlayOptions)
    : CesiumRasterOverlays::RasterOverlay(name, overlayOptions),
      _baseUrl(url),
      _headers(headers),
      _options(urlOptions) {}

UrlTemplateRasterOverlay::~UrlTemplateRasterOverlay() {}

Future<CesiumRasterOverlays::RasterOverlay::CreateTileProviderResult>
UrlTemplateRasterOverlay::createTileProvider(
    const CesiumRasterOverlays::CreateRasterOverlayTileProviderParameters&
        parameters) const {
  const CesiumAsync::AsyncSystem& asyncSystem =
      parameters.externals.asyncSystem;
  const auto& pAssetAccessor = parameters.externals.pAssetAccessor;
  const auto& pCreditSystem = parameters.externals.pCreditSystem;
  const auto& pPrepareRendererResources =
      parameters.externals.pPrepareRendererResources;
  const auto& pLogger = parameters.externals.pLogger;
  CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner =
      parameters.pOwner;

  std::string xmlUrlGetcapabilities =
      CesiumUtility::Uri::substituteTemplateParameters(
          "{baseUrl}",
          [this](const std::string& placeholder) {
            if (placeholder == "baseUrl") {
              return this->_baseUrl;
            } else if (placeholder == "version_") {
              return Uri::escape(this->_options.version);
            }
            // Keep other placeholders
            return "{" + placeholder + "}";
          });

  pOwner = pOwner ? pOwner : this;

  const std::optional<Credit> credit =
      this->_options.credit ? std::make_optional(pCreditSystem->createCredit(
                                  this->_options.credit.value(),
                                  pOwner->getOptions().showCreditsOnScreen))
                            : std::nullopt;
  // 3857      //Ellipsoid::WGS84,
  const auto projection =
      _options.projection.value_or(CesiumGeospatial::WebMercatorProjection(
          pOwner->getOptions().ellipsoid,
          _options.layers == "GCJ02" ? "GCJ02" : "WGS84"));
  CesiumGeospatial::GlobeRectangle tilingSchemeRectangle =
      CesiumGeospatial::WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE;
  // 4326
  // const auto projection = CesiumGeospatial::GeographicProjection();
  // CesiumGeospatial::GlobeRectangle tilingSchemeRectangle
  // =CesiumGeospatial::GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
  CesiumGeometry::Rectangle coverageRectangle =
      _options.coverageRectangle.value_or(
          projectRectangleSimple(projection, tilingSchemeRectangle));

  const int rootTilesX = 1;
  const int rootTilesY = 1;
  CesiumGeometry::QuadtreeTilingScheme tilingScheme =
      _options.tilingScheme.value_or(CesiumGeometry::QuadtreeTilingScheme(
          coverageRectangle,
          rootTilesX,
          rootTilesY));

  const CesiumAsync::IAssetAccessor::THeader cookie{
      "Cookie",
      "HWWAFSESID=*; HWWAFSESTIME=*"};
  std::vector<IAssetAccessor::THeader> headers_cookie(1, cookie);

  return asyncSystem
      .createResolvedFuture<RasterOverlay::CreateTileProviderResult>(
          new UrlTemplateTileProvider(
              pOwner,
              asyncSystem,
              pAssetAccessor,
              pCreditSystem,
              credit,
              pPrepareRendererResources,
              pLogger,
              projection,
              tilingScheme,
              coverageRectangle,
              _baseUrl,
              _baseUrl.find("tianditu") > 0 ? headers_cookie : _headers,
              _options.version,
              _options.layers,
              _options.format,
              _options.tileWidth < 1 ? 1 : uint32_t(_options.tileWidth),
              _options.tileHeight < 1 ? 1 : uint32_t(_options.tileHeight),
              _options.minimumLevel < 0 ? 0 : uint32_t(_options.minimumLevel),
              _options.maximumLevel < 0 ? 0 : uint32_t(_options.maximumLevel),
              _options.debug,
              _options.subdomains));
}

} // namespace CesiumHoloveser
