#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumRasterOverlays/QuadtreeRasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumHoloveser/ArcGisMapServerRasterOverlay.h>
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

class ArcGisMapServerTileProvider final
    : public CesiumRasterOverlays::QuadtreeRasterOverlayTileProvider {
public:
  ArcGisMapServerTileProvider(
      const IntrusivePointer<const CesiumRasterOverlays::RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
      std::optional<Credit> credit,
      const std::shared_ptr<CesiumRasterOverlays::IPrepareRasterOverlayRendererResources>&
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
      const std::string& tileMatrixSet,
      const std::string& style,
      const std::string& key,
      uint32_t width,
      uint32_t height,
      uint32_t minimumLevel,
      uint32_t maximumLevel,
      const std::string& epsg,
      bool debug)
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
        _style(style),
        _tileMatrixSet(tileMatrixSet),
        _key(key),
        _epsg(epsg) ,
        _debug(debug) {
    if (credit) {
      this->getCredits().emplace_back(*credit);
    }
  }

  virtual ~ArcGisMapServerTileProvider() {}

protected:
  virtual CesiumAsync::Future<CesiumRasterOverlays::LoadedRasterOverlayImage> loadQuadtreeTileImage(
      const CesiumGeometry::QuadtreeTileID& tileID) const override {

    CesiumRasterOverlays::LoadTileImageFromUrlOptions options;
    options.rectangle = this->getTilingScheme().tileToRectangle(tileID);
    options.moreDetailAvailable = tileID.level < this->getMaximumLevel();

    /* const CesiumGeospatial::GlobeRectangle tileRectangle =
        CesiumGeospatial::unprojectRectangleSimple(
            this->getProjection(),
            options.rectangle);*/

    const std::string urlTemplate =
        this->_url +
        "?Request=GetTile&Service=WMTS&Version={version}"
        "&Layer={layers}&Style={style}&TileMatrixSet={tilematrixset}"
        "&Format={format}"
        "&TileMatrix={tilematrix}&TileCol={tilecol}&TileRow={tilerow}";
    /* const auto radiansToDegrees = [](double rad) {
      return std::to_string(CesiumUtility::Math::radiansToDegrees(rad));
    };*/

    const std::map<std::string, std::string> urlTemplateMap = {
        {"baseUrl", this->_url},
        {"version", this->_version},
        {"layers", this->_layers},
        {"style", this->_style},
        {"format", this->_format},
        {"tilematrix",
         std::to_string(
             this->_epsg == "4490"? tileID.level + 1: tileID.level)}, // 4490差一级，可能是初始level为1的原因
        {"tilematrixset", this->_tileMatrixSet},
        {"tilerow",
         std::to_string(tileID.computeInvertedY(this->getTilingScheme()))},
        {"tilecol", std::to_string(tileID.x)}};

    std::string url = CesiumUtility::Uri::substituteTemplateParameters(
        urlTemplate,
        [&map = urlTemplateMap](const std::string& placeholder) {
          auto it = map.find(placeholder);
          return it == map.end() ? "{" + placeholder + "}"
                                 : Uri::escape(it->second);
        });

    if (_key.size() > 0) {
      url = url + "&" + _key;
    }
    
    if (url.find(_key) == url.npos && _key.size() > 0) {
      url = url + "&" + _key;
    }
    // 日志
    /*  SPDLOG_LOGGER_WARN(
       this->getLogger(),
       url,
       "TODO",
       "");*/
      if (_debug) {
      SPDLOG_LOGGER_INFO(this->getLogger(), url);
    }
    return this->loadTileImageFromUrl(url, this->_headers, std::move(options));
  }

private:
  std::string _url;
  std::vector<IAssetAccessor::THeader> _headers;
  std::string _version;
  std::string _layers;
  std::string _format;
  std::string _style;
  std::string _tileMatrixSet;
  std::string _key;
  std::string _epsg;
  bool _debug;
};

ArcGisMapServerRasterOverlay::ArcGisMapServerRasterOverlay(
    const std::string& name,
    const std::string& url,
    const std::vector<IAssetAccessor::THeader>& headers,
    const ArcGisMapServerRasterOverlayOptions& wmtsOptions,
    const CesiumRasterOverlays::RasterOverlayOptions& overlayOptions)
    : CesiumRasterOverlays::RasterOverlay(name, overlayOptions),
      _baseUrl(url),
      _headers(headers),
      _options(wmtsOptions) {}

ArcGisMapServerRasterOverlay::~ArcGisMapServerRasterOverlay() {}

static std::string getTiantiduCredit()
{
    const std::string credit =
    "<a href=\"https://www.esri.com\"><img "
    "src=\"data:image/"
    "png;base64,iVBORw0KGgoAAAANSUhEUgAAAEEAAAAkCAYAAADWzlesAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAADO9JREFUeNq0Wgl0jlca/pfvzyo6qNBSmhLLKE1kKEUtB9NTat+OYnBacwwJY19DZRC7sR41th60lWaizFSqRTOEw0lsrQSJGFIESSxJ/uRfv3nef+7Vt9f3p2E695z3fMt97/3ufe+7PO+9n9n0UzELsjKyiHdUdMZnVHTl2VyFe9nO7Kc/Io+4epUxmpWxeVkbr3hvUebgFf15GL9XUwZHndtAAYI09jGvIghOuoEwLOLeYiBoXr"
    "wGfZjYYOWAvWyMGlsk2YebXeV3NUEW1qcT5BBX4jUbCYEmHwwKEfdW1gEXgoWtiIlNRFeezcrkrQaTNSuraRYDdImrR1ylAALZBPnkXIJ0wRskeG2Cj3jsoFI2HhcfDDFWA9UBNdZZyc/PP4Z3HZYsWTLGbrffond0Xb9+/Qy6P3jw4F+HDx8+mu7XrVs3c+7cuX+i+3nz5o3n/Rw4cGAdf/7hhx9SZ8yYEcffHT9+/G/8uaSkJGvDhg3D8P3moNdXrlw5UtYVFxfnXL9+/V8PHz68grr2N2/eTC4tLb2E+9+Cotq1a/dOenr6njt37nxPdOrUqd0dO3bsjromoHBQKBPkEyFUB71MH"
    "6SPbNy4cRqfkMvlenzixImtqO/x3XffbXc6nSW5ubnpOTk5J1NTU/cQH91//fXXu3/88ccLy5cvj6d34B8gaBA9JyQk/OWjjz5aIu8Fz2DiWbZs2QLx/A4m0Qf9f/n48eNsPEeDfrdly5Y/U31UVNT7dJ04ceIsGseNGzfS6DkuLq4v8YE6Y/G+93g8XKZ6QUHBRVHfAPQC0xJfCRAv65EkeUP6gFx11JEkfw/qTc8ff/zxKofDUXrv3r08rOIBeU9CWbx48SLej5y4LGlpaf9YuHDhUv5OtqH+6Vty0riPAbWjheH8n3322VYpuG+//Xa5mGB7CGM8hKN7vV5dLfHx8WNI20E1aN4W"
    "P97YZyc7d+6MM5vNHRs2bDg3NjY23e12l5w8eZJWzIUJ9IdmlI4bNy4tICAgtHbt2hGdOnXaSe3oftu2bWmBgYFOn3MwmwcQLViwIJOeYVYJGGAZVuW2zWZzCZ6hoIGapnmknUMTQnr16vUeTOKydHqyHrx9t27dunro0KEfzJw5M4Pe3bp166Z0pHXr1g0Fj2EYCw8PD+N+SjNwUuSAKnxexOkswOWxZN63b9/MAQMGzIUwx5WXl99eunTpFLx+hJU/K9o/yM7OPhgZGdk5KSkpp0WLFv+Vrq7/na5nz57dR1dM6t7hw4e3DRkyJG7WrFlxgudzukIw58TzV3SF3Z+ByUzFbTk5O9j"
    "8fVH/JV3PnTv3uRijSdSR5/empKRkT5kypQxCC+UTxMKVQXuyWBT5WbiS4VFjIZLHWQsLN1ZFgFbm0U1KSNWUUMlDp9kAh0iNdCkRwiva2FjUsjJeJ5sYRYQwCGIYNGk8tC1UCuDQoUOb+vbtuxuPRUJ4FVwIFhZ7pUD45OXEbUpo9DIz8hgAFk0BORblWypm8BiQzkKnpoRnM+PxsEWhiYfFxMTUHTx4cDOYhg7tzM7IyLhNCiYEUEbCMxsAGYuCGjl4ClKE4GY+xCnIw95zBKqxvmyCOJqT7dws5ntZzLcoaJEjQiPUahMaESzudWEqhBEeiSuZvUvzA1+lxIMEhbD7QGYKUl0rBA"
    "gxC9vlq6IzNZZ9BYt+rMw8pBDLmSZZFBPQmBC8imaofo1roa5oKH82aQaaIH0CDTZM0sCBAxvBKbZ+7bXXGr3yyisN4ZjMDx48uAeAkofQdHbt2rUXhIpJKevMJwSLfqq3bt365enTp3eFh365SZMmBGpMFRUVZcAV1wFmzs2ZMyddtCkXk9ESExOjq1Wr9iLCbwAilA9xwrnlwimS4G2ffvppj1atWrWoWbNmbWCKAtj9V5MnT84cMWJEvTfeeKM+wqSFzCEoKMgJ3HEVgO6SkTlKMwgUgImwArn2DpMmTYrDALP0XyjEA9sbjTZtQZGij7qghqBWoK4AWPswkbLK+qHIsWPHjoXgf"
    "wvUhsZAAEflg+dfg0kuBlosUuvoO2jXl65qXWZm5g7UNRPIOIQLQqpcmECMJIAuRp1UVmiCACmTxAReFx+LhnPqV1hY+O9n6evIkSObSXCEHI0WASDtMMJ0uVHb7du3E6p9HxpxQK0DjN4r0Gc9kSZYeZiSNkuaUOv06dPTO3fuPNj0DAWgKWTFihVL+vfvT0J8kfohAsobV6tWrYbP0hf460pnLE2AF2jB21DvIKO2gO6FNB+ERJtaB+xjY37NN3+LogmkHi9s2rTp3bZt277LG8NuK5AopXbv3n0O7Gtsjx49ZmNye6GOD1RBwD9MFUKoSQSc30UdzJUrV26uWrVqP7D/lt27d+9/"
    "9OhRMas7gjYbhROzkv9R2wcHBwdWshjkYL1G7SBQTXGwTwQQLLIqWsGeGFAhVyFSO6C7Naj7ADRUJENDQGMjIiLmQl0LVLUbNWrUItSPhBNcodYhFyFklwAiYf0RNKZZs2YfFhUVXYcAvhFm0FFc++fl5eX4Mxto7JnRo0cvID4yHWSz70dHRw+khAxZ6yGVH8ndftS9DWokciWNx15fTN2zZ0+f6tWr1+LS279/fwYgcz4LPzJvdyGVLUFidFiVOIRAqx8KlQysZCdKboJUXL58uRAmMLFp06aLRbh1cGhrVEiD3nzzzTXIcU5R6gC6vXfv3kuIGgSIyq1Wq6cqpmdhiNAXFtu0adNe"
    "ZVq9enUWA0xywyVECC4AicwttQ2SrvpkYnfv3i1X6xo0aPAiJv2H+fPnt27UqFEN4YsCDBCk33Lt2rW8kSNHJuP2LqUc4kq+4KFAgg6LxeKtSl+a4hMC6tSp85QD27VrVy9I1U2SJaKYS/ZG8Rf5uhVXq91ud4aEhATINo0bN46glUQMv4aQV46MMpj3iRVvsGjRohFEENQtygCRmZ5B6DsqNNPFANJT5cyZM5RoPRBE/qREaJYEYm4aZ1WFwDG9ppoClebNm9czPV/xYXOo6J4xY8Z84I8Jgq9HBCDVfsKECR+mpqZ+gSQnRVQHGTm4CxcuXBP9l4qrneUNPtheVSFYKtkF/jUKqWbx"
    "2LFjUxBJViA82asSZvv06TPq+PHjE/D4GzI70jiVT+xDyBzDo8DhZyoWNXsD4Cn/FYVQLKgIofCfMIkhgKyr4bhO8pBoVGgvsEuXLq+SEIw0Qayyl5H+vIPUmJf2ZYOwz5twXE05U/369TfBZu+wvMBpkH7L3dwyYZ+l4uoRPL50FzCcQuAJstvIyMjacG5Rw4YN64b7V9XBxcbGdgJq/cZIE4TT0/2ceTyzJsiMj0JSxfnz50+rTECBUUq2aGd2WC7Izib+WFwdLJs0sczT1w+Q3d34+PhTSKQ2w4GeVL9LTtefY1Q2YEz/qxC8LIe3f/LJJ2kqU79+/WIGDRpUj+0L8N0lG7B6N+QG"
    "iS1btgxR9ha8gi949uzZ0UiENgBSR4iQyFNiL0zkrh+V/78XfjJDq1aWnJx85dixY8kqRE1KSopNSUkZ0K1btwjhsGpMmzatbVZW1nTy/JQbQHUXA26HMRul/gOQHkcBUK1BBGiJFHgtcMV7YqeXeEM7dOhQB4lXh6dCS1kZaZbDSBjinV6ZhsBkdAMz0o00SO4hhIrUl7K/7vfv37+hP0eBw8tBftFRpNNNExMThyMqlKp8SEXsADy5t1GM+qF6CHwe+hifm5t7Ta1PSEiYj7rWIhsMZaCPEkDyL+2PHj36hdqO3lGd4KkuYbN0jC5h22TPRT179pwCZ5j9rKqF0FWtd+/eL0kBA9Y2k"
    "RudvBB4og2al1CM+iFsgQFfJTCkaZrboL2DhUfd4NjAadROvHPyvUsLayxNghxaMWw0D1EhFiguqSrxXWZ/EN7IyZMnX5QHn127dk0Gxo+nnd6q9EHf2rx58zJgC1oxSrQKgR1cKl9YWJhdOFg329TlC1oBM3YYZJ8OubcozVZTJPjkzEEwOBGr1yIr+xz23xX23i48PPxVjiqRQV6GRuetXLkSbiPpCsPuTulzEAYPAh+cnzp1ao+YmJi31D5gevkwo3sZGRmn0M+RzMzMAhFtaGG0ixcvfpmfn39WbpNBC1zILK8KHqdykCsXszQ7O/sE8WMBNKGlbrxLF1HsSeQyV5JQBSrJUghLdD"
    "QmKB46ywTJFTKzfqqxftScwM1OjGXY/Vl0UU7IHcq3XMrutkz0QsX3bOwEWo5TfsNj9hMxjP5VCFR2fPl/AS4xMH7u71X6CWR92JQjer5t72AHLrpyKGRRhKbCZrNybhJg8HvBU+385Qv8DMKi/BjBEaKuHJK42YDU/x789cFhu1s5cFH/hTAp3/UqhzMm5cTM6G8br/qnyi8lTWYDoZiUP1TUEyc1Ble1D5OSA+gG7U0GR3b+fhUy+kVIN0Kb/xFgANrk0XIqRaL0AAAAAElFTkSuQmCC"
    "\" title=\"Esri Imagery\"/></a>";

  return credit;
}

Future<CesiumRasterOverlays::RasterOverlay::CreateTileProviderResult>
ArcGisMapServerRasterOverlay::createTileProvider(
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
          "{baseUrl}?request=GetCapabilities&version={version}&service=WMTS",
          [this](const std::string& placeholder) {
            if (placeholder == "baseUrl") {
              return this->_baseUrl;
            } else if (placeholder == "version") {
              return Uri::escape(this->_options.version);
            }
            // Keep other placeholders
            return "{" + placeholder + "}";
          });

  pOwner = pOwner ? pOwner : this;

  
  const std::optional<Credit> credit=std::make_optional(pCreditSystem->createCredit(getTiantiduCredit(),true));

  return pAssetAccessor->get(asyncSystem, xmlUrlGetcapabilities, this->_headers)
      .thenInMainThread(
          [pOwner,
           asyncSystem,
           pAssetAccessor,
           pCreditSystem,
           credit,
           pPrepareRendererResources,
           pLogger,
           options = this->_options,
           url = this->_baseUrl,
           headers =
               this->_headers](const std::shared_ptr<IAssetRequest>& pRequest)
              -> CreateTileProviderResult {
            const IAssetResponse* pResponse = pRequest->response();
            if (!pResponse) {
              /*  return
                 nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                   RasterOverlayLoadType::TileProvider,
                   std::move(pRequest),
                   "No response received from web map service."}); */
            }

            CesiumGeospatial::GlobeRectangle tilingSchemeRectangle =
                CesiumGeospatial::GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
            CesiumGeospatial::Projection projection;
            uint32_t rootTilesX;
            uint32_t rootTilesY;

            if (options.epsg == "4490") // 4326 4490
            {
              projection = CesiumGeospatial::GeographicProjection();
              tilingSchemeRectangle = CesiumGeospatial::GeographicProjection::
                  MAXIMUM_GLOBE_RECTANGLE;
              rootTilesX = 2;
              rootTilesY = 1;
            } else // 3857,900913
            {
              projection = CesiumGeospatial::WebMercatorProjection(
                  pOwner->getOptions().ellipsoid,
                  "GCJ02");
              tilingSchemeRectangle = CesiumGeospatial::WebMercatorProjection::
                  MAXIMUM_GLOBE_RECTANGLE;
              rootTilesX = 1;
              rootTilesY = 1;
            }

            CesiumGeometry::Rectangle coverageRectangle =
                projectRectangleSimple(projection, tilingSchemeRectangle);

            CesiumGeometry::QuadtreeTilingScheme tilingScheme(
                projectRectangleSimple(projection, tilingSchemeRectangle),
                rootTilesX,
                rootTilesY);

            // coverageRectangle=CesiumGeometry::Rectangle(-180,-90, 180,90);
            //coverageRectangle=CesiumGeometry::Rectangle(30.710719079012677,116.10358013377254, 35.21265930204362,122.09030402444137);

            /* CesiumGeometry::Rectangle
               coverageRectangle=projectRectangleSimple( projection,
                     CesiumGeospatial::GlobeRectangle::fromDegrees(
                         -180.0,-90.0, 180.0,90.0));
*/
            /*const auto projection = CesiumGeospatial::GeographicProjection();

           CesiumGeospatial::GlobeRectangle tilingSchemeRectangle =
               CesiumGeospatial::GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;

           CesiumGeometry::Rectangle coverageRectangle =
               projectRectangleSimple(projection, tilingSchemeRectangle);

           const int rootTilesX = 2;
           const int rootTilesY = 1;
           CesiumGeometry::QuadtreeTilingScheme tilingScheme(
               coverageRectangle,
               rootTilesX,
               rootTilesY);  */

            // 4326
            // const auto projection = CesiumGeospatial::GeographicProjection();
            // CesiumGeospatial::GlobeRectangle tilingSchemeRectangle
            // =CesiumGeospatial::GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
            /* CesiumGeometry::Rectangle coverageRectangle =
   projectRectangleSimple(
       projection,
       CesiumGeospatial::GlobeRectangle::fromDegrees(
           -20037508.3427892,-20037508.3427892,
           20037508.3427892,20037508.3427892));  */

            // 3857
            /*
            const auto projection = CesiumGeospatial::WebMercatorProjection();
            CesiumGeospatial::GlobeRectangle tilingSchemeRectangle =
                CesiumGeospatial::WebMercatorProjection::
                    MAXIMUM_GLOBE_RECTANGLE;

            CesiumGeometry::Rectangle coverageRectangle =
                projectRectangleSimple(projection, tilingSchemeRectangle);
            const int rootTilesX = 1;
            const int rootTilesY = 1;
            CesiumGeometry::QuadtreeTilingScheme tilingScheme(
                coverageRectangle,
                rootTilesX,
                rootTilesY);
             */

            const CesiumAsync::IAssetAccessor::THeader cookie{
                "Cookie",
                "HWWAFSESID=; HWWAFSESTIME="};

            std::vector<IAssetAccessor::THeader> headers_cookie(1, cookie);

            return new ArcGisMapServerTileProvider(
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
                url,
                headers,
                options.version,
                options.layers,
                options.format,
                options.tileMatrixSet,
                options.style,
                options.key,
                options.tileWidth < 1 ? 1 : uint32_t(options.tileWidth),
                options.tileHeight < 1 ? 1 : uint32_t(options.tileHeight),
                options.minimumLevel < 0 ? 0 : uint32_t(options.minimumLevel),
                options.maximumLevel < 0 ? 0 : uint32_t(options.maximumLevel),
                options.epsg,
                options.debug);
          });
}

} // namespace CesiumHoloveser
